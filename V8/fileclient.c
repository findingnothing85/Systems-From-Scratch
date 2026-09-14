#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>

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
        "192.168.11.5",
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

    char buffer[1024];

    ssize_t n;

    FILE *file = fopen(argv[1], "w");

    if (file == NULL) {
        perror("fopen");
        return 1;
    };

    while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(
            buffer,
            1,
            n,
            file
        );
    }

    fclose(file);
    freeaddrinfo(result);
    close(sockfd);

    return 0;
}
