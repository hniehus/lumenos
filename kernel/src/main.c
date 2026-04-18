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

USED SECTION(".limine_reqs.end")
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((noreturn)) void _start(void) {
    struct limine_module_response *module_response;
    struct limine_file *root_task_module;
    const char *elf_error;
    uint64_t root_task_entry = 0;

    if (!serial_init()) {
        halt_forever();
    }

    if (limine_base_revision[2] != 0ULL) {
        serial_write_string("kernel: unsupported limine base revision\n");
        halt_forever();
    }

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
    serial_write_string("kernel: root task entry = ");
    serial_write_hex(root_task_entry);
    serial_write_string("\n");
    halt_forever();
}
