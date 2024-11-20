/* -----------------------------------------------------------------------
*	File: my_daemon.c
*	Description: This programm create a daemon which manage a kernel module.
* Author:	Mehmed Blazevic
* Date: January 2019
* Comments: HES-SO MSE TIC
* -----------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "ssd1306.h"
#include "gpios.h"
#include "driver.h"
#include "common.h"


#define FIPATHIN  "/tmp/my_daemon.input.fifo"
#define MAX     5

#define FIPATHOUT  "/tmp/my_daemon.output.fifo"
#define MAXFIFOUT   20

#define NBR_D	3

typedef struct{
	int in;
	int out;
}str_fd_fifos;

typedef struct{
	struct epoll_event eventK1;
	struct epoll_event eventK2;
	struct epoll_event eventK3;
	struct epoll_event eventComm;
}str_events;


int finished=1;


/* functions */
int init_daemon(struct sigaction* act);
int init_epolls(int epfd,str_fd_gpios* fd_gpios,str_fd_fifos* fd_fifos,str_events* my_events);
int init_gpios(str_fd_gpios* fd_gpios);
int init_fifos(str_fd_fifos* fd_fifos);
void del_fifos(str_fd_fifos* fd_fifos);
void app_parser(str_fd_fifos *fd_fifos,char *cmd);	// parse commands
void catch_signals(int signo);
void *th_screen(); // this thread read the freq & the temp and update the screen

int main(){

	str_fd_fifos fd_fifos;	// structure of file descriptors (fifos)
	str_fd_gpios fd_gpios;	// structure of file descriptors (gpios)
	str_events my_events;
	int epfd;
	struct epoll_event events[NBR_D];
	pthread_t th;
	struct sigaction act;

	// init everything
	if(init_daemon(&act)!=0){
		syslog(LOG_NOTICE,"The folowing error occured: %s at the daemon init function\n",strerror(errno));return -1;}

	if(init_gpios(&fd_gpios)==-1)return log_error("init_gpios");

	if(init_driver()==-1)return log_error("init_driver");

	if(pthread_create(&th,NULL,th_screen,NULL)==-1)log_error("pthread_create");

	if(init_fifos(&fd_fifos)==-1)return log_error("init_fifos");

	epfd=epoll_create1(0);
	if(epfd==-1)return log_error("epoll_create1(0)");
	if(init_epolls(epfd,&fd_gpios,&fd_fifos,&my_events)==-1)return log_error("init_epolls");

	// the core: check file descriptors gpios k1, k2 & k3 AND the input fifo
	char buff[MAX];
	do{
		int nr=epoll_wait(epfd,events,NBR_D,-1);
		if(nr==-1){
			if(finished){return log_error("epoll_wait");}
			else {if(errno==EINTR)errno=0;}
		}
		for(int i=0;i<nr;i++){
			if(events[i].data.fd == fd_gpios.k1) {
				// fan speed ++
				if(read_gpio(fd_gpios.k1)==1){
					writ_gpio(fd_gpios.pwr,1);
					fan_speed(1);
				}else if(read_gpio(fd_gpios.k1)==0){
					writ_gpio(fd_gpios.pwr,0);
				}
			} if(events[i].data.fd == fd_gpios.k2) {
				// fan speed --
				if(read_gpio(fd_gpios.k2)==1){
					writ_gpio(fd_gpios.pwr,1);
					fan_speed(0);
				}else if(read_gpio(fd_gpios.k2)==0){
					writ_gpio(fd_gpios.pwr,0);
				}
			} if (events[i].data.fd == fd_gpios.k3) {
				// change mode
				if(read_gpio(fd_gpios.k3)==1){
					if(mode==AUTO){
						mode=MANUAL;
					}else {
						mode=AUTO;
					}
					change_mode(mode);
				}
			} else if(events[i].data.fd == fd_fifos.in){
				// inputs from fifo
				memset(buff,0,MAX);
				if(read(fd_fifos.in, buff, MAX)==-1)return log_error("input fifo read");
				app_parser(&fd_fifos,buff);
			}
		}
	}while(finished);

	// end
	closelog();
	del_fifos(&fd_fifos);
	del_gpios();
	if(pthread_join(th,NULL)!=0)return log_error("pthread_join");
	close_driver();
	return 0;
}
/*
* Very complicated function to create a daemon
*/
int init_daemon(struct sigaction* act){
	int spid;
	int pid;

	/* création du nouveau process */
	pid = fork();
	if(pid == 0){
		printf("i'm the father, pid: %d\n",getpid());
	}else if(pid>0){
		printf("i'm the grand-father, pid: %d\n",getpid());
		exit(0);
	}else{
		printf("Error fork!\n");
		exit(-1);
	}
	/* création de la nouvelle sessions */
	spid=setsid();
	if(spid==-1){printf("error setsid\n");exit(-1);}
	printf("spid: %d\n",spid);

	/* création du process deamon */
	pid = fork();
	if(pid == 0){
		pid=getpid();
		printf("i'm the son, pid: %d\n",pid);
	}else if(pid>0){
		printf("i'm the father again, pid: %d\n",getpid());
		exit(0);
	}else{
		printf("Error fork!\n");
		exit(-1);
	}

	/* capture des signaux souhaités */
	act->sa_handler=catch_signals;
	if(sigaction(SIGINT,act,NULL)==-1){printf("error sigaction\n");exit(-1);}
	if(sigaction(SIGQUIT,act,NULL)==-1){printf("error sigaction\n");exit(-1);}
	if(sigaction(SIGTERM,act,NULL)==-1){printf("error sigaction\n");exit(-1);}

	/* maj masque pour la création de fichiers */
	umask(027);
	if(chdir("/")==-1){printf("error chdir\n");exit(-1);}

	/* fermer tous les descripteurs de fichiers */
	if(close(STDIN_FILENO)==-1){printf("error close STDIN_FILENO\n");exit(-1);}
    if(close(STDOUT_FILENO)==-1){printf("error close STDOUT_FILENO\n");exit(-1);}
    if(close(STDERR_FILENO)==-1){printf("error close STDERR_FILENO\n");exit(-1);}

	/* redirection des entrées/sorties standard */
	if(dup2(open("/dev/null",0),STDIN_FILENO)==-1){printf("error dup2 STDIN_FILENO\n");exit(-1);}
	if(dup2(open("/dev/null",0),STDOUT_FILENO)==-1){printf("error dup2 STDOUT_FILENO\n");exit(-1);}
	if(dup2(open("/dev/null",0),STDERR_FILENO)==-1){printf("error dup2 STDOUT_FILENO\n");exit(-1);}

	/* allow log performing */
	openlog(NULL,LOG_PID,LOG_DAEMON);
	syslog(LOG_NOTICE,"my_daemon ready!\n"); // can be accessed through /var/log/messages

	return 0;
}
/*
* This function init an epoll context and configure the events.
* epfd: the poll context
* fd_gpios:  gpios file descriptors structure
* fd_fifos:  fifos file descriptors structure
* my_events: events structure
*
* return 0 if succeed and -1 if not
*/
int init_epolls(int epfd,str_fd_gpios* fd_gpios,str_fd_fifos* fd_fifos,str_events* my_events){

	char buf[20];
	pread(fd_gpios->k1, buf, 20, 0);
	pread(fd_gpios->k2, buf, 20, 0);
	pread(fd_gpios->k3, buf, 20, 0);
	my_events->eventK1.events=EPOLLERR;
	my_events->eventK1.data.fd=fd_gpios->k1;
	my_events->eventK2.events=EPOLLERR;
	my_events->eventK2.data.fd=fd_gpios->k2;
	my_events->eventK3.events=EPOLLERR;
	my_events->eventK3.data.fd=fd_gpios->k3;
	my_events->eventComm.events=EPOLLIN|EPOLLET;
	my_events->eventComm.data.fd=fd_fifos->in;
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd_gpios->k1,&my_events->eventK1)==-1)return log_error("epoll_ctl k1");
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd_gpios->k2,&my_events->eventK2)==-1)return log_error("epoll_ctl k2");
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd_gpios->k3,&my_events->eventK3)==-1)return log_error("epoll_ctl k3");
	if(epoll_ctl(epfd,EPOLL_CTL_ADD,fd_fifos->in,&my_events->eventComm)==-1)return log_error("epoll_ctl eventComm");
	return 0;
}
/*
*	This function init the input and the output fifo.
* fd_fifos: fifos file descriptors structure
*
* return 0 if succeed and -1 if not
*/
int init_fifos(str_fd_fifos* fd_fifos){
	unlink(FIPATHIN); // just in case
	unlink(FIPATHOUT);
	errno=0;

	if(mkfifo(FIPATHIN, S_IFIFO)==-1)return log_error("mkfifo (input fifo)");
	fd_fifos->in=open(FIPATHIN,O_RDWR);
	if(fd_fifos->in==-1)return log_error("opening the input fifo");

	if(mkfifo(FIPATHOUT, S_IFIFO)==-1)return log_error("mkfifo (output fifo)");
	fd_fifos->out=open(FIPATHOUT,O_RDWR);
	if(fd_fifos->out==-1)return log_error("opening the output fifo");

	return 0;
}
/*
* Remove & unlink all the fifos at the end of daemon.
* fd_fifos: fifos file descriptors structure
*/
void del_fifos(str_fd_fifos* fd_fifos){
	close(fd_fifos->in);
	unlink(FIPATHIN);
	close(fd_fifos->out);
	unlink(FIPATHOUT);
}
/*
*	This function parse commands received from the input fifo (e.g. the app)
* fd_fifos: fifos file descriptors structure
* cmd: the command received
*/
void app_parser(str_fd_fifos *fd_fifos,char *cmd){
	if((strcmp(cmd,"auto")==0)){
		mode=AUTO;
		change_mode(mode);
	}else if(strcmp(cmd,"man ")==0){
		mode=MANUAL;
		change_mode(mode);
	}else if(strcmp(cmd,"inc ")==0){
		fan_speed(1);
	}else if(strcmp(cmd,"dec ")==0){
		fan_speed(0);
	}else if(strcmp(cmd,"kill")==0){
		finished=0;
	}else if(strcmp(cmd,"temp")==0){
		char buff[MAXFIFOUT];
		memset(buff,0,MAXFIFOUT);
		get_temp2(buff);
		double val=atof(buff);
		val=val/1000;
		sprintf(buff,"%.3f",val);
		if(write(fd_fifos->out,buff,strlen(buff))==-1) log_error("get_temp2");
	}else if(strcmp(cmd,"fan ")==0){
		char buff[MAXFIFOUT];
		memset(buff,0,MAXFIFOUT);
		get_fan2(buff);
		if(write(fd_fifos->out,buff,strlen(buff))==-1) log_error("get_fan2");
	}
}
/*
*	Catch signal for finish properly the daemon
* signo: the signal number
*/
void catch_signals(int signo){
	if(signo==SIGINT || signo==SIGQUIT ||
	signo==SIGTERM){
		finished=0;
	}
}
/*
*	This thread read the temp & the led period and update the LCD screen every
* second
*/
void *th_screen(){
	char buff[MAXFIFOUT];
	char strTemp[MAXFIFOUT];
	char strFan[MAXFIFOUT];
	double val;
	ssd1306_init();
    ssd1306_set_position (0,0);
    ssd1306_puts("Master MSE");
    ssd1306_set_position (0,6);
    ssd1306_puts("Mehmed Blazevic");

	memset(strTemp,0,MAXFIFOUT);
	memset(buff,0,MAXFIFOUT);
	strcat(strTemp,"Temp: ");
	get_temp2(buff);
	val=atof(buff);
	val=val/1000;
	sprintf(buff,"%.2f",val);
	strcat(strTemp,buff);
	strcat(strTemp," °C");
	ssd1306_set_position (0,1);
	ssd1306_puts(strTemp);

	memset(strFan,0,MAXFIFOUT);
	memset(buff,0,MAXFIFOUT);
	get_fan2(buff);
	strcat(strFan,"Peri: ");
	strcat(strFan,buff);
	strcat(strFan," ms");
	ssd1306_set_position (0,2);
	ssd1306_puts(strFan);


	while(finished){
		sleep(1);
		memset(strTemp,0,MAXFIFOUT);
		memset(buff,0,MAXFIFOUT);
		strcat(strTemp,"Temp: ");
		get_temp2(buff);
		val=atof(buff);
		val=val/1000;
		sprintf(buff,"%.2f",val);
		strcat(strTemp,buff);
		strcat(strTemp," C");
		ssd1306_set_position (0,1);
		ssd1306_puts("                ");
		ssd1306_set_position (0,1);
		ssd1306_puts(strTemp);

		memset(strFan,0,MAXFIFOUT);
		memset(buff,0,MAXFIFOUT);
		get_fan2(buff);
		strcat(strFan,"Period: ");
		strcat(strFan,buff);
		strcat(strFan," ms");
		ssd1306_set_position (0,2);
		ssd1306_puts("                ");
		ssd1306_set_position (0,2);
		ssd1306_puts(strFan);

		if(mode==AUTO){
			ssd1306_set_position (0,3);
			ssd1306_puts("mode auto");
		}else{
			ssd1306_set_position (0,3);
			ssd1306_puts("mode manu");
		}
	}

	ssd1306_clear_display();
	ssd1306_set_position (0,5);
	ssd1306_puts("Bye Bye!");

	return 0;
}
