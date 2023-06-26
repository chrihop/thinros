#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "thinros_core.h"
#include "thinros_linux.h"

struct node_handle_t this_node;

struct publisher_t  mav_msg_pub; // udp -> thinros msg
struct subscriber_t mav_msg_sub; // thinros msg -> udp

msg_mavlink_t mav_msg; // to be published to the secure world

int sockfd = -1;

#define PORT    14660
#define BUFSIZE 512

struct sockaddr_in client_addr;
int                client_addr_len;

uint8_t udp_send_buffer[BUFSIZE];
int     connected = 0;

void
printf_hex(uint8_t* data, int n)
{
    int i = 0;
    for (i = 0; i < n; i++)
        printf("%02x ", data[i]);
    printf("\n");
}

void
on_mav_msg(void* data)
{
    // message received from secure world
    msg_mavlink_t* msg = (msg_mavlink_t*)data;
    if (connected)
    {
        int msg_id = msg->data[9] << 16 | msg->data[8] << 8 | msg->data[7];
        printf("%02x, %d\n", msg->data[4], msg_id);
        memcpy(udp_send_buffer, msg->data, msg->len);
        if (sendto(sockfd, (const char*)udp_send_buffer, msg->len, 0,
                &client_addr, client_addr_len)
            < 0)
        {
            perror("udp sendto failed");
            exit(EXIT_FAILURE);
        }
        // printf("\tsent\n");
    }
}

int
main(int argc, char** argv)
{

    uint8_t            buffer[BUFSIZE];
    struct sockaddr_in server_addr;
    int                n;
    client_addr_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("sockfd = %d\n", sockfd);

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr))
        < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct thinros_ipc* ipc;
    ipc = thinros_bind();
    if (ipc == NULL)
    {
        return (EXIT_FAILURE);
    }
    /* no need to initialize the partition, if the partition is created by
     * certikos first */
    if (ipc->par->status == PARTITION_UNINITIALIZED)
    {
        topic_partition_init(ipc->par);
    }
    topic_nonsecure_partition_init(ipc->par);

    /* create node */
    thinros_node(&this_node, ipc->par, "udp_proxy");

    /* create publishers and subscribers */
    thinros_advertise(&mav_msg_pub, &this_node, "mav_gateway_in");
    thinros_subscribe(&mav_msg_sub, &this_node, "mav_gateway_out", on_mav_msg);

    size_t total = 0;
    while (true)
    {
        // Non-blocking
        n = recvfrom(sockfd, buffer, BUFSIZE, MSG_DONTWAIT,
            (struct sockaddr*)&client_addr, &client_addr_len);

        if (n > 0)
        {
            connected = 1;
            // printf("UDP received %d bytes\n", n);
            // printf_hex(buffer, n);

            // publish
            mav_msg.len = n;
            memcpy(mav_msg.data, buffer, n);
            thinros_publish(&mav_msg_pub, &mav_msg, sizeof(msg_mavlink_t));
        }
        else
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                // due to non-blocking
            }
            else
            {
                perror("recvfrom error");
                exit(EXIT_FAILURE);
            }
        }

        thinros_spin(&this_node, SPIN_ONCE, NULL, 0llu);

        // usleep(20000);
        usleep(100);
    }

    thinros_unbind(ipc);
    return EXIT_SUCCESS;
}
