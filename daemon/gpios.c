#include "gpios.h"

/*
* This function init a gpio and return a file
* descriptor to the /sys/class/gpio/gpiox/value
* gpio: the gpio number
* dir:  "in" or "out" or "both"
* edge: "falling" or "rising" or "both" if in
*
* return -1 if error, fd instead
*/
int init_gpio(int gpio, char *dir, char *edge){
    dele_gpio(gpio);

    // export gpio
    char path[100];
    char strgpio[GSTR];
    int fd=open(GPIO_EXPORT,O_WRONLY);
    if(fd==-1) return log_error("init_gpios open gpio for export");
    if(sprintf(strgpio,"%d",gpio)<0) return log_error("init_gpios first sprintf");
    if(write(fd,strgpio,strlen(strgpio))==-1)return log_error("init_gpios first write");
    if(close(fd)==-1)return log_error("init_gpios first close");

    // set direction
    if(sprintf(path,"%s",SYS_GPIOS)<0)return log_error("init_gpios sprintf");
    strcat(path,strgpio);
    strcat(path,DIRECTION);
    fd=open(path,O_WRONLY);
    if(fd==-1) return log_error("init_gpios second open");
    if(write(fd,dir,strlen(dir))==-1)return log_error("init_gpios second write");
    if(close(fd)==-1)return log_error("init_gpios second close");

    // set edge if in
    if(strcmp(dir,"in")==0){
        if(sprintf(path,"%s",SYS_GPIOS)<0)return log_error("init_gpios sprintf");
        strcat(path,strgpio);
        strcat(path,EDGE);
        fd=open(path,O_WRONLY);
        if(fd==-1) return log_error("init_gpios third open");
        if(write(fd,edge,strlen(edge))==-1)return log_error("init_gpios third write");
        if(close(fd)==-1)return log_error("init_gpios third close");
    }

    // return the file descriptor from /sys/class/gpio/gpiox/value
    if(sprintf(path,"%s",SYS_GPIOS)<0)return log_error("init_gpios sprintf");
    strcat(path,strgpio);
    strcat(path,VALUE);
    if(strcmp(dir,"in")==0){
        fd=open(path,O_RDONLY);
    }else{
        fd=open(path,O_WRONLY);
    }
    if(fd==-1) return log_error("init_gpios last open");
    return fd;
}
/*
* This function unexport a gpio
* number: gpio number
*
* return 0 if succeed, -1 if not
*/
int dele_gpio(int number){
    char strgpio[GSTR];
    int fd=open(GPIO_UNEXPORT,O_WRONLY);
    sprintf(strgpio,"%d",number);
    if(fd==-1) return log_error("dele_gpio open");
    if(write(fd,strgpio,strlen(strgpio))==-1)return log_error("dele_gpio write");
    if(close(fd)==-1)return log_error("dele_gpio close");
    return 0;
}
/*
* This function write a value to a gpio.
* fd_gpio: the file descriptor returned by init_gpio
* val: 1 or 0
*
* return 0 if succeed, -1 if not
*/
int writ_gpio(int fd_gpio,int val){
    char strgpio[GSTR];
    sprintf(strgpio,"%d",val);
    if(write(fd_gpio,strgpio,strlen(strgpio))==-1)return log_error("writ_gpio write");
    return 0;
}
/*
* This function read a gpio.
* fd_gpio: the file descriptor returned by init_gpio
*
* return 0 or 1 if succeed,  or -1 if error
*/
int read_gpio(int fd_gpio){
    char strgpio[2];
    if(pread(fd_gpio,strgpio,2,0)==-1)return log_error("read_gpio pread");
    if(atoi(strgpio)){
        return 1;
    }
    return 0;
}
/*
* This function init all gpios needed for this project.
* fd_gpios: the structure of the gpios needed for this project 
*
* return 0 if succeed, -1 if not
*/
int init_gpios(str_fd_gpios* fd_gpios){
	fd_gpios->k1=init_gpio(0,"in","both");
	if(fd_gpios->k1==-1)return log_error("init_gpio 0");
	fd_gpios->k2=init_gpio(2,"in","both");
	if(fd_gpios->k2==-1)return log_error("init_gpio 2");
	fd_gpios->k3=init_gpio(3,"in","both");
	if(fd_gpios->k3==-1)return log_error("init_gpio 3");
	fd_gpios->pwr=init_gpio(362,"out","");
	if(fd_gpios->pwr==-1)return log_error("init_gpio 362");
	return 0;
}
/*
* Remove all the GPIOS, there is no return value
* because only called at the end of daemon.
* (Maybe not the best way..)
*/
void del_gpios(){
	dele_gpio(0);
	dele_gpio(2);
	dele_gpio(3);
	dele_gpio(362);
}
