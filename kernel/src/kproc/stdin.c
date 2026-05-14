#include <kproc.h>
#include <filesystem.h>
#include <syscall.h>
#include <video.h>
#include <kbd.h>

static key_t key;

void kproc_stdin() {
    while(true) {
        // it already wait for key event so no need to sleep
        kbd_wait_key(&key);

        if(!key.released) {
            video_printc(key.mapped, -1, -1, -1, true);
            vfs_write(0, &key.mapped, 1);
        }
    }
}

