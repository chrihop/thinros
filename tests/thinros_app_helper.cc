#include <unistd.h>
#include <fcntl.h>
#include <string>
#include "thinros_core.h"
#include "thinros_linux.h"

using namespace std;

struct node_handle_t this_node;

struct publisher_t lidar_scan;

struct subscriber_t steer;
struct subscriber_t throttle;

msg_steer_t steer_control;
msg_throttle_t throttle_control;
msg_lidar_t lidar_msg;

static size_t steer_count = 0, throttle_count = 0;
static float current_steer = 0;
static float current_throttle = 0;

void on_steer_control(void* data)
{
    auto* msg = (msg_steer_t*)data;
    steer_count ++;
    current_steer = msg->value;
}

void on_thr_control(void* data)
{
    auto* msg = (msg_throttle_t*)data;
    throttle_count ++;
    current_throttle = msg->value;
}

int main(int argc, char** argv)
{
    struct thinros_ipc* ipc;
    ipc = thinros_bind();
    if (ipc == nullptr)
    {
        return (EXIT_FAILURE);
    }

    fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK);

    /* no need to initialize the partition, if the partition is created by certikos first */
    if (ipc->par->status == PARTITION_UNINITIALIZED)
    {
        topic_partition_init(ipc->par);
    }
    topic_nonsecure_partition_init(ipc->par);

    /* create node */
    thinros_node(&this_node, ipc->par, (char *) (string{"proxy"}).c_str());

    /* create publishers and subscribers */
    /* topic name see @file://./lib/thinros_cfg.c */
    thinros_advertise(&lidar_scan, &this_node, (char *) (string{"fwd_scan"}).c_str());
    thinros_subscribe(&steer, &this_node, (char *) (string{"drv_steer"}).c_str(), on_steer_control);
    thinros_subscribe(&throttle, &this_node, (char *) (string{"drv_throttle"}).c_str(), on_thr_control);

    size_t total = 0;
    while (true)
    {
        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);
        thinros_publish(&lidar_scan, &lidar_msg, sizeof(msg_lidar_t));
        total ++;
        printf("[%lu] steer: %f (count=%lu), throttle: %f (count=%lu)\n",
               total, current_steer, steer_count, current_throttle, throttle_count);
        usleep(20000);
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
