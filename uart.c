#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "uart.h"

int uart_open(char const*filename) {
	int speed = B115200;

	struct termios options;
	int fd = open(filename, O_RDWR|O_NONBLOCK);
	if(fd < 0) return fd;

	fcntl(fd, F_SETFL, 0);
	tcgetattr(fd, &options);
	usleep(10000);
	cfsetospeed(&options, speed);
	cfsetispeed(&options, speed);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	options.c_oflag &= ~CRTSCTS;
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON|ECHO|ECHONL|IEXTEN|ISIG);

	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 40;

	tcsetattr(fd, TCSANOW, &options);
	tcflush(fd, TCIOFLUSH);
	usleep(10000);

	return fd;
}
