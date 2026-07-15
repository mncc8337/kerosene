#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/filesystem.h>

static file_mode_t flags_converter(int flags) {
    file_mode_t out = 0;

    int accmode = flags & O_ACCMODE;
    if(accmode == O_RDONLY) {
        out = FILE_OPEN_READ;
    } else if(accmode == O_WRONLY) {
        out = FILE_OPEN_WRITE;
    } else if(accmode == O_RDWR) {
        out = FILE_OPEN_READ | FILE_OPEN_WRITE;
    }

    if(flags & O_TRUNC) {
        out |= FILE_OPEN_TRUNCATE;
    }
    if(flags & O_CREAT) {
        out |= FILE_OPEN_CREATE;
    }
    if(flags & O_EXCL) {
        out |= FILE_OPEN_EXCLUSIVE;
    }
    if(flags & O_DIRECTORY) {
        out |= FILE_OPEN_ONLYDIR;
    }
    if(flags & O_APPEND) {
        out |= FILE_OPEN_APPEND;
    }

    // TODO:
    // add conversion to the rest

    return out;
}

int open(const char* path, int flags, ...) {
    file_mode_t mode = flags_converter(flags);

    if(flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        
        // TODO: pass permission
        (void)mode;
    }

    int fd = syscall_open(path, mode);

    if(fd < 0) {
        return -1;
    }

    return fd;
}
