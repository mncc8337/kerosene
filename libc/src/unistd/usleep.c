#include <unistd.h>
#include <sys/syscall.h>

unsigned sleep_ms(unsigned microseconds) {
    syscall_sleep(microseconds * TIMER_FREQUENCY / 1000000);
    // TODO:
    // return real unslept microseconds
    return 0;
}
