#include <kproc.h>
#include <filesystem.h>
#include <video.h>
#include <sys/syscall.h>
#include <sys/files.h>

static uint8_t buffer[513];

void kproc_stdout() {
    while(true) {
        // probly good enough for now
        syscall_sleep(10);
        int size = vfs_read(SYSFILE_FD_STDOUT, buffer, 512);
        if(size > 0) {
            buffer[size] = 0;
            video_prints((char*)buffer, -1, -1, -1, true);
        } else {
            syscall_sleep(90);
        }
    }
}
