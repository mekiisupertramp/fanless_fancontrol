#ifndef GPIOS_H
#define GPIOS_H

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

#define XSTR(x)
#define STR(x) XSTR(x)  #x

#define GPIO_EXPORT     "/sys/class/gpio/export"
#define GPIO_UNEXPORT   "/sys/class/gpio/unexport"

#define GPIO0       0
#define GPIO2       2
#define GPIO3       3

#define DIRECTION   "/direction"
#define VALUE       "/value"
#define EDGE        "/edge"
#define SYS_GPIOS   "/sys/class/gpio/gpio"

#define GPIOS(ingpio,operation) (SYS_GPIOS STR(ingpio) "/" operation)
#define GTOSTR(x)(STR(x))

#define GSTR    4

typedef struct{
	int k1;
	int k2;
	int k3;
	int pwr;
}str_fd_gpios;

int init_gpio(int gpio, char *dir, char *edge);
int dele_gpio(int number);
int writ_gpio(int fd_gpio, int val);
int read_gpio(int fd_gpio);
int init_gpios(str_fd_gpios* fd_gpios);
void del_gpios();

#endif
