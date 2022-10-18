#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"

#define STD_LIBC

#include "lib/thinros_linux.h"
#include "lib/thinros_core.h"

#include <stddef.h>
#include <stdint.h>

struct node_handle_t this_node;
struct publisher_t   tr_lidar_scan_pub;

msg_lidar_t lidar_msg;

int main(int argc, char **argv)
{
    printf("thinros proxy test node start to run ...\n");

    struct thinros_ipc* ipc;
    ipc = thinros_bind();
    if (ipc == NULL)
    {
        return (EXIT_FAILURE);
    }

    ros::init(argc, argv, "proxy_test_node");
    ros::NodeHandle n;

    ros::Rate loop_rate(20);

    /* create thinros node */
    topic_partition_init(ipc->par);
    thinros_node(&this_node, ipc->par, "proxy_test");
    thinros_advertise(&tr_lidar_scan_pub, &this_node, "fwd_scan");

    char count = 0;
    while(ros::ok())
    {
        lidar_msg.size = 4;
        lidar_msg.value[3] = count++;
        thinros_publish(&tr_lidar_scan_pub, &lidar_msg, sizeof(msg_lidar_t));
        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);
        ros::spinOnce();
        loop_rate.sleep();
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
