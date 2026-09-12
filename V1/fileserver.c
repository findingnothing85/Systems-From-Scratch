#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>

int main() {
    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "53665", &hints, &result);

    int server_fd = socket(
        result->ai_family,
        result->ai_socktype,
        result->ai_protocol
    );

    bind(
        server_fd,
        result->ai_addr,
        result->ai_addrlen
    );

    listen(server_fd, 1);

    int client_fd = accept(server_fd, NULL, NULL);

    FILE *file = fopen("your-file-name", "r");

    char buffer[1024];
    size_t n = fread(
        buffer,
        1, 
        sizeof(buffer),
        file
    );

    send(
        client_fd,
        buffer,
        n,
        0
    );

    fclose(file);
    freeaddrinfo(result);

    close(client_fd);
    close(server_fd);

    return 0;
}
