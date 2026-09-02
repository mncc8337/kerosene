#include <sys/syscall.h>

int syscall_mount(const char* target_path, const char* mount_path) {
    int ret;
    SYSCALL_2P(SYSCALL_MOUNT, ret, target_path, mount_path);
    return ret;
}