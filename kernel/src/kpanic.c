#include "kpanic.h"
#include "stdio.h"

#include "stdint.h"

#define MAX_FRAMES 30

struct stackframe {
  struct stackframe* ebp;
  uint32_t eip;
};

char function_name[512];

static char* find_function_name(unsigned frame) {
    return function_name;
}

static void stack_trace() {
    struct stackframe *stk;
    asm ("movl %%ebp, %0" : "=r"(stk) ::);
    puts("stack trace:");
    for(unsigned frame = 0; stk && frame < MAX_FRAMES; frame++) {
        printf("0x%x (", stk->eip);
        printf("%s", find_function_name(frame));
        puts(")");
        stk = stk->ebp;
    }
}

void kpanic() {
    stack_trace();
    puts("system halted!");
    asm volatile("cli");
    while(1) asm volatile("hlt");
}
