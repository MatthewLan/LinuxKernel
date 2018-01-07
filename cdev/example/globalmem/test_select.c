#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>


#define GLOBALMEM_MAGIC     'g'
#define MEM_CLEAR           _IO(GLOBALMEM_MAGIC, 0)


int main(int argc, char const *argv[])
{
    int  fd      = -1;
    int  num     = 0;
    char buf[20] = {0};

    fd_set rfds;
    fd_set wfds;

    if (-1 != (fd = open("/dev/globalmem", O_RDONLY | O_NONBLOCK))) {
        if (ioctl(fd, MEM_CLEAR, 0))
            printf("ioctl error\n");

        for (;;) {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(fd, &rfds);
            FD_SET(fd, &wfds);
            select(fd +1, &rfds, &wfds, NULL, NULL);
            if (FD_ISSET(fd, &rfds))
                printf("can be read\n");
            if (FD_ISSET(fd, &wfds))
                printf("can be written\n");
        }
    }
    else
        printf("open device node error\n");

    return 0;
}
