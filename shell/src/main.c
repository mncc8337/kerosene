#include <stdlib.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char buffer[512];
unsigned buffer_ptr = 0;
int exit_code = 0;

bool process_prompt() {
    char* prog = strtok(buffer, " ");
    int argc = 1;

    if(!strcmp(prog, "exit")) {
        char* exit_str = strtok(NULL, " ");
        if(exit_str) exit_code = atoi(exit_str);
        return true;
    }

    while(strtok(NULL, " ")) argc++;

    int fd = syscall_open(prog, FILE_OPEN_READ);
    if(fd >= 0) {
        syscall_close(fd);
        int ret = syscall_spawn(prog, true, argc, buffer, true, NULL, NULL);
        printf("'%s' exited with code %d\n", prog, ret);
    } else {
        printf("failed to open '%s'. file not found?\n", prog);
    }

    return false;
}

int main(int argc, char** argv) {
    puts("welcome to keroshell!");
    printf("build datetime: %s, %s\n", __TIME__, __DATE__);
    if(argc > 1) {
        printf("run with args: ");
        for(int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        putchar('\n');
    }

    printf(">>> ");

    char chr;
    while(true) {
        scanf("%c", &chr);
        putchar(chr);
        if(chr == '\n') {
            if(buffer_ptr > 0) {
                buffer[buffer_ptr] = 0;
                bool should_exit = process_prompt();
                buffer_ptr = 0;
                if(should_exit) return exit_code;
            }
            printf(">>> ");
        } else if(chr == '\b') {
            if(buffer_ptr > 0) buffer_ptr--;
        } else if(isprint(chr)) {
            buffer[buffer_ptr++] = chr;
        }
    }
}
