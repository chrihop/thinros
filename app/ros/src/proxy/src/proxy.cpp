#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"

#define STD_LIBC

#include "lib/thinros_linux.h"
#include "lib/thinros_core.h"

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <string.h>
#include <assert.h>

namespace ser = ros::serialization;

struct node_handle_t this_node;
struct publisher_t   tr_steer_pub;
struct publisher_t   tr_throttle_pub;
struct publisher_t   tr_lidar_pub;
struct subscriber_t  tr_lidar_scan;

msg_steer_t steer_control;
msg_throttle_t throttle_control;
msg_lidar_t lidar_msg;

struct lidar_frame_header_t
{
    //header
    uint32_t magic;
    uint32_t len_ranges;
    uint32_t len_intensities;
    //basic info
    uint32_t seq;
    uint32_t stamp_secs;
    uint32_t stamp_nsecs;
    float angle_min;
    float angle_max;
    float angle_increment;
    float time_increment;
    float scan_time;
    float range_min;
    float range_max;
};

static bool lidar_frame_deserializer(void * data, sensor_msgs::LaserScan * ros_data)
{
    assert(data != NULL);
    assert(ros_data != NULL);

    static size_t seq = 0;

    struct lidar_frame_header_t * header = (struct lidar_frame_header_t *) data;
    if (header->magic != 0x4c415345)
    {
        ROS_DEBUG("WARNING lidar magic does not match, discard the frame");
        return false;
    }

    if (header->len_ranges > 1024u)
    {
        ROS_DEBUG("WARNING lidar frame too large, discard the frame");
        return false;
    }

    if (header->len_intensities > 1024u)
    {
        ROS_DEBUG("WARNING lidar frame too large, discard the frame");
        return false;
    }

    ros_data->header.seq = seq ++;
    ros_data->header.stamp.sec = header->stamp_secs;
    ros_data->header.stamp.nsec = header->stamp_nsecs;
    ros_data->angle_min = header->angle_min;
    ros_data->angle_max = header->angle_max;
    ros_data->angle_increment = header->angle_increment;
    ros_data->time_increment = header->time_increment;
    ros_data->scan_time = header->scan_time;
    ros_data->range_min = header->range_min;
    ros_data->range_max = header->range_max;

    size_t i;
    for (i = 0; i < header->len_ranges; i++)
    {
        size_t offset = sizeof(struct lidar_frame_header_t) + sizeof(float) * i;
        ros_data->ranges.push_back(*(float *) ((size_t) data + offset));
    }

    size_t intensities_ofs = sizeof(struct lidar_frame_header_t) + sizeof(float) * header->len_ranges;
    for (i = 0; i < header->len_intensities; i++)
    {
        size_t offset = intensities_ofs + sizeof(float) * i;
        ros_data->intensities.push_back(*(float *) ((size_t) data + offset));
    }
    return true;
}


ros::Publisher ros_lidar_scan;

void on_lidar_frame(void* data)
{
    static int count = 0;
    memcpy(&lidar_msg, data, sizeof(msg_lidar_t));
    sensor_msgs::LaserScan scan;

    if (lidar_frame_deserializer(&lidar_msg, &scan))
    {
        ros_lidar_scan.publish(scan);
        count ++;
        ROS_DEBUG("forward %d lidar scan messages.\n", count);
    }
}

int main(int argc, char **argv)
{
    printf("thinros proxy node start to run ...\n");

    struct thinros_ipc* ipc;
    ipc = thinros_bind();
    if (ipc == NULL)
    {
        return (EXIT_FAILURE);
    }

    /* no need to initialize the partition, if the partition is created by certikos first */
//    topic_partition_init(ipc->par);
    topic_nonsecure_partition_init(ipc->par);

    ros::init(argc, argv, "proxy_node");
    ros::NodeHandle n;

    ros_lidar_scan = n.advertise<sensor_msgs::LaserScan>("/fwd_scan", 1);
    ros::Rate loop_rate(20);

    /* create thinros node */
    thinros_node(&this_node, ipc->par, "proxy");
    thinros_subscribe(&tr_lidar_scan, &this_node, "fwd_scan", on_lidar_frame);

    while(ros::ok())
    {
        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);
        ros::spinOnce();
        loop_rate.sleep();
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
