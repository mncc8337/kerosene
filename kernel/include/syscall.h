#pragma once

enum {
    SYSCALL_PUTCHAR,
    SYSCALL_TIME,
    SYSCALL_KILL_PROCESS,
    SYSCALL_SLEEP,
    MAX_SYSCALL
};

#define SYSCALL_0P(id, ret) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id))

#define SYSCALL_1P(id, ret, p1) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1))

#define SYSCALL_2P(id, ret, p1, p2) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2))

#define SYSCALL_3P(id, ret, p1, p2, p3) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3))

#define SYSCALL_4P(id, ret, p1, p2, p3, p4) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3), "S" (p4))

#define SYSCALL_5P(id, ret, p1, p2, p3, p4, p5) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5))

void syscall_init();
