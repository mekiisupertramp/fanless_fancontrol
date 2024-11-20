#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#define PATHMODE "/sys/devices/platform/skeleton/mode"
#define PATHOPER "/sys/devices/platform/skeleton/op"
#define PATHPERI "/sys/devices/platform/skeleton/period"
#define PATHTEMP "/sys/class/thermal/thermal_zone0/temp"

typedef struct{
	int mod;
	int op;
	int period;
	int temp;
}str_fd_driver;

enum modes {AUTO,MANUAL};
extern enum modes mode;

int init_driver();
void fan_speed(int val);
void change_mode(enum modes mode);
int get_temp();
void get_temp2(char *buf);
int get_fan();
void get_fan2(char *buf);
void close_driver();

#endif
