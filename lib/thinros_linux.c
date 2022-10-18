#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "thinros_linux.h"
#include "thinros_core.h"

#define n_devpath				"/proc/thinros"

struct thinros_ipc* thinros_bind(void)
{
    struct thinros_ipc* ipc = malloc(sizeof(struct thinros_ipc));

    ipc->fd = open(n_devpath, O_RDWR | O_SYNC);
    if (ipc->fd < 0)
    {
        perror(
            "cannot open file, please check if thinros driver is installed.\n");
        free(ipc);
        return NULL;
    }

    ipc->par = mmap((void *) NONSECURE_PARTITION_LOC, NONSECURE_PARTITION_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, ipc->fd, 0);
    if (ipc->par == MAP_FAILED)
    {
        perror("cannot bind to thinros partition (mmap failed)!\n");
        close(ipc->fd);
        free(ipc);
        return NULL;
    }
    return ipc;
}

void thinros_unbind(struct thinros_ipc* ipc)
{
    if (ipc != NULL)
    {
        close(ipc->fd);
        free(ipc);
    }
}


