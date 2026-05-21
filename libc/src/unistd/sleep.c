#include <unistd.h>
#include <sys/syscall.h>

unsigned sleep(unsigned seconds) {
    syscall_sleep(seconds * TIMER_FREQUENCY);
    // TODO:
    // return real unslept seconds
    return 0;
}
