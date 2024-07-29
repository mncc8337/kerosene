#include "kpanic.h"

void kpanic() {
    asm volatile("cli");
    while(1) asm volatile("hlt");
}
