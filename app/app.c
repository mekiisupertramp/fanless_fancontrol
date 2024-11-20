#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#define FIPATHIN  "/tmp/my_daemon.input.fifo"
#define MAX     5

#define FIPATHOUT  "/tmp/my_daemon.output.fifo"
#define MAXFIFOUT   20

#define COMMANDSSTR "\n\
Please use folowing commands: \n\
	<auto> : set automatic mode (default) \n\
	<man>  : set manual mode (only for expert) \n\
	<inc>  : increase fan speed in man mode \n\
	<dec>  : decrease fan speed in man mode \n\
	<temp> : get temp of CPU NOT IMPLEMENTED YET \n\
	<fan>  : get fan speed (pwm) NOT IMPLEMENTED YET \n\
	<exit> : exit properly this programms\n\
	<kill> : kill the bloody daemon \n\
	<help> : show this dialog\n"



char finito;
struct sigaction act;

void catch_signals(int signo){
	if(signo==SIGINT ||
	signo==SIGQUIT){
		printf("\nyou killer!\n");
		finito=0;
	}
}

int main(){
	char buff[MAX];
	int fd_fifin=open(FIPATHIN,O_WRONLY); // make sure fifos exist! it will block if not
	if(fd_fifin==-1){
		printf("sorry, the folowing error occured: %s\n",strerror(errno));
		return -1;
	}
	int fd_fifout=open(FIPATHOUT,O_RDONLY);
	if(fd_fifout==-1){
		printf("sorry, the folowing error occured: %s\n",strerror(errno));
		return -1;
	}

	printf("\nProgramm -App-\n");
	printf("Managing some stuff\n");
	printf("Created by Mehmed Blazevic\n");
	printf("HES-SO MSE \n");
	printf("%s\n",COMMANDSSTR);

	act.sa_handler=catch_signals;
	sigaction(SIGINT,&act,NULL);
	sigaction(SIGQUIT,&act,NULL);

	finito=1;
	do{
		printf(">");
		fgets(buff,MAX,stdin);

		if(strcmp(buff,"\n")!=0){
			// align data for transfert (always 4 chars (+'0')), because
			// FIFO is bytestream (not datas stream). The daemon always read 5 bytes
			for(int i=0;i<MAX-1;i++){
				if((buff[i]<'a')||(buff[i]>'z')){
					buff[i]=' ';
				}
			}
			buff[4]=0;
				// simple command
			if((strcmp(buff,"auto")==0)||(strcmp(buff,"man ")==0)||
				(strcmp(buff,"inc ")==0)||(strcmp(buff,"dec ")==0)||
				(strcmp(buff,"kill")==0)){
				if(write(fd_fifin,buff,MAX)==-1){printf("error with writin in the fifo\n"); return -1;}
				// read temp
			}else if(strcmp(buff,"temp")==0){
				if(write(fd_fifin,buff,MAX)==-1){printf("error with writin in the fifo %d\n",errno); return -1;}
				if(read(fd_fifout,buff,MAXFIFOUT)==-1){printf("error with writin in the fifo\n"); return -1;}
				printf("\ntemp: %s °C\n",buff);
				// read fan speed
			}else if(strcmp(buff,"fan ")==0){
				if(write(fd_fifin,buff,MAX)==-1){printf("error with writin in the fifo\n"); return -1;}
				if(read(fd_fifout,buff,MAXFIFOUT)==-1){printf("error with writin in the fifo\n"); return -1;}
				printf("\nperiod: %s ms\n",buff);
				// exit the programm
			}else if(strcmp(buff,"exit")==0){
				finito=0;
				// show help
			}else if(strcmp(buff,"help")==0){
				printf("%s\n",COMMANDSSTR);
			}else{
				printf("wrong command you fool!\n\n");
			}
		}
		memset(buff,0,MAX);
	}while(finito);

	close(fd_fifin);
	printf("\nSee you soon!\n");
	return 0;
}
