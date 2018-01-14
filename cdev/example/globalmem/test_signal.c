#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static void signal_io_handler(int signum)
{
    printf("receive a signal from globalmem, signum: %d\n", signum);
}

int main(int argc, char const *argv[])
{
    int fd = -1;
    int oflags = 0;

    do {
        if (-1 == (fd = open("/dev/globalmem", O_RDWR, S_IRUSR | S_IWUSR))) {
            break;
        }
        signal(SIGIO, signal_io_handler);
        fcntl(fd, F_SETOWN, getpid());
        oflags = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, oflags | FASYNC);
        for (;;) {
            sleep(100);
        }
    } while (0);
    return 0;
}
