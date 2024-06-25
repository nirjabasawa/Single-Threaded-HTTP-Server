//
// Nirja Basawa
// 05/07/24
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// create request struct type
typedef struct Request {
    char *method;
    char *uri;
    char *version;
    char *message_body;
    int in_fd;
    int content_length;
    int bytes_remaining;
} Request;

// parsing request - use regex
int request_parse(Request *req, char *buffer, ssize_t read_bytes);

// handling request in server
int request_handle(Request *req);
