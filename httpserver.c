//
// Nirja Basawa
// nbasawa
// UCSC CSE130
// Assignment 4
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
#include <pthread.h>

#include "request_parse.h"
#include "queue.h"

#define BUFF_SIZE          2048
#define DEFAULT_THREAD_NUM 4

// global variables
pthread_t *worker_threads;
int num_threads;
queue_t *connection_queue;

// global funcs
void *worker();

// struct for connection (so worker can access)
/*typedef struct {
    int connfd;
} Connection;*/

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

void handle_connection(int connfd) {
    //Handle connection;
    char buffer[BUFF_SIZE + 1] = { '\0' };
    //connfd = listener_accept(&sock);
    if (connfd == -1) {
        fprintf(stderr, "Cannot Establish Connection\n");
        return;
    }
    //handle_connection(connfd);
    Request req;
    req.in_fd = connfd;
    // ssize_t read_bytes = read_until(connfd, buffer, BUFF_SIZE, "\r\n\r\n");
    ssize_t read_bytes = read(connfd, buffer, BUFF_SIZE);

    if (read_bytes == -1) {
        //printf("Inside read byte -1:\n");
        dprintf(
            req.in_fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return;
    }
    if (request_parse(&req, buffer, strlen(buffer)) != EXIT_FAILURE) {
        request_handle(&req);
    }
    close(connfd);
    memset(buffer, '\0', sizeof(buffer));
}

void *worker() {
    uintptr_t sock_fd = 0;
    while (1) {
        queue_pop(connection_queue, (void **) &sock_fd);
        handle_connection(sock_fd);
        close(sock_fd);
    }
}

int main(int argc, char *argv[]) {
    // correct num of arguments?
    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    //char buffer[BUFF_SIZE + 1] = { '\0' };
    char *endptr = NULL;
    int opt;
    int port;
    num_threads = DEFAULT_THREAD_NUM;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': num_threads = (int) strtol(optarg, &endptr, 10); break;
        default: fprintf(stderr, "usage: %s [-t threads <port>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }

    port = (int) strtol(argv[optind], &endptr, 10);
    if (port < 1 || port > 65535 || errno == EINVAL) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    // ignores sigpipe signals
    signal(SIGPIPE, SIG_IGN);

    Listener_Socket sock;
    int sock_status = listener_init(&sock, port);
    if (sock_status == -1) {
        fprintf(stderr, "Invalid Port\n");
        return EXIT_FAILURE;
    }

    // initialize queue for connections
    connection_queue = queue_new(num_threads);

    // create worker threads
    worker_threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&worker_threads[i], NULL, worker, NULL);
    }

    while (1) {
        // accept connection, add to connection_queue
        uintptr_t connfd = (uintptr_t) listener_accept(&sock);
        queue_push(connection_queue, (void *) connfd);

        /*int connfd = listener_accept(&sock);
        if (connfd == -1) {
            fprintf(stderr, "Cannot Establish Connection\n");
            return EXIT_FAILURE;
        }
        //handle_connection(connfd);
        Request req;
        req.in_fd = connfd;
        // ssize_t read_bytes = read_until(connfd, buffer, BUFF_SIZE, "\r\n\r\n");
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
        memset(buffer, '\0', sizeof(buffer));*/
    }
    return EXIT_SUCCESS;
}

/*int get_line(int sock, char *buf, int size) {
    int i = 0;
    char client = '\0';
    int n_recv;

    while ((i < size - 1) && (client != '\n')) {
        n_recv = recv(sock, &client, 1, 0);
        */
/* DEBUG printf("%02X\n", c); */ /*
        if (n_recv > 0) {
            if (client == '\r') {
                n_recv = recv(sock, &client, 1, MSG_PEEK);
                */
/* DEBUG printf("%02X\n", client); */ /*
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
}*/
