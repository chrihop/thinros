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

#ifndef _THINROS_NETWORK_DOS_DEMO_
#error "This file is only for thinros network dos demo"
#endif

#if defined(__aarch64__)
gcc_inline uint64_t pmccntr_el0()
{
    uint64_t val=0;
    asm volatile("mrs %0, pmccntr_el0" : "=r" (val));
    return val;
}

gcc_inline uint64_t cntvct_el0()
{
    uint64_t val=0;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

gcc_inline uint64_t get_timestamp()
{
    return cntvct_el0();
}
#elif defined(__x86_64__) || defined(__i386__)
gcc_inline uint64_t get_timestamp()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
#endif

struct node_handle_t adv_node, atk_node;
struct publisher_t adv_pub, atk_pub;
static msg_control_t adv_control, atk_control;

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
    thinros_node(&adv_node, ipc->par, "advanced_controller");
    thinros_node(&atk_node, ipc->par, "attacker");

    /* create publishers and subscribers */
    /* topic name see @file://./lib/thinros_cfg.c */
    thinros_advertise(&adv_pub, &adv_node, "control");
    thinros_advertise(&atk_pub, &atk_node, "control");

    /* create attacker thread */
    pid_t pid = fork();
    if (pid == 0)
    {
        /* attacker */
        memset(&atk_control, 0, sizeof(msg_control_t));
        atk_control.id = 1;
        atk_control.len = 32;
        atk_control.sequence = 0;
        struct timespec t_100ns;
        t_100ns.tv_nsec = 100;

        while (true)
        {
            atk_control.timestamp = get_timestamp();
            atk_control.sequence++;
            thinros_publish(&atk_pub, &atk_control, sizeof(msg_control_t));
            nanosleep(&t_100ns, NULL);
        }
    }

    /* create advanced controller thread */
    memset(&adv_control, 0, sizeof(msg_control_t));
    adv_control.id = 0;
    adv_control.len = 32;
    adv_control.sequence = 0;
    struct timespec t_10ms;
    t_10ms.tv_nsec = 10000000;

    while(true)
    {
        adv_control.sequence++;
        for (int i = 0; i < 32; i++)
        {
            adv_control.data[i] = rand() % 256;
        }
        adv_control.timestamp = get_timestamp();
        thinros_publish(&adv_pub, &adv_control, sizeof(msg_control_t));
        nanosleep(&t_10ms, NULL);
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
