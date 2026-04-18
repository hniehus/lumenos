const char hello_message[] = "user: hello lumen\n";

__attribute__((noreturn, naked)) void _start(void) {
    __asm__ volatile(
        "lea hello_message(%rip), %rdi\n"
        "mov $18, %rsi\n"
        "mov $1, %rax\n"
        "int $0x80\n"
        "1:\n"
        "pause\n"
        "jmp 1b\n");
}
