#include <stdio.h>
#include <time.h>

#include <syscall.h>

int buffer[512];
char sss[] = "ngu1";

int main() {
    printf("hi user eeee!\n");

    puts("buffer manipulation test");
    for(int i = 0; i < 512; i++)
        buffer[i] = i;
    puts("buffer print");
    for(int i = 0; i < 512; i++)
        printf("%d ", buffer[i]);
    putchar('\n');

    printf("print a string from memory: %s\n", sss);

    printf("current time: %d\n", time(NULL));

    int ret;
    SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
    return 0;
}
