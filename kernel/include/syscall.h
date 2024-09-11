#pragma once

#define MAX_SYSCALL 4

#define SYSCALL_TEST              0
#define SYSCALL_PUTCHAR           1
#define SYSCALL_TIME              2
#define SYSCALL_PROCESS_TERMINATE 3

#define SYSCALL_0P(id, ret) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id))

#define SYSCALL_1P(id, ret, p1) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1))

#define SYSCALL_2P(id, ret, p1, p2) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2))

#define SYSCALL_3P(id, ret, p1, p2, p3) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3))

#define SYSCALL_4P(id, ret, p1, p2, p3, p4) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3), "s" (p4))

#define SYSCALL_5P(id, ret, p1, p2, p3, p4, p5) \
asm volatile("int $0x80" : "=a" (ret) : "0" (id), "b" (p1), "c" (p2), "d" (p3), "s" (p4), "d" (p5))

void syscall_init();
