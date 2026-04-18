__attribute__((noreturn, naked)) void _start(void) {
    __asm__ volatile(
        "int3\n"
        "1:\n"
        "pause\n"
        "jmp 1b\n");
}
