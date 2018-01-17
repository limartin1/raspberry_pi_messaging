#define _POSIX_C_SOURCE 201112L

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp.h"

#define ERROR_IF(F) if(F) { perror(#F); return -1; }

#define	ADDRESS "172.16.32.27"
#define PORT "1337"

int tcp_connect(char *addr, char *port) {
	struct addrinfo hints, *res;
	int fd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ERROR_IF(getaddrinfo(addr, port, &hints, &res) != 0);
	ERROR_IF((fd = socket(res->ai_family, res->ai_socktype, 0)) == -1);
	ERROR_IF(connect(fd, res->ai_addr, res->ai_addrlen) == -1);

	return fd;
}
