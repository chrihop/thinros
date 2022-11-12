#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#define BUFSIZE 512
#define PORT    14660

uint8_t udp_recv_buffer[BUFSIZE];

int main(int argc, char ** argv)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    socklen_t addr_len = sizeof(addr);
    int handshake;
    sendto(fd, &handshake, sizeof(handshake), 0, (struct sockaddr*)&addr, addr_len);

    while (true)
    {
        int len = recvfrom(fd, udp_recv_buffer, sizeof(udp_recv_buffer), 0, NULL, NULL);
        if (len == -1)
        {
            perror("[client] error on udp recvfrom");
            return 1;
        }
        if (len == 0)
        {
            printf("[client] connection closed\n");
            return 0;
        }
        int msg_id = udp_recv_buffer[9] << 16 | udp_recv_buffer[8] << 8 | udp_recv_buffer[7];
        int seq = udp_recv_buffer[4];
        printf("%02x: %d\n", seq, msg_id);
    }

    close(fd);
    return 0;
}
