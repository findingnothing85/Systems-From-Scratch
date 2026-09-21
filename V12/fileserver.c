#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

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

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

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
            return NULL;
        }

        if (received == 0) {
            printf("Client closed before request completed\n");
            close(client_fd);
            return NULL;
        }

        total_received += received;
        request[total_received] = '\0';

        if(strchr(request,'\n') != NULL) {
            break;
        };
    };

    request[strcspn(request, "\n")] = '\0';

    printf(
        "Client requested: %s\n",
        request
    );

    FILE *file  = fopen(request, "r");

    if (file == NULL) {
        perror("fopen");
        close(client_fd);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char header[64];

    int header_length = snprintf(
        header,
        sizeof(header),
        "%ld\n",
        file_size
    );

    if (send_all(client_fd, header, header_length) == -1) {
        perror("send");
        fclose(file);
        close(client_fd);
        return NULL;
    }

    char buffer[1024];

    size_t n;

    int send_failed = 0;

    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send_all(client_fd, buffer, n) == -1) {
            perror("send");
            fclose(file);
            close(client_fd);
            return NULL;
        }
    }

    fclose(file);
    close(client_fd);

    return NULL;
}

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

    if (server_fd == -1) {
        fprintf(stderr, "Failed to bind to any address\n");
        freeaddrinfo(result);
        return 1;
    };

    freeaddrinfo(result);

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    };

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            perror("accept");
            continue;
        };

        int *client_arg = malloc(sizeof(int));

        if (client_arg == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *client_arg = client_fd;

        pthread_t thread;

        if (pthread_create(
            &thread,
            NULL,
            handle_client,
            client_arg
        ) != 0) {
            perror("pthread_create");
            free(client_arg);
            close(client_fd);
            continue;
        }

        pthread_detach(thread);
    }

    close(server_fd);

    return 0;
}
