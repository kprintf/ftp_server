#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

static FILE *err;
/*static int initializer_pid;*/

static inline void log_send(char *type, char *fmt, ...)
{
	char t_name[20];
	va_list args;
	va_start(args, fmt);
	thread_get_name(t_name);
	fprintf(err,"[%16s:%s] ", t_name, type);
	vfprintf(err,fmt, args);
	fprintf(err,"\n");
	va_end(args);
#ifndef NO_STDOUT_LOGS
	printf("[%16s:%s] ", t_name, type);
	va_start(args, fmt);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
#endif
	fflush(err);
}

//void log_close(void);

void log_init(void)
{
	err = fopen("ftpmod.log", "w");
}

void log_close(void)
{
		fclose(err);
}

void log_info(char *msg)
{
	log_send("INFO", "%s", msg);
}

void log_infos(char *msg, ...)
{
	va_list args;
	char buf[256];
	va_start(args, msg);
	vsnprintf(buf, 256, msg, args);
	va_end(args);
	log_send("INFO", "%s", buf);
}

void log_warn(char *msg)
{
	log_send("WARN", "%s", msg);
}

void log_warns(char *msg, ...)
{
	va_list args;
	char buf[256];
	va_start(args, msg);
	vsnprintf(buf, 256, msg, args);
	va_end(args);
	log_send("WARN", "%s", buf);
}
void log_error(char *msg)
{
	log_send("ERR!", "%s", msg);
}

void log_errors(char *msg, ...)
{
	va_list args;
	char buf[256];
	va_start(args, msg);
	vsnprintf(buf, 256, msg, args);
	va_end(args);
	log_send("ERR!", "%s", buf);
}
// vim: noai:ts=8:sw=8
