CC=clang
CFLAGS= -Wall -Werror -Wextra -pedantic
LDFLAGS = -pthread

all: httpserver httpserver.o

httpserver: httpserver.o
	$(CC) -g $(CFLAGS) $(LDFLAGS) -o httpserver httpserver.o

httpserver.o: httpserver.c
	$(CC) -g $(CFLAGS) $(LDFLAGS) -c httpserver.c
	
clean:
	rm -f *.o
	rm httpserver