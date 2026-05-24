#pragma once

#include <stdint.h>
#include <stddef.h>
#include <syscall.h>

#include <time.h>

time_t syscall_time(time_t* timer);
void syscall_kill_process();
void syscall_sleep(unsigned ticks);
int syscall_open(const char* path, const char* modestr);
void syscall_close(int file_descriptor);
int syscall_read(int file_descriptor, uint8_t* buffer, size_t size);
int syscall_write(int file_descriptor, const uint8_t* buffer, size_t size);
void syscall_seek(int file_descriptor, int64_t seek_position, int whence, int64_t* final_position);
