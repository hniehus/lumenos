BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso_root
ISO := $(BUILD_DIR)/lumenos.iso
BOOTLOG := $(BUILD_DIR)/boot.log

KERNEL := $(BUILD_DIR)/kernel.elf
ROOT_TASK := $(BUILD_DIR)/bootstrap.elf
GENERATED_DIR := $(BUILD_DIR)/generated
KERNEL_VERSION_H := $(GENERATED_DIR)/version.h
KERNEL_VERSION_TXT := $(BUILD_DIR)/kernel_version.txt
ROOT_TASK_EXTRA_CFLAGS ?=
FAULT_ADDR ?= 0xffffffff80000000

include version/version.mk

LIMINE_DIR := third_party/limine
LIMINE := $(LIMINE_DIR)/limine
LIMINE_BIOS_SYS := $(LIMINE_DIR)/limine-bios.sys
LIMINE_BIOS_CD := $(LIMINE_DIR)/limine-bios-cd.bin
LIMINE_UEFI_CD := $(LIMINE_DIR)/limine-uefi-cd.bin
LIMINE_EFI := $(LIMINE_DIR)/BOOTX64.EFI

.PHONY: all kernel root-task version print-version iso-root image run smoke clean FORCE

all: image

kernel: FORCE $(KERNEL)

root-task: $(ROOT_TASK)

FORCE:

$(KERNEL_VERSION_H) $(KERNEL_VERSION_TXT): FORCE version/version.mk version/build.counter scripts/generate-kernel-version.sh
	@mkdir -p $(BUILD_DIR) $(GENERATED_DIR)
	@bash ./scripts/generate-kernel-version.sh \
		version/version.mk \
		version/build.counter \
		$(KERNEL_VERSION_H) \
		$(KERNEL_VERSION_TXT)

$(KERNEL): kernel/src/main.c kernel/include/limine.h kernel/linker.ld $(KERNEL_VERSION_H)
	clang kernel/src/main.c \
		-std=c23 \
		-ffreestanding \
		-fno-stack-protector \
		-fno-pic \
		-fno-asynchronous-unwind-tables \
		-fno-unwind-tables \
		-m64 \
		-mcmodel=kernel \
		-mno-red-zone \
		-nostdlib \
		-nostartfiles \
		-fuse-ld=lld \
		-Wall \
		-Wextra \
		-Wl,-T,kernel/linker.ld \
		-Wl,-m,elf_x86_64 \
		-Wl,-static \
		-Wl,-no-pie \
		-Wl,--build-id=none \
		-Wl,-z,max-page-size=0x1000 \
		-Ikernel/include \
		-I$(GENERATED_DIR) \
		-o $(KERNEL)

$(ROOT_TASK): FORCE root-task/src/main.c
	@mkdir -p $(BUILD_DIR)
	clang root-task/src/main.c \
		-std=c23 \
		-ffreestanding \
		-fno-stack-protector \
		-fno-pic \
		-fno-asynchronous-unwind-tables \
		-fno-unwind-tables \
		-m64 \
		-mno-red-zone \
		-static \
		-nostdlib \
		-nostartfiles \
		-fuse-ld=lld \
		-Wall \
		-Wextra \
		$(ROOT_TASK_EXTRA_CFLAGS) \
		-Wl,-m,elf_x86_64 \
		-Wl,-e,_start \
		-Wl,--build-id=none \
		-o $(ROOT_TASK)

iso-root: kernel root-task
	@rm -rf $(ISO_ROOT)
	@mkdir -p $(ISO_ROOT)/boot
	@mkdir -p $(ISO_ROOT)/boot/limine
	@mkdir -p $(ISO_ROOT)/EFI/BOOT
	cp limine.conf $(ISO_ROOT)/limine.conf
	cp limine.conf $(ISO_ROOT)/boot/limine/limine.conf
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp $(ROOT_TASK) $(ISO_ROOT)/boot/bootstrap.elf
	cp $(LIMINE_BIOS_SYS) $(ISO_ROOT)/limine-bios.sys
	cp $(LIMINE_BIOS_SYS) $(ISO_ROOT)/boot/limine/limine-bios.sys
	cp $(LIMINE_BIOS_CD) $(ISO_ROOT)/limine-bios-cd.bin
	cp $(LIMINE_UEFI_CD) $(ISO_ROOT)/limine-uefi-cd.bin
	cp $(LIMINE_EFI) $(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI

version: print-version

print-version:
	@. ./version/version.mk; \
	current=$$(cat version/build.counter 2>/dev/null || echo 0); \
	next=$$(awk -v current="$$current" 'BEGIN { printf "%05d", current + 1 }'); \
	printf '%s\n' "$$MAJOR.$$MINOR.$$PATCH:$$next"

image: iso-root
	xorriso -as mkisofs -R -r -J \
		-b limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(ISO_ROOT) \
		-o $(ISO)
	$(LIMINE) bios-install $(ISO)

run: image
	@mkdir -p $(BUILD_DIR)
	@rm -f $(BOOTLOG)
	./run-qemu.sh

smoke: run
	@grep -q "kernel: early boot ok" $(BOOTLOG)
	@grep -q "kernel: entering user mode" $(BOOTLOG)
	@grep -q "user: hello lumen" $(BOOTLOG)
	@grep -q "kernel: syscall handled" $(BOOTLOG)
	@grep -q "vmo: create id=" $(BOOTLOG)
	@grep -q "vmo: map id=" $(BOOTLOG)
	@grep -q "vmo: cross-vmo overlap rejected" $(BOOTLOG)
	@grep -q "vmo: unmap id=" $(BOOTLOG)
	@grep -q "vmo: drop id=" $(BOOTLOG)
	@grep -q "kernel: user fault page" $(BOOTLOG)
	@grep -q "kernel: user fault addr = $(FAULT_ADDR)" $(BOOTLOG)

smoke-guard:
	$(MAKE) ROOT_TASK_EXTRA_CFLAGS=-DROOT_TASK_FAULT_GUARD_PAGE FAULT_ADDR=0x00007ffffffdf010 smoke

clean:
	rm -rf $(BUILD_DIR)
