#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/filesystem.h>

enum {
    SYSCALL_TIME,
    SYSCALL_KILL_PROCESS,
    SYSCALL_SLEEP,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_READ,
    SYSCALL_WRITE,
    SYSCALL_SEEK,
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

uint64_t syscall_time();
void syscall_kill_process(int exit_code);
void syscall_sleep(unsigned ticks);
int syscall_open(const char* path, const file_mode_t mode);
void syscall_close(int file_descriptor);
int syscall_read(int file_descriptor, uint8_t* buffer, size_t size);
int syscall_write(int file_descriptor, const uint8_t* buffer, size_t size);
void syscall_seek(int file_descriptor, int64_t seek_position, int whence, int64_t* final_position);
