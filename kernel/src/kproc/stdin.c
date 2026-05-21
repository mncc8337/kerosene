#include <kproc.h>
#include <filesystem.h>
#include <video.h>
#include <kbd.h>
#include <sysfiles.h>

static key_t key;

void kproc_stdin() {
    while(true) {
        // it already wait for key event so no need to sleep
        kbd_wait_key(&key);

        if(!key.released) {
            video_printc(key.mapped, -1, -1, -1, true);
            vfs_write(SYSFILE_FD_STDIN, &key.mapped, 1);
        }
    }
}

