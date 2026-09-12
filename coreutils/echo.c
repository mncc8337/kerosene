#include <stdio.h>
#include <stdlib.h>

void print_token(char** argv, int idx) {
    if(argv[idx][0] != '$') {
        printf("%s", argv[idx]);
    } else {
        printf("%s", getenv(argv[idx] + 1));
    }
}

int main(int argc, char** argv) {
    if(argc == 1) return 0;

    if(argc >= 3) {
        for(int i = 1; i < argc - 1; i++) {
            print_token(argv, i);
            putchar(' ');
        }
    }

    print_token(argv, argc - 1);
    putchar('\n');
    return 0;
}
