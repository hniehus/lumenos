OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(0x5);
    rodata PT_LOAD FLAGS(0x4);
    data PT_LOAD FLAGS(0x6);
}

SECTIONS
{
    KERNEL_VMA = 0xffffffff80000000;
    . = KERNEL_VMA;

    .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_VMA) {
        KEEP(*(.text._start))
        *(.text .text.*)
    } :text

    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata .rodata.*)
    } :rodata

    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VMA) {
        KEEP(*(.limine_reqs.start))
        KEEP(*(.limine_reqs.requests))
        KEEP(*(.limine_reqs.end))
        *(.data .data.*)
    } :data

    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.comment)
    }
}
