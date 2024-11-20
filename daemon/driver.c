#include "driver.h"

enum modes mode = AUTO;           // the module mode
static str_fd_driver fd_driver;   // the secret structure

/*
* This function simply init the module by opening the correct files and
* getting the file descriptors for each file.
*
* return 0 if succeed and -1 if not, log the error
*/
int init_driver(){
    fd_driver.mod=open(PATHMODE,O_WRONLY);
    if(fd_driver.mod==-1)return log_error("init_driver open mode");
    fd_driver.op=open(PATHOPER,O_WRONLY);
    if(fd_driver.op==-1)return log_error("init_driver open op");
    fd_driver.period=open(PATHPERI,O_RDONLY);
    if(fd_driver.period==-1)return log_error("init_driver open period");
    fd_driver.temp=open(PATHTEMP,O_RDONLY);
    if(fd_driver.temp==-1)return log_error("init_driver open temp");
    // change_mode(AUTO); // set initial mode
    return 0;
}
/*
* This function increase (val=1) or decrease (val=0) the fan speed (led blink)
*/
void fan_speed(int val){
    if(mode==MANUAL){
        char buf[2];
        if(val){
            strcpy(buf,"1");
        }else{
            strcpy(buf,"0");
        }
        if(write(fd_driver.op,buf,2)==-1)log_error("fan_speed write");
    }
}
/*
* This function change the mode of the driver
*/
void change_mode(enum modes mode){
    char buf[2];
	if(mode==AUTO){
        strcpy(buf,"1");
        if(write(fd_driver.mod,buf,2)==-1)log_error("change_mode write");
    }else{
        strcpy(buf,"0");
        if(write(fd_driver.mod,buf,2)==-1)log_error("change_mode write");
    }
}
/*
* This function read the temp from /sys/class/thermal/thermal_zone0/temp.
*
* return the temp value
*/
int get_temp(){
    char buf[7];
    if(pread(fd_driver.temp,buf,20,0)==-1)log_error("get_temp read");
    return atoi(buf);
}
/*
* This function return the temp value directly in the buf variable.
*/
void get_temp2(char *buf){
    if(pread(fd_driver.temp,buf,20,0)==-1)log_error("get_temp read");
}
/*
* This function get the fan speed (aka the led blink periode).
*
* return the periode value
*/
int get_fan(){
    char buf[7];
    if(pread(fd_driver.period,buf,20,0)==-1)log_error("get_fan read");
    return atoi(buf);
}
/*
* This function return the fan speed (aka the led blink period) directly
* in the buf variable.
*/
void get_fan2(char *buf){
    if(pread(fd_driver.period,buf,20,0)==-1)log_error("get_fan read");
}
/*
* This function simple close the driver.
*/
void close_driver(){
    if(close(fd_driver.mod)==-1)log_error("init_driver close mode");
    if(close(fd_driver.op)==-1)log_error("init_driver close op");
    if(close(fd_driver.period)==-1)log_error("init_driver close period");
    if(close(fd_driver.temp)==-1)log_error("init_driver close temp");
}
