#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>

int open(const char* path, int flags, ...) {
    const char* modestr;
    
    int accmode = flags & O_ACCMODE;

    if(accmode == O_RDONLY) {
        modestr = "r";
    } else if(accmode == O_WRONLY) {
        if(flags & O_APPEND) {
            modestr = "a";
        } else {
            modestr = "w";
        }
    } else if(accmode == O_RDWR) {
        if(flags & O_APPEND) {
            modestr = "a+";
        } else if(flags & O_TRUNC) {
            modestr = "w+";
        } else {
            modestr = "r+";
        }
    } 
    else {
        return -1;
    }

    if(flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        
        // TODO:
        // pass permission
        (void)mode;
    }

    int fd = syscall_open(path, modestr);

    if(fd < 0) {
        return -1;
    }

    return fd;
}
