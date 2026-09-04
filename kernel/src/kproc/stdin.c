#include <kproc.h>
#include <filesystem.h>
#include <video.h>
#include <kbd.h>
#include <sys/syscall.h>
#include <sys/files.h>

static key_t key;

void kproc_stdin() {
    int stdin_fd = syscall_open(SYSFILE_PATH_STDIN, FILE_OPEN_WRITE | FILE_OPEN_APPEND);
    if(stdin_fd < 0) {
        video_prints("failed to open stdin\n", -1, -1, -1, true);
        syscall_kill(-1);
    }

    while(true) {
        // it already wait for key event so no need to sleep
        kbd_wait_key(&key);

        if(!key.released) {
            syscall_write(stdin_fd, &key.mapped, 1);
        }
    }
}

