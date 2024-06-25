//
// Nirja Basawa
// 05/07/24
//
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "request_parse.h"

#define BUFF_SIZE 2048


typedef struct {
    int fd;
} Listener_Socket;

int listener_init(Listener_Socket *sock, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if ((sock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (bind(sock->fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        return -1;
    }

    if (listen(sock->fd, 128) < 0) {
        return -1;
    }

    return 0;
}

int listener_accept(Listener_Socket *sock) {
    int connfd = accept(sock->fd, NULL, NULL);
    return connfd;
}
/* -------------------------------------------------------------------------- */

int get_line(int sock, char *buf, int size) {
    int i = 0;
    char client = '\0';
    int n_recv;

    while ((i < size - 1) && (client != '\n')) {
        n_recv = recv(sock, &client, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n_recv > 0) {
            if (client == '\r') {
                n_recv = recv(sock, &client, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", client); */
                if ((n_recv > 0) && (client == '\n'))
                    recv(sock, &client, 1, 0);
                else
                    client = '\n';
            }
            buf[i] = client;
            i++;
        } else
            client = '\n';
    }
    buf[i] = '\0';
    return (i);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char buffer[BUFF_SIZE + 1] = { '\0' };
    char *endptr = NULL;

    int port = (int) strtol(argv[1], &endptr, 10);
    if (port < 1 || port > 65535 || errno == EINVAL) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    Listener_Socket sock;
    int sock_status = listener_init(&sock, port);
    if (sock_status == -1) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    while (1) {
        int connfd = listener_accept(&sock);
        if (connfd == -1) {
            fprintf(stderr, "Cannot Establish Connection\n");
            return EXIT_FAILURE;
        }
    
        Request req;
        req.in_fd = connfd;
        ssize_t read_bytes = read(connfd, buffer, BUFF_SIZE);

        if (read_bytes == -1) {
            //printf("Inside read byte -1:\n");
            dprintf(req.in_fd,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            return EXIT_FAILURE;
        }
        if (request_parse(&req, buffer, strlen(buffer)) != EXIT_FAILURE) {
            request_handle(&req);
        }
        close(connfd);
        memset(buffer, '\0', sizeof(buffer));
    }
    return EXIT_SUCCESS;
}
