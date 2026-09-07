#include <stdio.h>

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

    return 0;
}
