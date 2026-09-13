#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>

int main() {
    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(
        "192.168.11.5",
        "53665",
        &hints,
        &result
    );

    int sockfd = socket(
        result->ai_family,
        result->ai_socktype,
        result->ai_protocol
    );

    connect(
        sockfd,
        result->ai_addr,
        result->ai_addrlen
    );

    char buffer[1024];

    ssize_t n;

    FILE *file = fopen("dog.png", "w");

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
