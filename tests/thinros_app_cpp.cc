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
#include <atomic>

#include "thinros_linux.h"
#include "thinros_core.h"

struct node_handle_t this_node;

struct publisher_t steer_pub;
struct publisher_t throttle_pub;
struct publisher_t lidar_pub;
struct subscriber_t lidar_scan;

msg_steer_t steer_control;
msg_throttle_t throttle_control;
msg_lidar_t lidar_msg;

void on_lidar_frame(void* data)
{
    msg_lidar_t* lidar = (msg_lidar_t*)data;
    printf("lidar: %u\n", lidar->size);
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
    topic_partition_init(ipc->par);

    /* create node */
    thinros_node(&this_node, ipc->par, "proxy");

    /* create publishers and subscribers */
    /* topic name see @file://./lib/thinros_cfg.c */
    thinros_advertise(&steer_pub, &this_node, "drv_steer");
    thinros_advertise(&throttle_pub, &this_node, "drv_throttle");
    thinros_subscribe(&lidar_scan, &this_node, "fwd_scan", on_lidar_frame);

    thinros_advertise(&lidar_pub, &this_node, "fwd_scan");
    lidar_msg.size = 4;
    lidar_msg.value[0] = 5;
    thinros_publish(&lidar_pub, &lidar_msg, sizeof(msg_lidar_t));


    while (true)
    {
        steer_control.value++;
        thinros_publish(&steer_pub, &steer_control, sizeof(msg_steer_t));

        throttle_control.value++;
        thinros_publish(&throttle_pub, &throttle_control, sizeof(msg_steer_t));

        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
