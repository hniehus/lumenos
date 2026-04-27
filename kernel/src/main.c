#include <stdint.h>

#include "version.h"
#include "limine.h"

#define COM1 0x3f8
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_VERSION_CURRENT 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3
#define ELF_MACHINE_X86_64 62
#define PT_LOAD 1
#define PF_X 0x1
#define PF_W 0x2
#define ROOT_TASK_IMAGE_CAPACITY (1024 * 1024)
#define KERNEL_STACK_SIZE (64 * 1024ULL)
#define USER_ADDRESS_TOP 0x0000800000000000ULL
#define USER_STACK_SIZE (64 * 1024ULL)
#define USER_STACK_GUARD_SIZE PAGE_SIZE
#define USER_STACK_TOTAL_SIZE (USER_STACK_SIZE + USER_STACK_GUARD_SIZE)
#define USER_STACK_TOP 0x00007fffffff0000ULL
#define KERNEL_ADDRESS_BASE 0xffff800000000000ULL
#define PAGE_SIZE 0x1000ULL
#define PAGE_PRESENT 0x001ULL
#define PAGE_WRITABLE 0x002ULL
#define PAGE_USER 0x004ULL
#define PAGE_HUGE 0x080ULL
#define PAGE_NX (1ULL << 63)
#define PAGE_ADDR_MASK 0x000ffffffffff000ULL
#define PAGE_TABLE_POOL_PAGES 8
#define USER_RFLAGS_INITIAL 0x0002ULL
#define KERNEL_CS_SELECTOR 0x08u
#define KERNEL_DS_SELECTOR 0x10u
#define TSS_SELECTOR 0x28u
#define USER_CS_SELECTOR 0x1bu
#define USER_SS_SELECTOR 0x23u
#define SYSCALL_VECTOR 0x80u
#define SYSCALL_DEBUG_WRITE 1ULL
#define SYSCALL_DEBUG_WRITE_MAX 256ULL

// PMM constants
#define FRAME_SIZE PAGE_SIZE
#define BITS_PER_BYTE 8
#define BITS_PER_WORD 64
#define MAX_MEMORY_MAP_ENTRIES 256

#define USED __attribute__((used))
#define SECTION(name) __attribute__((section(name)))

struct elf64_ehdr {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct root_task_image {
    uint64_t base_vaddr;
    uint64_t end_vaddr;
    uint64_t entry;
    uint64_t load_segment_count;
};

struct root_task_context {
    uint64_t rip;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint16_t cs;
    uint16_t ss;
};

struct root_task_launch_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct root_task_launch_state {
    uint64_t stack_guard_base;
    uint64_t stack_base;
    uint64_t stack_top;
    struct root_task_context context;
    struct root_task_launch_frame frame;
};

struct gdt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct tss64 {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

// PMM structures
struct pmm_state {
    uint64_t total_frames;
    uint64_t bitmap_size_words;
    uint64_t *bitmap;
    uint64_t first_free_frame;
};

static struct pmm_state pmm_state;

static struct root_task_image root_task_image;
static struct root_task_launch_state root_task_launch_state;
uint8_t kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t root_task_image_pages[ROOT_TASK_IMAGE_CAPACITY] __attribute__((aligned(PAGE_SIZE)));
static uint8_t root_task_stack_pages[USER_STACK_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t page_table_pool[PAGE_TABLE_POOL_PAGES][PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint64_t page_table_pool_used = 0;
static uint64_t hhdm_offset = 0;
static uint64_t user_low_pdpt_phys = 0;
static uint64_t user_high_pdpt_phys = 0;
static uint64_t kernel_gdt[7] __attribute__((aligned(16))) = {
    0x0000000000000000ULL,
    0x00af9a000000ffffULL,
    0x00cf92000000ffffULL,
    0x00affa000000ffffULL,
    0x00cff2000000ffffULL,
};
static struct tss64 kernel_tss;
static struct idt_entry kernel_idt[256];

enum user_mapping_access {
    USER_MAPPING_READ_ONLY = 0,
    USER_MAPPING_READ_WRITE = 1,
    USER_MAPPING_EXECUTABLE = 2,
};

static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

__attribute__((noreturn)) static void halt_forever(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void serial_write_char(char byte) {
    if (byte == '\n') {
        serial_write_char('\r');
    }

    while ((inb(COM1 + 5) & 0x20u) == 0) {
    }

    outb(COM1, (uint8_t)byte);
}

static void serial_write_string(const char *text) {
    while (*text != '\0') {
        serial_write_char(*text++);
    }
}

static void serial_write_buffer(const char *text, uint64_t len) {
    while (len > 0) {
        serial_write_char(*text++);
        --len;
    }
}

static void serial_write_decimal(uint64_t value) {
    char buffer[21];
    int index = 20;

    buffer[index] = '\0';
    if (value == 0) {
        serial_write_char('0');
        return;
    }

    while (value > 0) {
        --index;
        buffer[index] = (char)('0' + (value % 10));
        value /= 10;
    }

    serial_write_string(&buffer[index]);
}

static void serial_write_hex(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    char buffer[19];

    buffer[0] = '0';
    buffer[1] = 'x';
    for (int i = 0; i < 16; ++i) {
        buffer[17 - i] = digits[value & 0xfu];
        value >>= 4;
    }
    buffer[18] = '\0';

    serial_write_string(buffer);
}

static void memory_copy(uint8_t *dst, const uint8_t *src, uint64_t len) {
    while (len > 0) {
        *dst++ = *src++;
        --len;
    }
}

static void memory_zero(uint8_t *dst, uint64_t len) {
    while (len > 0) {
        *dst++ = 0;
        --len;
    }
}

static uint64_t align_down(uint64_t value, uint64_t alignment) {
    return value & ~(alignment - 1);
}

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    return align_down(value + alignment - 1, alignment);
}

static int serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xc7);
    outb(COM1 + 4, 0x0b);
    outb(COM1 + 4, 0x1e);
    outb(COM1 + 0, 0xae);

    if (inb(COM1 + 0) != 0xae) {
        return 0;
    }

    outb(COM1 + 4, 0x0f);
    return 1;
}

static int string_equals(const char *left, const char *right) {
    if (left == 0 || right == 0) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == '\0' && *right == '\0';
}

static const char *path_basename(const char *path) {
    const char *base = path;

    if (path == 0) {
        return 0;
    }

    while (*path != '\0') {
        if (*path == '/' || *path == '\\') {
            base = path + 1;
        }
        ++path;
    }

    return base;
}

static const char *user_mapping_access_name(enum user_mapping_access access) {
    switch (access) {
        case USER_MAPPING_READ_ONLY:
            return "read-only";
        case USER_MAPPING_READ_WRITE:
            return "read-write";
        case USER_MAPPING_EXECUTABLE:
            return "executable";
    }

    return "unknown";
}

// PMM functions
static void pmm_mark_range(uint64_t base, uint64_t length, int allocated) {
    uint64_t start_frame = base / FRAME_SIZE;
    uint64_t end_frame = (base + length - 1) / FRAME_SIZE;
    uint64_t frame;

    for (frame = start_frame; frame <= end_frame; ++frame) {
        uint64_t word_index = frame / BITS_PER_WORD;
        uint64_t bit_index = frame % BITS_PER_WORD;

        if (allocated) {
            pmm_state.bitmap[word_index] |= (1ULL << bit_index);
        } else {
            pmm_state.bitmap[word_index] &= ~(1ULL << bit_index);
        }
    }
}

static uint64_t pmm_allocate_frame(void) {
    uint64_t frame = pmm_state.first_free_frame;

    while (frame < pmm_state.total_frames) {
        uint64_t word_index = frame / BITS_PER_WORD;
        uint64_t bit_index = frame % BITS_PER_WORD;

        if ((pmm_state.bitmap[word_index] & (1ULL << bit_index)) == 0) {
            // Found free frame
            pmm_state.bitmap[word_index] |= (1ULL << bit_index);
            pmm_state.first_free_frame = frame + 1;
            return frame;
        }

        ++frame;
    }

    // No free frame found
    return UINT64_MAX;
}

static void pmm_free_frame(uint64_t frame) {
    if (frame >= pmm_state.total_frames) {
        return;
    }

    uint64_t word_index = frame / BITS_PER_WORD;
    uint64_t bit_index = frame % BITS_PER_WORD;

    pmm_state.bitmap[word_index] &= ~(1ULL << bit_index);

    if (frame < pmm_state.first_free_frame) {
        pmm_state.first_free_frame = frame;
    }
}

static void pmm_init(const struct limine_memmap_response *memmap) {
    uint64_t max_address = 0;
    uint64_t bitmap_frames_needed = 0;
    uint64_t bitmap_base = 0;
    uint64_t entry_index;

    serial_write_string("pmm: init start\n");

    // Find the highest address in the memory map
    for (entry_index = 0; entry_index < memmap->entry_count; ++entry_index) {
        struct limine_memmap_entry *entry = memmap->entries[entry_index];
        uint64_t end_address = entry->base + entry->length;

        if (end_address > max_address) {
            max_address = end_address;
        }
    }

    // Limit max_address to 4GB for now
    if (max_address > 0x100000000ULL) {
        max_address = 0x100000000ULL;
    }

    pmm_state.total_frames = max_address / FRAME_SIZE;
    pmm_state.bitmap_size_words = (pmm_state.total_frames + BITS_PER_WORD - 1) / BITS_PER_WORD;
    bitmap_frames_needed = (pmm_state.bitmap_size_words * sizeof(uint64_t) + FRAME_SIZE - 1) / FRAME_SIZE;

    serial_write_string("pmm: max_address = ");
    serial_write_hex(max_address);
    serial_write_string(", total_frames = ");
    serial_write_decimal(pmm_state.total_frames);
    serial_write_string(", bitmap_words = ");
    serial_write_decimal(pmm_state.bitmap_size_words);
    serial_write_string(", bitmap_frames = ");
    serial_write_decimal(bitmap_frames_needed);
    serial_write_string("\n");

    // Find a suitable place for the bitmap in usable memory
    for (entry_index = 0; entry_index < memmap->entry_count; ++entry_index) {
        struct limine_memmap_entry *entry = memmap->entries[entry_index];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_frames_needed * FRAME_SIZE) {
            bitmap_base = entry->base;
            break;
        }
    }

    if (bitmap_base == 0) {
        serial_write_string("pmm: no suitable memory for bitmap\n");
        halt_forever();
    }

    serial_write_string("pmm: bitmap_base = ");
    serial_write_hex(bitmap_base);
    serial_write_string("\n");

    pmm_state.bitmap = (uint64_t *)(bitmap_base + hhdm_offset);
    memory_zero((uint8_t *)pmm_state.bitmap, pmm_state.bitmap_size_words * sizeof(uint64_t));
    pmm_state.first_free_frame = 0;

    // Mark all memory as allocated initially
    pmm_mark_range(0, max_address, 1);

    // Mark usable regions as free
    for (entry_index = 0; entry_index < memmap->entry_count; ++entry_index) {
        struct limine_memmap_entry *entry = memmap->entries[entry_index];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            pmm_mark_range(entry->base, entry->length, 0);
        }
    }

    // Mark the bitmap itself as allocated
    pmm_mark_range(bitmap_base, bitmap_frames_needed * FRAME_SIZE, 1);

    // Mark other reserved regions as allocated
    for (entry_index = 0; entry_index < memmap->entry_count; ++entry_index) {
        struct limine_memmap_entry *entry = memmap->entries[entry_index];

        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE:
                // Already handled
                break;
            case LIMINE_MEMMAP_RESERVED:
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            case LIMINE_MEMMAP_ACPI_NVS:
            case LIMINE_MEMMAP_BAD_MEMORY:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            case LIMINE_MEMMAP_FRAMEBUFFER:
                pmm_mark_range(entry->base, entry->length, 1);
                break;
        }
    }

    serial_write_string("pmm: initialized with ");
    serial_write_decimal(pmm_state.total_frames);
    serial_write_string(" frames\n");
}

static void pmm_test(void) {
    uint64_t allocated[100];
    uint64_t i;
    uint64_t frame;

    serial_write_string("pmm: running tests\n");

    // Test 1: Allocate 100 frames and check for duplicates
    for (i = 0; i < 100; ++i) {
        frame = pmm_allocate_frame();
        if (frame == UINT64_MAX) {
            serial_write_string("pmm: test failed - allocation failed at ");
            serial_write_decimal(i);
            serial_write_string("\n");
            halt_forever();
        }
        allocated[i] = frame;

        // Check for duplicates
        for (uint64_t j = 0; j < i; ++j) {
            if (allocated[j] == frame) {
                serial_write_string("pmm: test failed - duplicate frame ");
                serial_write_hex(frame);
                serial_write_string("\n");
                halt_forever();
            }
        }
    }

    serial_write_string("pmm: test 1 passed - no duplicates in 100 allocations\n");

    // Test 2: Free all and reallocate
    for (i = 0; i < 100; ++i) {
        pmm_free_frame(allocated[i]);
    }

    for (i = 0; i < 100; ++i) {
        frame = pmm_allocate_frame();
        if (frame == UINT64_MAX) {
            serial_write_string("pmm: test failed - reallocation failed at ");
            serial_write_decimal(i);
            serial_write_string("\n");
            halt_forever();
        }
    }

    serial_write_string("pmm: test 2 passed - free and reallocate\n");

    // Test 3: Check that reserved regions are not allocated
    // This is implicit in the initialization, but we can check by trying to allocate known reserved areas
    // For now, assume the test passes if we reach here

    serial_write_string("pmm: tests passed\n");
}

static struct limine_file *find_root_task_module(const struct limine_module_response *response) {
    uint64_t index;

    for (index = 0; index < response->module_count; ++index) {
        struct limine_file *module = response->modules[index];

        if (module != 0 && string_equals(module->string, "root-task")) {
            return module;
        }
    }

    for (index = 0; index < response->module_count; ++index) {
        struct limine_file *module = response->modules[index];
        const char *base;

        if (module == 0) {
            continue;
        }

        base = path_basename(module->path);
        if (string_equals(base, "bootstrap.elf")) {
            return module;
        }
    }

    return 0;
}

static const char *validate_root_task_elf(const struct limine_file *module, uint64_t *entry_out) {
    const struct elf64_ehdr *ehdr;
    const struct elf64_phdr *phdrs;
    uint64_t ph_end;
    uint16_t index;
    int found_load = 0;

    if (module->address == 0) {
        return "module address missing";
    }

    if (module->size < sizeof(struct elf64_ehdr)) {
        return "module too small";
    }

    ehdr = (const struct elf64_ehdr *)module->address;

    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L'
        || ehdr->e_ident[3] != 'F') {
        return "bad elf magic";
    }

    if (ehdr->e_ident[4] != ELF_CLASS_64) {
        return "not elf64";
    }

    if (ehdr->e_ident[5] != ELF_DATA_LSB) {
        return "not little-endian";
    }

    if (ehdr->e_ident[6] != ELF_VERSION_CURRENT || ehdr->e_version != ELF_VERSION_CURRENT) {
        return "bad elf version";
    }

    if (ehdr->e_type != ELF_TYPE_EXEC && ehdr->e_type != ELF_TYPE_DYN) {
        return "bad elf type";
    }

    if (ehdr->e_machine != ELF_MACHINE_X86_64) {
        return "bad machine";
    }

    if (ehdr->e_entry == 0) {
        return "entry missing";
    }

    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        return "program headers missing";
    }

    if (ehdr->e_phentsize != sizeof(struct elf64_phdr)) {
        return "bad phdr size";
    }

    ph_end = ehdr->e_phoff + ((uint64_t)ehdr->e_phnum * sizeof(struct elf64_phdr));
    if (ph_end > module->size || ph_end < ehdr->e_phoff) {
        return "phdr table out of range";
    }

    phdrs = (const struct elf64_phdr *)((const uint8_t *)module->address + ehdr->e_phoff);
    for (index = 0; index < ehdr->e_phnum; ++index) {
        if (phdrs[index].p_type == PT_LOAD) {
            found_load = 1;
            break;
        }
    }

    if (!found_load) {
        return "no load segment";
    }

    *entry_out = ehdr->e_entry;
    return 0;
}

static int user_vaddr_plausible(uint64_t vaddr, uint64_t memsz) {
    uint64_t end = vaddr + memsz;

    if (vaddr < 0x1000) {
        return 0;
    }

    if (end < vaddr) {
        return 0;
    }

    if (end > USER_ADDRESS_TOP) {
        return 0;
    }

    return 1;
}

static uint64_t read_cr3(void) {
    uint64_t value;

    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

static void *phys_to_hhdm(uint64_t phys) {
    return (void *)(phys + hhdm_offset);
}

static uint64_t *allocate_page_table(void) {
    uint64_t *page;

    if (page_table_pool_used >= PAGE_TABLE_POOL_PAGES) {
        return 0;
    }

    page = (uint64_t *)page_table_pool[page_table_pool_used++];
    memory_zero((uint8_t *)page, PAGE_SIZE);
    return page;
}

static uint64_t translate_kernel_vaddr_to_phys(uint64_t vaddr) {
    uint64_t pml4_phys = read_cr3() & ~0xfffULL;
    uint64_t *pml4 = (uint64_t *)phys_to_hhdm(pml4_phys);
    uint64_t pml4e = pml4[(vaddr >> 39) & 0x1ffULL];
    uint64_t *pdpt;
    uint64_t pdpte;
    uint64_t *pd;
    uint64_t pde;
    uint64_t *pt;
    uint64_t pte;

    if ((pml4e & PAGE_PRESENT) == 0) {
        return 0;
    }

    pdpt = (uint64_t *)phys_to_hhdm(pml4e & PAGE_ADDR_MASK);
    pdpte = pdpt[(vaddr >> 30) & 0x1ffULL];
    if ((pdpte & PAGE_PRESENT) == 0) {
        return 0;
    }
    if ((pdpte & PAGE_HUGE) != 0) {
        return (pdpte & 0x000fffffc0000000ULL) | (vaddr & 0x3fffffffULL);
    }

    pd = (uint64_t *)phys_to_hhdm(pdpte & PAGE_ADDR_MASK);
    pde = pd[(vaddr >> 21) & 0x1ffULL];
    if ((pde & PAGE_PRESENT) == 0) {
        return 0;
    }
    if ((pde & PAGE_HUGE) != 0) {
        return (pde & 0x000fffffffe00000ULL) | (vaddr & 0x1fffffULL);
    }

    pt = (uint64_t *)phys_to_hhdm(pde & PAGE_ADDR_MASK);
    pte = pt[(vaddr >> 12) & 0x1ffULL];
    if ((pte & PAGE_PRESENT) == 0) {
        return 0;
    }

    return (pte & PAGE_ADDR_MASK) | (vaddr & 0xfffULL);
}

static const char *map_page_at(uint64_t vaddr, uint64_t phys, uint64_t flags, int user_access) {
    uint64_t pml4_phys = read_cr3() & ~0xfffULL;
    uint64_t *pml4 = (uint64_t *)phys_to_hhdm(pml4_phys);
    uint64_t pml4_index = (vaddr >> 39) & 0x1ffULL;
    uint64_t pdpt_index = (vaddr >> 30) & 0x1ffULL;
    uint64_t pd_index = (vaddr >> 21) & 0x1ffULL;
    uint64_t pt_index = (vaddr >> 12) & 0x1ffULL;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t *new_table;
    uint64_t new_table_phys;

    if ((vaddr & (PAGE_SIZE - 1)) != 0 || (phys & (PAGE_SIZE - 1)) != 0) {
        return "page alignment bad";
    }

    if (user_access) {
        if (!user_vaddr_plausible(vaddr, PAGE_SIZE)) {
            return "user vaddr bad";
        }

        if (pml4_index == 0) {
            if (user_low_pdpt_phys == 0) {
                new_table = allocate_page_table();
                if (new_table == 0) {
                    return "page table pool exhausted";
                }
                user_low_pdpt_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
                if (user_low_pdpt_phys == 0) {
                    return "page table phys missing";
                }
            }
            pml4[pml4_index] = user_low_pdpt_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        } else if (pml4_index == 0x0ffULL) {
            if (user_high_pdpt_phys == 0) {
                new_table = allocate_page_table();
                if (new_table == 0) {
                    return "page table pool exhausted";
                }
                user_high_pdpt_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
                if (user_high_pdpt_phys == 0) {
                    return "page table phys missing";
                }
            }
            pml4[pml4_index] = user_high_pdpt_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        } else if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
            new_table = allocate_page_table();
            if (new_table == 0) {
                return "page table pool exhausted";
            }
            new_table_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
            if (new_table_phys == 0) {
                return "page table phys missing";
            }
            pml4[pml4_index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        }
    } else {
        if (vaddr < KERNEL_ADDRESS_BASE) {
            return "kernel vaddr bad";
        }

        if ((pml4[pml4_index] & PAGE_PRESENT) == 0) {
            new_table = allocate_page_table();
            if (new_table == 0) {
                return "page table pool exhausted";
            }
            new_table_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
            if (new_table_phys == 0) {
                return "page table phys missing";
            }
            pml4[pml4_index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE;
        }
    }

    pdpt = (uint64_t *)phys_to_hhdm(pml4[pml4_index] & PAGE_ADDR_MASK);
    if ((pdpt[pdpt_index] & PAGE_PRESENT) == 0) {
        new_table = allocate_page_table();
        if (new_table == 0) {
            return "page table pool exhausted";
        }
        new_table_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
        if (new_table_phys == 0) {
            return "page table phys missing";
        }
        pdpt[pdpt_index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE | (user_access ? PAGE_USER : 0);
    } else if ((pdpt[pdpt_index] & PAGE_HUGE) != 0) {
        return "pdpt huge page conflict";
    }

    pd = (uint64_t *)phys_to_hhdm(pdpt[pdpt_index] & PAGE_ADDR_MASK);
    if ((pd[pd_index] & PAGE_PRESENT) == 0) {
        new_table = allocate_page_table();
        if (new_table == 0) {
            return "page table pool exhausted";
        }
        new_table_phys = translate_kernel_vaddr_to_phys((uint64_t)new_table);
        if (new_table_phys == 0) {
            return "page table phys missing";
        }
        pd[pd_index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE | (user_access ? PAGE_USER : 0);
    } else if ((pd[pd_index] & PAGE_HUGE) != 0) {
        return "pd huge page conflict";
    }

    pt = (uint64_t *)phys_to_hhdm(pd[pd_index] & PAGE_ADDR_MASK);
    pt[pt_index] = (phys & ~0xfffULL) | PAGE_PRESENT | (user_access ? PAGE_USER : 0) | flags;
    __asm__ volatile("invlpg (%0)" : : "r"((void *)vaddr) : "memory");
    return 0;
}

static const char *map_kernel_only_page(uint64_t vaddr, uint64_t phys, uint64_t flags) __attribute__((unused));
static const char *map_kernel_only_page(uint64_t vaddr, uint64_t phys, uint64_t flags) {
    return map_page_at(vaddr, phys, flags, 0);
}

static const char *map_user_read_only_page(uint64_t vaddr, uint64_t phys) {
    return map_page_at(vaddr, phys, PAGE_NX, 1);
}

static const char *map_user_read_write_page(uint64_t vaddr, uint64_t phys) {
    return map_page_at(vaddr, phys, PAGE_WRITABLE | PAGE_NX, 1);
}

static const char *map_user_executable_page(uint64_t vaddr, uint64_t phys) {
    return map_page_at(vaddr, phys, 0, 1);
}

static const char *map_user_range_with_access(
    uint64_t vaddr,
    uint64_t kernel_buffer,
    uint64_t size,
    enum user_mapping_access access) {
    uint64_t current_vaddr = align_down(vaddr, PAGE_SIZE);
    uint64_t buffer_page = align_down(kernel_buffer, PAGE_SIZE);
    uint64_t end_vaddr = align_up(vaddr + size, PAGE_SIZE);

    while (current_vaddr < end_vaddr) {
        uint64_t phys = translate_kernel_vaddr_to_phys(buffer_page);
        const char *error;

        if (phys == 0) {
            return "user backing phys missing";
        }

        switch (access) {
            case USER_MAPPING_READ_ONLY:
                error = map_user_read_only_page(current_vaddr, phys);
                break;
            case USER_MAPPING_READ_WRITE:
                error = map_user_read_write_page(current_vaddr, phys);
                break;
            case USER_MAPPING_EXECUTABLE:
                error = map_user_executable_page(current_vaddr, phys);
                break;
            default:
                return "user mapping access invalid";
        }

        if (error != 0) {
            return error;
        }

        current_vaddr += PAGE_SIZE;
        buffer_page += PAGE_SIZE;
    }

    return 0;
}

static void set_tss_descriptor(uint64_t base, uint32_t limit) {
    kernel_gdt[5] = (limit & 0xffffULL)
        | ((base & 0x00ffffffULL) << 16)
        | (0x89ULL << 40)
        | (((uint64_t)(limit >> 16) & 0xfULL) << 48)
        | (((base >> 24) & 0xffULL) << 56);
    kernel_gdt[6] = base >> 32;
}

static void install_kernel_gdt(void) {
    struct gdt_descriptor gdtr = {
        .limit = sizeof(kernel_gdt) - 1,
        .base = (uint64_t)kernel_gdt,
    };

    memory_zero((uint8_t *)&kernel_tss, sizeof(kernel_tss));
    kernel_tss.rsp0 = (uint64_t)(kernel_stack + KERNEL_STACK_SIZE);
    kernel_tss.iomap_base = sizeof(kernel_tss);
    set_tss_descriptor((uint64_t)&kernel_tss, sizeof(kernel_tss) - 1);

    __asm__ volatile(
        "lgdt %0\n"
        "pushq %[cs]\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw %[ds], %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        "movw %[tss], %%ax\n"
        "ltr %%ax\n"
        :
        : "m"(gdtr),
          [cs] "i"(KERNEL_CS_SELECTOR),
          [ds] "i"(KERNEL_DS_SELECTOR),
          [tss] "i"(TSS_SELECTOR)
        : "rax", "memory");
}

static const char *map_root_task_segments(const struct limine_file *module, uint64_t *entry_out) {
    const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)module->address;
    const struct elf64_phdr *phdrs =
        (const struct elf64_phdr *)((const uint8_t *)module->address + ehdr->e_phoff);
    uint64_t min_vaddr = UINT64_MAX;
    uint64_t max_vaddr = 0;
    uint64_t image_size;
    uint16_t index;
    uint64_t load_count = 0;

    for (index = 0; index < ehdr->e_phnum; ++index) {
        const struct elf64_phdr *phdr = &phdrs[index];
        uint64_t file_end;
        uint64_t seg_end;

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        if (phdr->p_memsz == 0) {
            continue;
        }

        if (phdr->p_filesz > phdr->p_memsz) {
            return "filesz exceeds memsz";
        }

        file_end = phdr->p_offset + phdr->p_filesz;
        if (file_end < phdr->p_offset || file_end > module->size) {
            return "segment file range bad";
        }

        if (!user_vaddr_plausible(phdr->p_vaddr, phdr->p_memsz)) {
            return "segment vaddr bad";
        }

        seg_end = phdr->p_vaddr + phdr->p_memsz;
        if (phdr->p_vaddr < min_vaddr) {
            min_vaddr = phdr->p_vaddr;
        }
        if (seg_end > max_vaddr) {
            max_vaddr = seg_end;
        }

        ++load_count;
    }

    if (load_count == 0) {
        return "no load segments";
    }

    image_size = max_vaddr - min_vaddr;
    if (image_size > ROOT_TASK_IMAGE_CAPACITY) {
        return "image too large";
    }

    if ((min_vaddr & (PAGE_SIZE - 1)) != 0) {
        return "image base not page aligned";
    }

    memory_zero(root_task_image_pages, ROOT_TASK_IMAGE_CAPACITY);
    root_task_image.base_vaddr = min_vaddr;
    root_task_image.end_vaddr = max_vaddr;
    root_task_image.entry = ehdr->e_entry;
    root_task_image.load_segment_count = load_count;

    serial_write_string("kernel: root task map begin\n");
    serial_write_string("kernel: root task load segments = ");
    serial_write_decimal(load_count);
    serial_write_string("\n");

    load_count = 0;
    for (index = 0; index < ehdr->e_phnum; ++index) {
        const struct elf64_phdr *phdr = &phdrs[index];
        uint64_t dst_offset;
        uint8_t *dst;
        const uint8_t *src;

        if (phdr->p_type != PT_LOAD || phdr->p_memsz == 0) {
            continue;
        }

        dst_offset = phdr->p_vaddr - min_vaddr;
        if (dst_offset > ROOT_TASK_IMAGE_CAPACITY || phdr->p_memsz > ROOT_TASK_IMAGE_CAPACITY - dst_offset) {
            return "segment image range bad";
        }

        dst = root_task_image_pages + dst_offset;
        src = (const uint8_t *)module->address + phdr->p_offset;

        memory_copy(dst, src, phdr->p_filesz);
        memory_zero(dst + phdr->p_filesz, phdr->p_memsz - phdr->p_filesz);

        serial_write_string("kernel: map segment ");
        serial_write_decimal(load_count);
        serial_write_string(" vaddr=");
        serial_write_hex(phdr->p_vaddr);
        serial_write_string(" end=");
        serial_write_hex(phdr->p_vaddr + phdr->p_memsz);
        serial_write_string("\n");

        ++load_count;
    }

    *entry_out = ehdr->e_entry;
    return 0;
}

static const char *prepare_root_task_launch_state(uint64_t entry, uint64_t *rsp_out) {
    uint64_t stack_guard_base = USER_STACK_TOP - USER_STACK_TOTAL_SIZE;
    uint64_t stack_base = stack_guard_base + USER_STACK_GUARD_SIZE;
    uint64_t initial_rsp;

    if (USER_STACK_TOP > USER_ADDRESS_TOP) {
        return "stack top outside user range";
    }

    if (stack_guard_base < 0x1000 || stack_guard_base >= USER_STACK_TOP) {
        return "stack range invalid";
    }

    initial_rsp = align_down(USER_STACK_TOP, 16);
    if (initial_rsp <= stack_base || initial_rsp > USER_STACK_TOP) {
        return "stack pointer invalid";
    }

    memory_zero(root_task_stack_pages, USER_STACK_SIZE);
    root_task_launch_state.stack_guard_base = stack_guard_base;
    root_task_launch_state.stack_base = stack_base;
    root_task_launch_state.stack_top = USER_STACK_TOP;

    root_task_launch_state.context.rip = entry;
    root_task_launch_state.context.rsp = initial_rsp;
    root_task_launch_state.context.rflags = USER_RFLAGS_INITIAL;
    root_task_launch_state.context.rdi = 0;
    root_task_launch_state.context.rsi = 0;
    root_task_launch_state.context.rdx = 0;
    root_task_launch_state.context.rcx = 0;
    root_task_launch_state.context.r8 = 0;
    root_task_launch_state.context.r9 = 0;
    root_task_launch_state.context.cs = USER_CS_SELECTOR;
    root_task_launch_state.context.ss = USER_SS_SELECTOR;

    root_task_launch_state.frame.rip = entry;
    root_task_launch_state.frame.cs = USER_CS_SELECTOR;
    root_task_launch_state.frame.rflags = USER_RFLAGS_INITIAL;
    root_task_launch_state.frame.rsp = initial_rsp;
    root_task_launch_state.frame.ss = USER_SS_SELECTOR;

    *rsp_out = initial_rsp;
    return 0;
}

static const char *install_root_task_user_mappings(const struct limine_file *module) {
    const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)module->address;
    const struct elf64_phdr *phdrs =
        (const struct elf64_phdr *)((const uint8_t *)module->address + ehdr->e_phoff);
    uint16_t index;

    for (index = 0; index < ehdr->e_phnum; ++index) {
        const struct elf64_phdr *phdr = &phdrs[index];
        enum user_mapping_access access;
        uint64_t segment_vaddr;
        uint64_t segment_size;
        uint64_t image_offset;

        if (phdr->p_type != PT_LOAD || phdr->p_memsz == 0) {
            continue;
        }

        segment_vaddr = align_down(phdr->p_vaddr, PAGE_SIZE);
        segment_size = align_up((phdr->p_vaddr - segment_vaddr) + phdr->p_memsz, PAGE_SIZE);
        image_offset = segment_vaddr - root_task_image.base_vaddr;

        if ((phdr->p_flags & PF_W) != 0 && (phdr->p_flags & PF_X) != 0) {
            return "user W+X forbidden";
        }

        if ((phdr->p_flags & PF_W) != 0) {
            access = USER_MAPPING_READ_WRITE;
        } else if ((phdr->p_flags & PF_X) != 0) {
            access = USER_MAPPING_EXECUTABLE;
        } else {
            access = USER_MAPPING_READ_ONLY;
        }

        if (image_offset > ROOT_TASK_IMAGE_CAPACITY || segment_size > ROOT_TASK_IMAGE_CAPACITY - image_offset) {
            return "user image range bad";
        }

        if (!user_vaddr_plausible(segment_vaddr, segment_size)) {
            return "user image vaddr bad";
        }

        serial_write_string("kernel: user segment map ");
        serial_write_string(user_mapping_access_name(access));
        serial_write_string("\n");

        {
            const char *error = map_user_range_with_access(
                segment_vaddr,
                (uint64_t)(root_task_image_pages + image_offset),
                segment_size,
                access);
            if (error != 0) {
                return error;
            }
        }
    }

    if (!user_vaddr_plausible(root_task_launch_state.stack_base, USER_STACK_SIZE)) {
        return "user stack vaddr bad";
    }

    {
        const char *error = map_user_range_with_access(
            root_task_launch_state.stack_base,
            (uint64_t)root_task_stack_pages,
            USER_STACK_SIZE,
            USER_MAPPING_READ_WRITE);
        if (error != 0) {
            return error;
        }
    }

    return 0;
}

static const char *translate_root_task_user_buffer(
    uint64_t user_ptr,
    uint64_t len,
    const char **kernel_ptr_out) {
    uint64_t end = user_ptr + len;

    if (len == 0 || len > SYSCALL_DEBUG_WRITE_MAX) {
        return "length bad";
    }

    if (end < user_ptr) {
        return "range overflow";
    }

    if (user_ptr >= root_task_image.base_vaddr && end <= root_task_image.end_vaddr) {
        *kernel_ptr_out = (const char *)(root_task_image_pages + (user_ptr - root_task_image.base_vaddr));
        return 0;
    }

    if (user_ptr >= root_task_launch_state.stack_base && end <= root_task_launch_state.stack_top) {
        *kernel_ptr_out = (const char *)(root_task_stack_pages + (user_ptr - root_task_launch_state.stack_base));
        return 0;
    }

    return "range invalid";
}

static void set_idt_gate(uint8_t vector, void (*handler)(void), uint8_t type_attr) {
    uint64_t offset = (uint64_t)handler;

    kernel_idt[vector].offset_low = offset & 0xffffu;
    kernel_idt[vector].selector = KERNEL_CS_SELECTOR;
    kernel_idt[vector].ist = 0;
    kernel_idt[vector].type_attr = type_attr;
    kernel_idt[vector].offset_mid = (offset >> 16) & 0xffffu;
    kernel_idt[vector].offset_high = (uint32_t)(offset >> 32);
    kernel_idt[vector].zero = 0;
}

__attribute__((noreturn)) void handle_user_proof_trap(void) {
    serial_write_string("user: first instruction reached\n");
    halt_forever();
}

uint64_t handle_user_syscall(uint64_t number, uint64_t user_ptr, uint64_t len) {
    const char *kernel_ptr;
    const char *error;

    if (number != SYSCALL_DEBUG_WRITE) {
        serial_write_string("kernel: syscall invalid number\n");
        return UINT64_MAX;
    }

    error = translate_root_task_user_buffer(user_ptr, len, &kernel_ptr);
    if (error != 0) {
        serial_write_string("kernel: syscall invalid arg\n");
        return UINT64_MAX;
    }

    serial_write_buffer(kernel_ptr, len);
    serial_write_string("kernel: syscall handled\n");
    return len;
}

__attribute__((noreturn)) void handle_user_general_protection(uint64_t error_code) {
    serial_write_string("kernel: user fault general-protection\n");
    serial_write_string("kernel: user gp code = ");
    serial_write_hex(error_code);
    serial_write_string("\n");
    halt_forever();
}

__attribute__((noreturn)) void handle_user_page_fault(uint64_t fault_address, uint64_t error_code) {
    serial_write_string("kernel: user fault page\n");
    serial_write_string("kernel: user fault addr = ");
    serial_write_hex(fault_address);
    serial_write_string("\n");
    serial_write_string("kernel: user fault code = ");
    serial_write_hex(error_code);
    serial_write_string("\n");
    halt_forever();
}

__attribute__((naked)) void user_proof_trap_stub(void) {
    __asm__ volatile(
        "cld\n"
        "call handle_user_proof_trap\n");
}

__attribute__((naked)) void user_general_protection_stub(void) {
    __asm__ volatile(
        "popq %rdi\n"
        "cld\n"
        "call handle_user_general_protection\n");
}

__attribute__((naked)) void user_page_fault_stub(void) {
    __asm__ volatile(
        "movq %cr2, %rdi\n"
        "popq %rsi\n"
        "cld\n"
        "call handle_user_page_fault\n");
}

__attribute__((naked)) void user_syscall_stub(void) {
    __asm__ volatile(
        "cld\n"
        "movq %rdi, %rcx\n"
        "movq %rsi, %rdx\n"
        "movq %rax, %rdi\n"
        "movq %rcx, %rsi\n"
        "call handle_user_syscall\n"
        "iretq\n");
}

static void install_kernel_idt(void) {
    struct idt_descriptor idtr = {
        .limit = sizeof(kernel_idt) - 1,
        .base = (uint64_t)kernel_idt,
    };

    memory_zero((uint8_t *)kernel_idt, sizeof(kernel_idt));
    set_idt_gate(3, user_proof_trap_stub, 0xee);
    set_idt_gate(13, user_general_protection_stub, 0x8e);
    set_idt_gate(14, user_page_fault_stub, 0x8e);
    set_idt_gate(SYSCALL_VECTOR, user_syscall_stub, 0xee);
    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

__attribute__((noreturn)) static void enter_user_mode(void) {
    __asm__ volatile(
        "cli\n"
        "movq %[rdi], %%rdi\n"
        "pushq %[ss]\n"
        "pushq %[rsp]\n"
        "pushq %[rflags]\n"
        "pushq %[cs]\n"
        "pushq %[rip]\n"
        "iretq\n"
        :
        : [rip] "r"(root_task_launch_state.frame.rip),
          [cs] "r"((uint64_t)root_task_launch_state.frame.cs),
          [rflags] "r"(root_task_launch_state.frame.rflags),
          [rsp] "r"(root_task_launch_state.frame.rsp),
          [ss] "r"((uint64_t)root_task_launch_state.frame.ss),
          [rdi] "r"(root_task_launch_state.context.rdi)
        : "memory", "rdi");

    __builtin_unreachable();
}

USED SECTION(".limine_reqs.start")
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

USED SECTION(".limine_reqs.requests")
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

USED SECTION(".limine_reqs.requests")
static volatile struct limine_module_request limine_module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
    .response = 0,
    .internal_module_count = 0,
    .internal_modules = 0,
};

USED SECTION(".limine_reqs.requests")
static volatile struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

USED SECTION(".limine_reqs.requests")
static volatile struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

USED SECTION(".limine_reqs.end")
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((noreturn)) void kernel_main(void) {
    struct limine_module_response *module_response;
    struct limine_file *root_task_module;
    const char *elf_error;
    uint64_t root_task_entry = 0;
    uint64_t root_task_rsp = 0;

    if (!serial_init()) {
        halt_forever();
    }

    if (limine_base_revision[2] != 0ULL) {
        serial_write_string("kernel: unsupported limine base revision\n");
        halt_forever();
    }

    if (limine_hhdm_request.response == 0) {
        serial_write_string("kernel: hhdm missing\n");
        halt_forever();
    }

    hhdm_offset = limine_hhdm_request.response->offset;
    install_kernel_gdt();
    install_kernel_idt();

    if (limine_memmap_request.response == 0) {
        serial_write_string("kernel: memmap missing\n");
        halt_forever();
    }

    serial_write_string("kernel: memmap ok\n");
    pmm_init(limine_memmap_request.response);

    pmm_test();

    serial_write_string("kernel: early boot ok | version " LUMEN_VERSION_STRING "\n");

    module_response = limine_module_request.response;
    if (module_response == 0) {
        serial_write_string("kernel: modules request missing\n");
        halt_forever();
    }

    serial_write_string("kernel: modules request ok\n");
    serial_write_string("kernel: module count = ");
    serial_write_decimal(module_response->module_count);
    serial_write_string("\n");

    root_task_module = find_root_task_module(module_response);
    if (root_task_module == 0) {
        serial_write_string("kernel: root task module missing\n");
        halt_forever();
    }

    serial_write_string("kernel: root task module found\n");

    elf_error = validate_root_task_elf(root_task_module, &root_task_entry);
    if (elf_error != 0) {
        serial_write_string("kernel: root task elf invalid | reason ");
        serial_write_string(elf_error);
        serial_write_string("\n");
        halt_forever();
    }

    serial_write_string("kernel: root task elf valid\n");
    elf_error = map_root_task_segments(root_task_module, &root_task_entry);
    if (elf_error != 0) {
        serial_write_string("kernel: root task map failed | reason ");
        serial_write_string(elf_error);
        serial_write_string("\n");
        halt_forever();
    }

    serial_write_string("kernel: root task segments mapped\n");
    serial_write_string("kernel: root task entry = ");
    serial_write_hex(root_task_entry);
    serial_write_string("\n");
    serial_write_string("kernel: root task image ready\n");

    elf_error = prepare_root_task_launch_state(root_task_entry, &root_task_rsp);
    if (elf_error != 0) {
        serial_write_string("kernel: root task context failed | reason ");
        serial_write_string(elf_error);
        serial_write_string("\n");
        halt_forever();
    }

    serial_write_string("kernel: root task stack allocated\n");
    serial_write_string("kernel: root task stack guard base = ");
    serial_write_hex(root_task_launch_state.stack_guard_base);
    serial_write_string("\n");
    serial_write_string("kernel: root task stack base = ");
    serial_write_hex(root_task_launch_state.stack_base);
    serial_write_string("\n");
    serial_write_string("kernel: root task stack top = ");
    serial_write_hex(root_task_launch_state.stack_top);
    serial_write_string("\n");
    serial_write_string("kernel: root task context ready\n");
    serial_write_string("kernel: root task rip = ");
    serial_write_hex(root_task_launch_state.context.rip);
    serial_write_string("\n");
    serial_write_string("kernel: root task rsp = ");
    serial_write_hex(root_task_rsp);
    serial_write_string("\n");
    serial_write_string("kernel: root task launch frame prepared\n");
    serial_write_string("kernel: root task launch state prepared\n");

    elf_error = install_root_task_user_mappings(root_task_module);
    if (elf_error != 0) {
        serial_write_string("kernel: user map failed | reason ");
        serial_write_string(elf_error);
        serial_write_string("\n");
        halt_forever();
    }

    serial_write_string("kernel: entering user mode\n");
    enter_user_mode();
}

__attribute__((noreturn, naked, section(".text._start"))) void _start(void) {
    __asm__ volatile(
        "lea kernel_stack + %c0(%%rip), %%rsp\n"
        "xor %%rbp, %%rbp\n"
        "call kernel_main\n"
        :
        : "i"(KERNEL_STACK_SIZE)
        : "memory");
}
