#include <stdlib.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char buffer[4096];
unsigned buffer_ptr = 0;
int exit_code = 0;

bool process_prompt() {
    char* prog = strtok(buffer, " ");

    if(!strcmp(prog, "exit")) {
        char* exit_str = strtok(NULL, " ");
        if(exit_str) exit_code = atoi(exit_str);
        return true;
    }

    // replace all ' ' with '\0'
    while(strtok(NULL, " "));

    int fd = syscall_open(prog, FILE_OPEN_READ);
    if(fd >= 0) {
        syscall_close(fd);
        extern char** environment_pointer;
        int ret = syscall_spawn(
            prog,
            true,
            buffer,
            environment_pointer[0],
            true,
            NULL,
            NULL
        );
        printf("'%s' exited with code %d\n", prog, ret);
    } else {
        printf("failed to open '%s'. file not found?\n", prog);
    }

    return false;
}

int main(int argc, char** argv, char** envp) {
    puts("welcome to keroshell!");
    printf("build datetime: %s, %s\n", __TIME__, __DATE__);
    if(argc > 1) {
        printf("run with args: ");
        for(int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        putchar('\n');
    }

    puts("environment:");
    for(int envc = 0; envp[envc] != NULL; envc++) {
        printf("    %s\n", envp[envc]);
    }

    puts("try `/bin/echo $PING`");

    printf(">>> ");

    char chr;
    while(true) {
        scanf("%c", &chr);
        if(chr == '\n') {
            putchar('\n');
            if(buffer_ptr > 0) {
                buffer[buffer_ptr] = 0;
                bool should_exit = process_prompt();
                buffer_ptr = 0;
                if(should_exit) return exit_code;
            }
            printf(">>> ");
        } else if(chr == '\b') {
            if(buffer_ptr > 0) {
                buffer_ptr--;
                putchar(chr);
            }
        } else if(isprint(chr)) {
            buffer[buffer_ptr++] = chr;
            putchar(chr);
        }
    }
}
