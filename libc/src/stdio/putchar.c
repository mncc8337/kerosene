#include <system.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int putchar(int ic) {
    syscall_write(STDOUT_FILENO, (uint8_t*)&ic, 1);
    return ic;
}
