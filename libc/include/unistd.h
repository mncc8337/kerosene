#pragma once

#include <sys/files.h>
#include <stddef.h>
#include <sys/types.h>
#include <bits/seek.h>

// standard file descriptor numbers
#define STDIN_FILENO  SYSFILE_FD_STDIN
#define STDOUT_FILENO SYSFILE_FD_STDOUT

void close(int file_descriptor);
int read(int file_descriptor, void* buffer, size_t size);
int write(int file_descriptor, const void* buffer, size_t size);
off_t lseek(int file_descriptor, off_t offset, int whence);

unsigned int usleep(unsigned microseconds);
unsigned int sleep(unsigned seconds);
