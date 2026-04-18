#include <stdint.h>

#include "version.h"
#include "limine.h"

#define COM1 0x3f8
#define USED __attribute__((used))
#define SECTION(name) __attribute__((section(name)))

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

USED SECTION(".limine_reqs.start")
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

USED SECTION(".limine_reqs.requests")
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

USED SECTION(".limine_reqs.end")
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((noreturn)) void _start(void) {
    if (!serial_init()) {
        halt_forever();
    }

    if (limine_base_revision[2] != 0ULL) {
        serial_write_string("kernel: unsupported limine base revision\n");
        halt_forever();
    }

    serial_write_string("kernel: early boot ok | version " LUMEN_VERSION_STRING "\n");
    halt_forever();
}
