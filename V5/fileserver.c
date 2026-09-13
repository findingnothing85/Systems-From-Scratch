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

    int status = getaddrinfo(NULL, "53665", &hints, &result);

    if (status != 0) {
        printf(
            "getaddrinfo failed: %s\n",
            gai_strerror(status)
        );
        return 1;
    };

    int server_fd;

    for (
        struct addrinfo *p = result;
        p != NULL;
        p = p->ai_next
    ) {
        server_fd = socket(
            p->ai_family,
            p->ai_socktype,
            p->ai_protocol
        );

        if (server_fd == -1) {
            continue; 
        };

        if (bind(
            server_fd,
            p->ai_addr,
            p->ai_addrlen
        ) == 0) {
            break;
        };

        close(server_fd);
        server_fd = -1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        freeaddrinfo(result);
        close(server_fd);
        return 1;
    };

    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd == -1) {
        perror("accept");
        freeaddrinfo(result);
        close(server_fd);
        return 1;
    };

    FILE *file  = fopen("dog.png", "r");

    if (file == NULL) {
        perror("fopen");
        freeaddrinfo(result);
        close(server_fd);
        close(client_fd);
        return 1;
    }

    char buffer[1024];

    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sent = send(
            client_fd,
            buffer,
            n,
            0
        );

        if (sent == -1) {
            perror("send");
            fclose(file);
            freeaddrinfo(result);
            close(server_fd);
            close(client_fd);
            return 1;
        }
    }

    fclose(file);
    freeaddrinfo(result);

    close(client_fd);
    close(server_fd);

    return 0;
}
