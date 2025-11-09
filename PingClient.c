#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

#define PING_COUNT 10
#define BUFFER_SIZE 1024

double time_diff_ms(struct timeval *start, struct timeval *end) {
    return (double)(end->tv_sec - start->tv_sec) * 1000.0 +
           (double)(end->tv_usec - start->tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        exit(1);
    }

    char *server_name = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    //make udp sock
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(1);
    }

    //serv addy
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_name);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(1);
    }

    int sent = 0;
    int received = 0;
    double min_rtt = 1e9;
    double max_rtt = 0.0;
    double sum_rtt = 0.0;

    for (int seq = 1; seq <= PING_COUNT; seq++) {
        sent++;

        struct timeval start, end;
        gettimeofday(&start, NULL);
        snprintf(buffer, BUFFER_SIZE, "PING %d %ld.%06ld",
                 seq, (long)start.tv_sec, (long)start.tv_usec);

        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr *) &server_addr, addr_len);
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                (struct sockaddr *) &server_addr, &addr_len);

        gettimeofday(&end, NULL);

        if (recv_len < 0) {
            printf("Request timeout for seq#=%d\n", seq);
        } else {
            buffer[recv_len] = '\0';
            double rtt = time_diff_ms(&start, &end);
            received++;

            if (rtt < min_rtt) min_rtt = rtt;
            if (rtt > max_rtt) max_rtt = rtt;
            sum_rtt += rtt;

            printf("PING received from %s: seq#=%d time=%.3f ms\n",
                   server_name, seq, rtt);
        }

        sleep(1);
    }

    // double loss = ( (double) (sent - received) / (double)sent) * 100.0;
    // double avg_rtt = (received > 0) ? (sum_rtt / (double) received) : 0.0;

    // printf("--- %s ping statistics ---\n", server_name);
    // printf("%d packets transmitted, %d received, %d%% packet loss ",
    //        sent, received, (int)loss);
    // if (received > 0) {
    //     printf("rtt min/avg/max = %.3f %.3f %.3f ms\n",
    //            min_rtt, avg_rtt, max_rtt);
    // }

    // close(sockfd);
    // return 0;
    double loss = ((double)(sent - received) / (double)sent) * 100.0;
    double avg_rtt = (received > 0) ? (sum_rtt / (double)received) : 0.0;

    printf("--- %s ping statistics ---\n", server_name);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n",
        sent, received, loss);

    if (received > 0) {
        printf("rtt min/avg/max = %.3f %.3f %.3f ms\n",
            min_rtt, avg_rtt, max_rtt);
    }

    close(sockfd);
    return 0;
}

