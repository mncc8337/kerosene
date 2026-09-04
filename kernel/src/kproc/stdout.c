#include <kproc.h>
#include <filesystem.h>
#include <video.h>
#include <sys/syscall.h>
#include <sys/files.h>

static uint8_t buffer[512];

void kproc_stdout() {
    int stdout_fd = syscall_open(SYSFILE_PATH_STDOUT, FILE_OPEN_READ);
    if(stdout_fd < 0) {
        video_prints("failed to open stdout\n", -1, -1, -1, true);
        syscall_kill(-1);
    }

    while(true) {
        int size = syscall_read(stdout_fd, buffer, 511);
        if(size > 0) {
            buffer[size] = 0;
            video_prints((char*)buffer, -1, -1, -1, true);
        }
    }
}
