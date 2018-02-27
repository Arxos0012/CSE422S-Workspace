#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>

int 
main(int     argc,
     char ** argv)
{
    int fd, status;

    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
        return -1;
    }
    
    status = enable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }

    sleep(3);

    status = disable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    printf("Monitor ran to completion!\n");

    return 0;
}
