# boot-contract.md

## Status

Approved for first bring-up target.

## Purpose

This document defines the initial boot contract between the bootloader and the LumenOS kernel for the first execution target:

- **Architecture:** x86_64
- **Execution environment:** QEMU system emulation
- **Bootloader / protocol:** Limine Boot Protocol
- **Protocol baseline:** Limine base revision 6
- **Initial paging mode:** 4-level paging

The goal is not maximum flexibility. The goal is a minimal, explicit, reproducible handoff that is easy to implement, debug, and evolve.

---

## 1. Why this first target

### Chosen first target

LumenOS boots first on **x86_64 under QEMU** using **Limine**.

### Rationale

This combination is the shortest path to a serious kernel bring-up:

- x86_64 is well understood and gives direct access to the mechanisms we need to validate first: exceptions, paging, interrupts, timers, ACPI, APIC, and SMP.
- QEMU gives a repeatable, scriptable machine model for fast boot-test-debug loops.
- Limine provides a modern, explicit request/response boot protocol and serves as the reference implementation of that protocol.
- Limine already provides an official x86_64 kernel template, which reduces accidental bootstrapping complexity.

### Non-goals of this decision

This is **not** a statement that x86_64 is the only long-term architecture.
It is only the first execution target used to validate the kernel architecture under controlled conditions.

---

## 2. Protocol baseline

The kernel shall implement the **Limine Boot Protocol** and explicitly request **base revision 6**.

### Rules

1. The kernel must not rely on deprecated base revisions.
2. The kernel must explicitly declare its requested base revision.
3. The kernel must use Limine request structures only through pinned protocol headers committed in-repo.
4. Limine version and protocol header version must be pinned in the build and documented in the repository.

---

## 3. Entry point contract

### Entry symbol

The kernel entry point is the normal executable entry point defined by the kernel image.

For the first bring-up, no custom Limine Entry Point feature is required unless the linker layout forces it.

### ABI assumptions

At handoff on x86_64:

- execution follows the **System V ABI without FP/SIMD** for the protocol boundary
- all pointers supplied by Limine are 64-bit
- non-NULL Limine pointers already include the **HHDM** offset

### Return behavior

The kernel entry point must never return.

If control returns, the kernel shall halt the CPU in a fail-fast loop.

---

## 4. Expected CPU and machine state at kernel entry

At first kernel instruction, the following state is assumed.

### Execution mode

- CPU is already in **64-bit long mode**
- protected mode and paging are enabled
- PAE is enabled
- write protection is enabled
- NX is enabled if supported
- paging mode for the first target is **4-level paging**

### Instruction pointer and stack

- `RIP` points to the kernel entry point
- `RSP` points to a bootloader-provided stack
- the boot stack is in **bootloader-reclaimable memory**
- minimum boot stack size is **64 KiB**
- an invalid return address (`0`) is already pushed

### Segmentation and descriptor tables

- bootloader enters with 64-bit code/data segments loaded
- a bootloader GDT exists
- **IDT state is undefined**
- kernel must load its **own IDT**
- kernel should install its **own GDT** immediately instead of relying on bootloader descriptors longer than necessary

### Flags and interrupt state

- IF is cleared on entry
- VM flag is cleared
- direction flag is cleared
- other flags are treated as undefined unless explicitly needed later

### Interrupt controllers

- legacy PIC IRQs are masked
- relevant I/O APIC redirection entries are masked by the bootloader

### Boot services

- if boot happened via EFI/UEFI, boot services are already exited before handoff

---

## 5. Memory layout and paging contract

### Kernel placement

- the kernel image is loaded in the **higher half**
- the executable is loaded at or above `0xffffffff80000000`
- the loaded kernel image is physically contiguous
- physical placement is **not** otherwise assumed stable

### Authoritative way to locate the kernel image

The kernel shall use the **Executable Address feature** as the authoritative source for:

- kernel physical base
- kernel virtual base

The memory map entry of type executable/modules is informational only and must **not** be used as the source of truth for locating the kernel image.

### Paging ownership

For the first bring-up, the kernel may temporarily execute on Limine-provided page tables during the earliest boot phase.

However, early boot must establish a clear transfer of ownership:

1. inspect Limine-provided memory layout and HHDM offset
2. bring up minimal early memory access helpers
3. install kernel-owned page tables as soon as practical
4. stop depending on bootloader page tables beyond early bring-up

### First paging policy

For the first target, the kernel shall standardize on:

- **4-level paging**
- higher-half kernel mapping
- HHDM-aware early physical access
- no general-purpose heap-backed virtual memory subsystem in phase 0

---

## 6. Required boot data passed to the kernel

The following Limine features are part of the mandatory boot contract for the first bring-up.

### Mandatory

#### 6.1 Memory Map

The kernel must request the Limine memory map and consume it as the authoritative early RAM topology.

The early kernel must at minimum understand these classes:

- usable
- reserved
- ACPI reclaimable
- ACPI NVS
- bad memory
- bootloader reclaimable
- executable and modules
- framebuffer
- reserved mapped

Early policy:

- allocate only from **usable** memory
- treat **bootloader reclaimable** as unavailable until all required bootloader data has been copied or is no longer needed
- never treat executable/modules or framebuffer ranges as allocatable RAM

#### 6.2 HHDM

The kernel must request the HHDM offset.

This is required so that early boot can safely interpret Limine pointers and access physical memory through the higher-half direct map.

#### 6.3 Executable Address

The kernel must request the executable address feature in order to determine:

- physical base of the kernel image
- virtual base of the kernel image

#### 6.4 Bootloader Info

The kernel should request bootloader name/version for diagnostics and panic reporting.

### Required when available / enabled

#### 6.5 RSDP

The kernel must request the ACPI RSDP pointer.

Even if ACPI parsing is not fully implemented in the first week, this contract is fixed now because later APIC, timer, and SMP work will rely on ACPI discovery.

#### 6.6 MP (Multiprocessor)

The kernel should include the Limine MP request from the start, even if only the BSP is used initially.

Policy for phase 0:

- BSP-only execution is acceptable for the first successful boot to userspace init
- AP startup may remain disabled in kernel logic at first
- MP metadata must still be accepted and logged when provided

### Optional, not prioritized

#### 6.7 Framebuffer

Framebuffer support is optional in phase 0.

It may be requested, but it is not required for successful bring-up.
Serial output remains the primary debug channel.

---

## 7. Earliest kernel responsibilities

The first kernel code owns exactly these responsibilities, in this order.

### Stage A — preserve the contract

1. establish a tiny `early_boot_ctx`
2. capture and validate required Limine responses
3. fail hard if mandatory responses are missing
4. record HHDM offset, memory map pointer/count, executable addresses, and optional ACPI/MP metadata

### Stage B — make exceptions safe

5. install kernel-owned GDT
6. install kernel-owned IDT
7. install minimal exception stubs
8. provide a guaranteed panic path for faults during early boot

### Stage C — make output deterministic

9. initialize serial console as the first trusted output channel
10. print boot banner, protocol revision, bootloader info, and key address ranges

### Stage D — own memory deliberately

11. parse Limine memory map
12. reserve kernel image and all boot-critical regions
13. initialize a minimal physical page allocator from usable memory only
14. retain bootloader-reclaimable memory as reserved until boot data is no longer needed
15. establish kernel-owned early page tables
16. switch to kernel-owned page tables

### Stage E — prepare platform discovery

17. validate RSDP presence when requested
18. initialize minimal ACPI table walking capability
19. record BSP LAPIC / CPU identity information
20. accept but do not yet fully exploit MP metadata

### Stage F — hand off to the next boot phase

21. enter `kmain_early()`
22. continue into interrupt controller, timer, scheduler, and IPC bring-up

---

## 8. Serial console policy

Serial output is the **primary** phase-0 debug interface.

### Contract

- COM1-based early serial output shall be available before heap initialization
- early panic output must work without framebuffer support
- all early boot milestones must be emitted over serial
- first successful milestone is:
  - boot banner
  - memory map summary
  - kernel physical/virtual base
  - HHDM offset
  - RSDP presence
  - BSP CPU/APIC identity if available

### Why

A kernel without deterministic early text output is blind.
Framebuffer can wait. Serial cannot.

---

## 9. Ownership and lifetime rules for bootloader data

All Limine responses and associated structures are placed in **bootloader-reclaimable memory**.

Therefore:

1. the kernel may read them immediately during early boot
2. the kernel must copy or internalize data it still needs before reclaiming that memory
3. reclaiming bootloader-reclaimable memory is forbidden until:
   - required Limine data is copied, or
   - the kernel has explicitly decided to keep those pages reserved longer

This rule is non-negotiable. Violating it creates nondeterministic early-boot corruption.

---

## 10. Initial failure policy

The first implementation should fail loudly and narrowly.

### Hard-fail conditions

The kernel must stop with explicit panic output if any of the following is true:

- Limine base revision negotiation failed
- memory map response missing
- HHDM response missing
- executable address response missing
- serial console initialization failed
- own IDT could not be installed
- own early page tables could not be established
- required ACPI pointer missing once ACPI is marked mandatory in code

### Soft-fail conditions

The kernel may continue with degraded behavior when:

- framebuffer is absent
- MP response is absent in BSP-only phase if SMP has not yet been enabled as mandatory
- ACPI parsing is not yet complete, as long as current boot stage does not require it

---

## 11. Explicit non-goals for phase 0

The following are intentionally out of scope for the boot contract itself:

- full VMM design
- kernel heap as a general allocator
- rich graphics console
- user-facing boot UI
- NUMA policy
- hotplug CPU handling
- full SMP scheduling
- complex module loading policy

The boot contract exists to get us from bootloader handoff to controlled kernel ownership.
Nothing more.

---

## 12. Minimal request set for first implementation

The first implementation shall wire these Limine requests into the kernel image:

- base revision 6 tag
- bootloader info
- HHDM
- memory map
- executable address
- RSDP
- MP
- optional framebuffer
- optional stack size request if the default 64 KiB stack becomes insufficient

---

## 13. Kernel-side implementation notes

### Recommended early structures

```c
struct early_boot_ctx {
    uint64_t hhdm_offset;

    const struct limine_memmap_response *memmap;
    const struct limine_executable_address_response *exe_addr;
    const struct limine_rsdp_response *rsdp;
    const struct limine_mp_response *mp;
    const struct limine_bootloader_info_response *bootloader;
    const struct limine_framebuffer_response *framebuffer;

    uint64_t kernel_phys_base;
    uint64_t kernel_virt_base;
};
```

### Recommended early boot call chain

```text
_start
  -> early_capture_limine()
  -> early_serial_init()
  -> early_install_gdt()
  -> early_install_idt()
  -> early_parse_memmap()
  -> early_pmm_init()
  -> early_paging_init()
  -> early_acpi_init()
  -> kmain_early()
```

---

## 14. Final architectural position

For the first execution target, LumenOS standardizes on:

- **Bootloader:** Limine
- **Architecture:** x86_64
- **Machine model:** QEMU
- **Protocol baseline:** Limine base revision 6
- **Initial paging mode:** 4-level paging
- **Primary early debug channel:** serial console
- **Mandatory boot data:** memory map, HHDM, executable address
- **Expected next-layer platform data:** RSDP and MP metadata
- **Early kernel ownership boundary:** own GDT, own IDT, then own page tables

This is the boot contract.

Everything after this is kernel policy, not boot ambiguity.
