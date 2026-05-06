#include <kproc.h>
#include <filesystem.h>
#include <syscall.h>
#include <video.h>

static uint8_t buffer[513];
static int ret;

void kproc_stdout() {
    while(true) {
        // probly good enough for now
        SYSCALL_1P(SYSCALL_SLEEP, ret, 10);
        size_t size = vfs_read(0, buffer, 512);
        if(size > 0) {
            buffer[size] = 0;
            video_prints((char*)buffer, -1, -1, -1, true);
        } else {
            SYSCALL_1P(SYSCALL_SLEEP, ret, 90);
        }
    }
}
