CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic
OBJS = httpserver.o request_parse.o

all: httpserver

httpserver: $(OBJS)
	$(CC) -o httpserver $(OBJS)

httpserver.o: httpserver.c
	$(CC) $(CFLAGS) -c httpserver.c

request_parse.o:
	$(CC) $(CFLAGS) -c request_parse.c

clean:
	rm -f httpserver httpserver.o global.o request_parse.o
