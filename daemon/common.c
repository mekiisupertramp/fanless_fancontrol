#include "common.h"

/*
* This function log an error and return -1
*/
int log_error(char* str){
	syslog(LOG_NOTICE,"The folowing error occured: %s at %s\n",strerror(errno),str);
	return -1;
}