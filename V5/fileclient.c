#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>

int main() {
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
            "getaddrinfo failed: %s/n",
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

    char buffer[1024];

    ssize_t n;

    FILE *file = fopen("V5.md", "w");

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
