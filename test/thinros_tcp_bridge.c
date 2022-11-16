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
#include <stdbool.h>
#include <threads.h>

#define PORT    14660

struct tcp_bridge_t
{
    int                sockfd;
    int                clientfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int                client_addr_len;
    bool               terminate;
    thrd_t             uplink_thread;
};

void uplink(void* data)
{
    struct tcp_bridge_t* bridge = (struct tcp_bridge_t*)data;
    while (!bridge->terminate)
    {
        while (bridge->clientfd == -1)
        {
            bridge->clientfd = accept(bridge->sockfd, &bridge->client_addr,
                                      &bridge->client_addr_len);
            if (bridge->clientfd == -1)
            {
                perror("accept error");
                sleep(1);
            }
        }

        ssize_t len = recv(bridge->clientfd, &packet, sizeof(packet), 0);
        if (len == 0)
        {
            printf("[client] server closed\n");
            break;
        }
        else if (len != sizeof(packet))
        {
            perror("[client] error on tcp recv, reconnect ...");
            int rv = -1;
            while (rv == -1)
            {
                close(bridge->clientfd);
                bridge->clientfd = socket(AF_INET, SOCK_STREAM, 0);
                rv = connect(bridge->clientfd, (struct sockaddr*)&bridge->client_addr, sizeof(bridge->client_addr));
            }
        }

        printf("%lu\n", packet.index);
        usleep(100000);
    }
}

int
main(int argc, char** argv)
{
    struct sockaddr_in server_addr;
    socklen_t          server_addr_len = sizeof(server_addr);

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, server_addr_len);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(sockfd, (const struct sockaddr*)&server_addr, server_addr_len) < 0)
    {
        perror("socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 32) < 0)
    {
        perror("socket listen failed");
        exit(EXIT_FAILURE);
    }

    int conn;




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
