#include <stdio.h>

int main(int argc, char** argv) {
    if(argc == 1) return 0;

    if(argc >= 3) {
        for(int i = 1; i < argc - 1; i++) {
            printf("%s ", argv[i]);
        }
    }
    printf("%s\n", argv[argc - 1]);
    return 0;
}
