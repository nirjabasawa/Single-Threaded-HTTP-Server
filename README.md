# Single-Threaded-HTTP-Server
creates a simple single-threaded HTTP server that handles basic GET and PUT requests from the client. 

Files:
- request_parse.h -- header file for request_parse
- request_parse.c -- source code file, implements parsing of HTTP request received by server from client
- httpserver.c -- source code file (main file), implements infinite loop of accepting requests from client, processes bytes sent by client using HTTP protocol
- Makefile -- compiles httpserver and request_parse

completed May 7, 2024
