#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "thinros_core.h"
#include "thinros_linux.h"


struct node_handle_t this_node;

struct publisher_t steer_pub;
struct publisher_t throttle_pub;
struct publisher_t lidar_pub;
struct subscriber_t lidar_scan;

msg_drv_steer_t steer_control;
msg_drv_throttle_t throttle_control;
msg_fwd_scan_t lidar_msg;

static struct timespec t_start, t_last, t_now;
static size_t count = 0;

void on_lidar_frame(void* data)
{
    msg_fwd_scan_t* lidar = (msg_fwd_scan_t*)data;
    count ++;
//    printf("lidar frame size: %u <%u>\n", lidar->size, lidar->value[0]);
}

static time_t timespec_diff(struct timespec *t1, struct timespec * t0)
{
    /* return t1 - t0 */
    time_t v = t1->tv_nsec - t0->tv_nsec +
                (t1->tv_sec - t0->tv_sec) * 1000000000;
    return v;
}

int main(int argc, char** argv)
{
    struct thinros_ipc* ipc;
    ipc = thinros_bind();
    if (ipc == NULL)
    {
        return (EXIT_FAILURE);
    }
    /* no need to initialize the partition, if the partition is created by certikos first */
    if (ipc->par->status == PARTITION_UNINITIALIZED)
    {
        topic_partition_init(ipc->par);
    }
    topic_nonsecure_partition_init(ipc->par);

    /* create node */
    thinros_node(&this_node, ipc->par, "proxy");

    /* create publishers and subscribers */
    /* topic name see @file://./lib/thinros_cfg.c */
    thinros_advertise(&steer_pub, &this_node, "drv_steer");
    thinros_advertise(&throttle_pub, &this_node, "drv_throttle");
    thinros_subscribe(&lidar_scan, &this_node, "fwd_scan", on_lidar_frame);

    clock_gettime(CLOCK_REALTIME, &t_last);
    t_start = t_last;
    size_t total = 0;
    while (true)
    {
        steer_control.value++;
        thinros_publish(&steer_pub, &steer_control, sizeof(msg_drv_steer_t));

        throttle_control.value++;
        thinros_publish(&throttle_pub, &throttle_control, sizeof(msg_drv_steer_t));

        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);

        clock_gettime(CLOCK_REALTIME, &t_now);
        if (t_now.tv_sec - t_last.tv_sec >= 1)
        {
            total += count;
            printf("[%.3f] duration %.3f received %lu (%lu KB/s)\n",
                timespec_diff(&t_now, &t_start) / 1000000000.0,
                timespec_diff(&t_now, &t_last) / 1000000000.0,
                total, count * sizeof(msg_fwd_scan_t) / 1024);
            t_last = t_now;
            count = 0;
        }

        usleep(20000);
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
