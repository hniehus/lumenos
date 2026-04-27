const char hello_message[] = "user: hello lumen\n";

#define USER_STACK_TOP 0x00007fffffff0000ULL
#define USER_STACK_SIZE (64 * 1024ULL)
#define USER_STACK_GUARD_SIZE 0x1000ULL
#define USER_STACK_BASE (USER_STACK_TOP - USER_STACK_SIZE)
#define USER_STACK_GUARD_BASE (USER_STACK_BASE - USER_STACK_GUARD_SIZE)
#define KERNEL_ADDRESS_BASE 0xffffffff80000000ULL
#if defined(ROOT_TASK_FAULT_GUARD_PAGE)
#define FAULT_ADDRESS (USER_STACK_GUARD_BASE + 0x10ULL)
#else
#define FAULT_ADDRESS KERNEL_ADDRESS_BASE
#endif

__attribute__((noreturn, naked)) void _start(void) {
    __asm__ volatile(
        "lea hello_message(%%rip), %%rdi\n"
        "mov $18, %%rsi\n"
        "mov $1, %%rax\n"
        "int $0x80\n"
        "movabs $%c0, %%rdi\n"
        "movb $0x41, (%%rdi)\n"
        "1:\n"
        "pause\n"
        "jmp 1b\n"
        :
        : "i"(FAULT_ADDRESS)
        : "rdi", "memory");
}
