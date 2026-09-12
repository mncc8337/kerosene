#include <stdlib.h>
#include <string.h>

extern char **environment_pointer;

const char *getenv(const char *name) {
    if(environment_pointer == NULL || name == NULL) {
        return NULL;
    }
    size_t name_len = strlen(name);

    for(int i = 0; environment_pointer[i] != NULL; i++) {
        if(strncmp(environment_pointer[i], name, name_len) == 0) {
            if(environment_pointer[i][name_len] == '=') {
                return &environment_pointer[i][name_len + 1];
            }
        }
    }
    return NULL;
}
