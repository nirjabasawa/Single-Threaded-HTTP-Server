//
// Nirja Basawa
// nbasawa
// UCSC CSE130
// Assignment 4
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "request_parse.h"

#define REQREGEX    "([A-Za-z]+) +(/[a-zA-Z0-9.-]{1,63}) +(HTTP/[0-9][.][0-9])"
#define REQIDREGEX  "([a-zA-Z0-9.-]{1,128}): ([1-128])+\r\n"
#define HEADERREGEX "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})+\r\n"

// method helper functions

// int method_get(), int method_put()
int method_get(Request *req);
int method_put(Request *req);

// int request_handle()
int request_handle(Request *req) {
    // check for possible errors in method and version
    if (strncmp(req->version, "HTTP/1.1", 8) != 0) {
        dprintf(req->in_fd,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: %d\r\n\r\nVersion Not "
            "Supported\n",
            22);
        return EXIT_FAILURE;
    } else if (strncmp(req->method, "GET", 3) == 0) {
        return (method_get(req));
    } else if (strncmp(req->method, "PUT", 3) == 0) {
        return (method_put(req));
    } else {
        // method is not get/put -- not implemented error
        dprintf(req->in_fd,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\n Not Implemented\n", 16);
        return EXIT_FAILURE;
    }
}

// int request_parse()
int request_parse(Request *req, char *buffer, ssize_t read_bytes) {
    //printf("\nEntering request_parse");
    int tot_offset = 0;
    regex_t regex;
    regmatch_t pmatches[4];
    int rv;

    rv = regcomp(&regex, REQREGEX, REG_EXTENDED);
    rv = regexec(&regex, buffer, 4, pmatches, 0);

    if (rv == 0) { // matched into buffer
        //printf("\nMatched regex");
        // get method, uri, and version from buffer
        req->method = buffer;
        req->uri = buffer + pmatches[2].rm_so;
        req->version = buffer + pmatches[3].rm_so;

        // fit null char at end of string
        buffer[pmatches[1].rm_eo] = '\0';
        req->uri[pmatches[2].rm_eo - pmatches[2].rm_so] = '\0';
        req->version[pmatches[3].rm_eo - pmatches[3].rm_so] = '\0';

        // move to next regex area
        buffer += pmatches[3].rm_eo + 2;
        tot_offset += pmatches[3].rm_eo + 2;
    } else {
        //printf("Not able to match regex");
        // remember to free regex
        dprintf(
            req->in_fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        regfree(&regex);
        return EXIT_FAILURE;
    }

    // request-id parsing
    req->request_id = 0; // by default, if not found should be 0
    if (buffer[2] != '\r' && buffer[3] != '\n') {
        rv = regcomp(&regex, REQIDREGEX, REG_EXTENDED);
        rv = regexec(&regex, buffer, 3, pmatches, 0);

        if (rv == 0) {
            // update null char at end
            buffer[pmatches[1].rm_eo] = '\0';
            buffer[pmatches[2].rm_eo] = '\0';
            if (strncmp(buffer, "Request-Id", 10) == 0) {
                // match
                int val = strtol(buffer + pmatches[2].rm_so, NULL, 10);
                if (errno == EINVAL) {
                    //printf("\nInside request-id check");
                    // invalid
                    dprintf(req->in_fd,
                        "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
                }
                req->request_id = val;
            }
            buffer += pmatches[2].rm_eo + 2;
            tot_offset += pmatches[2].rm_eo + 2;
            //rv = regexec(&regex, buffer, 3, pmatches, 0);
        }
    }

    //printf("\nmethod=%s, uri=%s, version=%s", req->method, req->uri, req->version);
    // content length parsing
    req->content_length = -1; // value -1 for unknown

    if (strncmp(req->method, "PUT", 3) == 0) {
        rv = regcomp(&regex, HEADERREGEX, REG_EXTENDED);
        rv = regexec(&regex, buffer, 3, pmatches, 0);

        while (rv == 0) {
            // update null char at end
            buffer[pmatches[1].rm_eo] = '\0';
            buffer[pmatches[2].rm_eo] = '\0';
            if (strncmp(buffer, "Content-Length", 14) == 0) {
                // matched
                int val = strtol(buffer + pmatches[2].rm_so, NULL, 10);
                if (errno == EINVAL) {
                    //printf("\nInside content-length check");
                    // invalid
                    dprintf(req->in_fd,
                        "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
                }
                req->content_length = val;
            }
            buffer += pmatches[2].rm_eo + 2;
            tot_offset += pmatches[2].rm_eo + 2;
            rv = regexec(&regex, buffer, 3, pmatches, 0);
        }

        // message body parsing
        if ((rv != 0) && (buffer[0] == '\r' && buffer[1] == '\n')) {
            req->message_body = buffer + 2;
            tot_offset += 2;
            req->bytes_remaining = read_bytes - tot_offset;
        } else if (rv != 0) {
            //printf("\nInside message body parsing");
            dprintf(req->in_fd,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            regfree(&regex);
            return EXIT_FAILURE;
        }
    }

    // free regex, done
    regfree(&regex);
    return EXIT_SUCCESS;
}

// method_get()
int method_get(Request *req) {
    //printf("\nStart of method_get");
    // open connection
    int fd;
    char *fileToServe = req->uri + 1;

    if ((fd = open(fileToServe, O_RDONLY | O_DIRECTORY)) != -1) {
        dprintf(req->in_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        fprintf(stderr, "%s,%s,403,%d", req->method, req->uri, req->request_id);
        return EXIT_FAILURE;
    }
    if ((fd = open(fileToServe, O_RDONLY)) == -1) {
        if (errno == ENOENT) {
            // file/directory doesn't exist
            dprintf(
                req->in_fd, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nNot Found\n", 10);
            fprintf(fprintf(stderr, "%s,%s,404,%d", req->method, req->uri, req->request_id);)
        } else if (errno == EACCES) {
            // no permissions
            dprintf(
                req->in_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
            fprintf(stderr, "%s,%s,403,%d", req->method, req->uri, req->request_id);
        } else {
            dprintf(req->in_fd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
            fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
        }
        return EXIT_FAILURE;
    }

    // get file size/info to read from
    struct stat file_info;
    fstat(fd, &file_info);
    off_t file_size = file_info.st_size;

    char buffer[4096] = { '\0' };
    dprintf(req->in_fd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_size);
    fprintf(stderr, "%s,%s,200,%d", req->method, req->uri, req->request_id);

    int n;
    int written_bytes = 0;
    while ((n = read(fd, &buffer, sizeof(buffer))) > 0) {
        written_bytes += write(req->in_fd, &buffer, n);
    }

    if (written_bytes == -1) {
        dprintf(req->in_fd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
        return EXIT_FAILURE;
    }

    /* close descriptor for file that was sent */
    close(fd);
    //printf("\nEnd of method_get");
    return EXIT_SUCCESS;
}

// method_put
int method_put(Request *req) {
    //printf("\nStart of method_put");
    if (req->content_length == -1) {
        dprintf(
            req->in_fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        fprintf(stderr, "%s,%s,400,%d", req->method, req->uri, req->request_id);
        return EXIT_FAILURE;
    }

    char *fileToWrite = req->uri + 1;

    int fd;
    int status_code = 0;
    // check for directory
    if ((fd = open(fileToWrite, O_RDWR)) == -1 && errno == EISDIR) {
        dprintf(req->in_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        fprintf(stderr, "%s,%s,403,%d", req->method, req->uri, req->request_id);
        return EXIT_FAILURE;
    }

    // check if file exists -- create if not existing
    if ((fd = open(fileToWrite, O_WRONLY | O_CREAT | O_EXCL, 0666)) == -1) {
        if (errno == EEXIST) {
            // file exists
            status_code = 200;
        } else if (errno == EACCES) {
            dprintf(
                req->in_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
            fprintf(stderr, "%s,%s,403,%d", req->method, req->uri, req->request_id);
            return EXIT_FAILURE;
        } else {
            dprintf(req->in_fd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
            fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
            return EXIT_FAILURE;
        }
    } else if (fd != -1) {
        // created
        status_code = 201;
    }

    // file exists already
    if (status_code == 200) {
        if ((fd = open(fileToWrite, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
            if (errno == EACCES) {
                // permission denied
                dprintf(req->in_fd,
                    "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
                fprintf(stderr, "%s,%s,403,%d", req->method, req->uri, req->request_id);
                return EXIT_FAILURE;
            } else {
                // file function issue
                dprintf(req->in_fd,
                    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal "
                    "Server Error\n",
                    22);
                fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
                return EXIT_FAILURE;
            }
        }
    }

    // write to file
    int written_bytes = write(fd, req->message_body, req->bytes_remaining);
    if (written_bytes == -1) {
        dprintf(req->in_fd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
        return EXIT_FAILURE;
    }

    int remaining_message_bodySize = req->content_length - req->bytes_remaining;

    // If there is extra message body than initial read
    if (remaining_message_bodySize > 0) {
        int n;
        char buffer[4096] = { '\0' };
        written_bytes = 0;

        while (
            remaining_message_bodySize > 0 && (n = read(req->in_fd, &buffer, sizeof(buffer))) > 0) {
            written_bytes += write(fd, &buffer, n);
            remaining_message_bodySize -= n;
        }

        if (written_bytes == -1) {
            dprintf(req->in_fd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
            fprintf(stderr, "%s,%s,500,%d", req->method, req->uri, req->request_id);
            return EXIT_FAILURE;
        }
    }

    // output status codes with response
    if (status_code == 201) {
        dprintf(req->in_fd, "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\nCreated\n", 8);
        fprintf(stderr, "%s,%s,201,%d", req->method, req->uri, req->request_id);
    } else {
        dprintf(req->in_fd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nOK\n", 3);
        fprintf(stderr, "%s,%s,200,%d", req->method, req->uri, req->request_id);
    }

    // finish and close
    close(fd);
    //printf("\nEnd of method_put");
    return EXIT_SUCCESS;
}
