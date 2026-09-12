#include <sys/syscall.h>

int syscall_spawn(const char* path, bool is_user, unsigned argc, char* args, bool attach, const char* stdin_path, const char* stdout_path) {
    int ret;
    syscall_spawn_args_t _args = {
        path,
        is_user,
        argc,
        args,
        attach,
        stdin_path,
        stdout_path
    };
    SYSCALL_1P(SYSCALL_SPAWN, ret, &_args);
    return ret;
}
