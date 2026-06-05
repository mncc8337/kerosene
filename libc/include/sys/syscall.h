#pragma once

#include <stdint.h>
#include <stddef.h>
#include <syscall.h>

uint64_t syscall_time();
void syscall_kill_process(int exit_code);
void syscall_sleep(unsigned ticks);
int syscall_open(const char* path, const char* modestr);
void syscall_close(int file_descriptor);
int syscall_read(int file_descriptor, uint8_t* buffer, size_t size);
int syscall_write(int file_descriptor, const uint8_t* buffer, size_t size);
void syscall_seek(int file_descriptor, int64_t seek_position, int whence, int64_t* final_position);
