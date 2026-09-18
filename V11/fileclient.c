#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    };

    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(
        "192.168.11.4",
        "53665",
        &hints,
        &result
    );

    if (status != 0) {
        printf(
            "getaddrinfo failed: %s\n",
            gai_strerror(status)
        );
        return 1;
    };

    int sockfd;

    for (
        struct addrinfo *p = result;
        p != NULL;
        p = p->ai_next
    ) {
        sockfd = socket(
            p->ai_family,
            p->ai_socktype,
            p->ai_protocol
        );

        if (sockfd == -1) {
            continue;
        }

        if (connect(
                sockfd,
                p->ai_addr,
                p->ai_addrlen
            ) == 0) {
                break;
        }

        close(sockfd);
        sockfd = -1;
    }

    if (sockfd == -1) {
        fprintf(stderr, "Failed to connect to any address\n");
        freeaddrinfo(result);
        return 1;
    };

    char request[1024];

    snprintf(
        request,
        sizeof(request),
        "%s\n",
        argv[1]
    );

    size_t total_sent = 0;
    size_t request_size = strlen(request);

    while (total_sent < request_size) {
        ssize_t sent = send(
            sockfd,
            request + total_sent,
            request_size - total_sent,
            0
        );

        if (sent == -1) {
            perror("send");
            freeaddrinfo(result);
            close(sockfd);
            return 1;
        }

        total_sent += sent;
    }

    char size_buffer[64];
    size_t size_received = 0;

    for (;;) {
        char c;

        ssize_t received = recv(
            sockfd,
            &c,
            1,
            0
        );

        if (received == -1) {
            perror("recv");
            close(sockfd);
            return 1;
        }

        if (received == 0) {
            printf("Server closed connection\n");
            close(sockfd);
            return 1;
        }

        if (c == '\n') {
            break;
        }

        size_buffer[size_received] = c;
        size_received++;
    }

    size_buffer[size_received] = '\0';

    long file_size = atol(size_buffer);

    printf("File size: %ld bytes\n", file_size);

    char buffer[1024];

    ssize_t n;

    FILE *file = fopen(argv[1], "w");

    if (file == NULL) {
        perror("fopen");
        return 1;
    };

    long total_received = 0;

    while (total_received < file_size) {
        ssize_t n = recv(
            sockfd,
            buffer,
            sizeof(buffer),
            0
        );

        if (n == -1) {
            perror("recv");
            fclose(file);
            close(sockfd);
            return 1;
        }

        if (n == 0) {
            printf("Server closed connection early\n");
            break;
        }
        
        fwrite(
            buffer,
            1,
            n,
            file
        );

        total_received += n;

        double progress =
            (double) total_received /
            file_size *
            100.0;

        printf(
            "\rProgress: %.1f%%",
            progress
        );

        fflush(stdout);
    }

    printf("\n");

    fclose(file);
    freeaddrinfo(result);
    close(sockfd);

    return 0;
}
