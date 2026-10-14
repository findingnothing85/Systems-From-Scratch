#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>

const char *storage_ips[] = {
    "192.168.11.4",
    "192.168.11.8"
};

#define NUM_SERVERS 2

int send_all(int fd, char *content, size_t n) {
    size_t total_sent = 0;
    while (total_sent < n) {
        ssize_t sent = send(
            fd,
            content + total_sent,
            n - total_sent,
            0
        );

        if (sent == -1) {
            return -1;
        }

        total_sent += sent;
    }

    return 0;
}

int read_header(int fd, char *header, size_t size) {
    size_t total_received = 0;

    for (;;) {
        if (total_received >= size -1) {
            return -1;
        }

        char c;

        ssize_t received = recv(fd, &c, 1, 0);

        if (received <= 0) {
            return -1;
        }

        header[total_received++] = c;

        if (c == '\n') {
            header[total_received] = '\0';
            return total_received;
        }
    }
}

void handle_client(int client_fd) {
    char request[1024];
    size_t total_received = 0;

    for (;;) {
        ssize_t received = recv(
            client_fd,
            request + total_received,
            sizeof(request) - 1 - total_received,
            0
        );

        if (received == -1) {
            perror("recv");
            close(client_fd);
            return;
        }

        if (received == 0) {
            printf("Client closed before request completed\n");
            close(client_fd);
            return;
        }

        total_received += received;
        request[total_received] = '\0';

        if (strchr(request, '\n') != NULL) {
            break;
        }
    }

    printf(
        "Client requested: %s\n",
        request
    );

    int search_failed = 0;

    for (int i = 0; i < NUM_SERVERS; i++) {

        printf("Trying server %s\n", storage_ips[i]);
        fflush(stdout);

        struct addrinfo hints = {0};
        struct addrinfo *result;

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int status = getaddrinfo(
            storage_ips[i],
            "53665",
            &hints,
            &result
        );

        if (status != 0) {
            printf(
                "getaddrinfo failed: %s\n",
                gai_strerror(status)
            );

            search_failed = 1;
            continue;   
        }

        int storage_fd = -1;

        for (
            struct addrinfo *p = result;
            p != NULL;
            p = p->ai_next
        ) {
            storage_fd = socket(
                p->ai_family,
                p->ai_socktype,
                p->ai_protocol
            );

            if (storage_fd == -1) {
                continue;
            }

            if (connect(
                storage_fd,
                p->ai_addr,
                p->ai_addrlen
            ) == 0) {
                break;
            }

            close(storage_fd);
            storage_fd = -1;
        }

        printf("Connected to storage server\n");
        fflush(stdout);

        if (storage_fd == -1) {
            fprintf(stderr, "Failed to connect to any address\n");

            search_failed = 1;
            continue;
        }

        size_t len = strlen(request);
        request[len] = '\n';
        request[len + 1] = '\0';

        if (send_all(
            storage_fd,
            request,
            strlen(request)
        ) == -1) {
            perror("send");
            close(storage_fd);
            search_failed = 1;
            continue;
        }

        char header[64];

        printf("Waiting for storage response...\n");
        fflush(stdout);

        int header_length = read_header(
            storage_fd,
            header,
            sizeof(header)
        );

        if (header_length == -1) {
            close(storage_fd);
            search_failed = 1;
            continue;
        }

        if (strcmp(header, "NOT_FOUND\n") == 0) {
            printf(
                "File not found on %s\n",
                storage_ips[i]
            );

            close(storage_fd);
            continue;
        }

        if (send_all(
            client_fd,
            header,
            header_length
        ) == -1) {
            close(storage_fd);
            close(client_fd);
            return;
        }

        char buffer[1024];
        ssize_t n;

        while((n = recv(
            storage_fd,
            buffer,
            sizeof(buffer),
            0
        )) > 0) {
            if (send_all(
                client_fd,
                buffer,
                n
            ) == -1) {
                break;
            }
        }

        close(storage_fd);
        close(client_fd);
        return;
    }

    if (search_failed) {
        char response[] = "ERROR\n";
        send_all(client_fd, response, strlen(response));
    } else {
        char response[] = "NOT_FOUND\n";
        send_all(client_fd, response, strlen(response));
    }

    close(client_fd);

    return;
}

int main() {
    struct addrinfo hints = {0};
    struct addrinfo *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "53666", &hints, &result);

    if (status != 0) {
        printf(
            "getaddrinfo failed: %s\n",
            gai_strerror(status)
        );
    }

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
        }

        int yes = 1;

        if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &yes,
            sizeof(yes)
        ) == -1) {
            perror("setsockopt");
            close(server_fd);
            continue;
        }

        if (bind(
            server_fd,
            p->ai_addr,
            p->ai_addrlen
        ) == 0) {
            break;
        }

        close(server_fd);
        server_fd = -1;
    }

    freeaddrinfo(result);

    if (server_fd == -1) {
        fprintf(stderr, "Failed to bind to any address\n");
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        handle_client(client_fd);
    }
}
