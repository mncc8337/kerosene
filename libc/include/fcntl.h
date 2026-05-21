#pragma once

#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002

#define O_ACCMODE   00000003

#define O_CREAT     00000100
#define O_EXCL      00000200
#define O_TRUNC     00001000
#define O_DIRECTORY 00200000

#define O_APPEND    00002000
#define O_NONBLOCK  00004000
#define O_SYNC      00010000

int open(const char* pathname, int flags, ...);
