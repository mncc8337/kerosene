#include <kproc.h>
#include <filesystem.h>
#include <video.h>
#include <sys/syscall.h>
#include <sys/files.h>

static uint8_t buffer[512];

void kproc_stdout() {
    while(true) {
        int size = syscall_read(SYSFILE_FD_STDOUT, buffer, 511);
        if(size > 0) {
            buffer[size] = 0;
            video_prints((char*)buffer, -1, -1, -1, true);
        }
    }
}
