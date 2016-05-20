#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "log.h"
#include "server.h"
#include "common.h"
#include "configuration.h"
#include "auth.h"

#define HELP_MSG	"ftpMod is not completed implementation of ftp server.\n"\
			"Usage:\n"\
			"\t-- end of arguments\n"\
			"\t-c point configuration file\n"\
			"\t-s point IPC socket path for configuration (only if build with interactive mode)\n"\
			"\t-h show this help\n"

static void parse_args(int argc, char **argv)
{
	int i = 1;
	while(i < argc)
	{
		if(argv[i][0]=='-')
		{
			if(argv[i][1]=='-')
				return;
		else	if(argv[i][1]=='c')
				cfg_set_config_path(argv[i+=2]);
		else	if(argv[i][1]=='s')
				cfg_set_config_sock(argv[i+=2]);
		else	if(argv[i][1]=='h')
				printf(HELP_MSG), i++, exit(0);
		}
	}
}

int main(int argc, char **argv)
{
	pid_t pid;
	int status;
	
	parse_args(argc, argv);

	log_init();
	cfg_init();
	auth_init();
#ifdef INTERACTIVE_MODE
	thread_name("ftp CLI");
	switch(pid=fork())
	{
		case -1:
			log_error("fork() failed");
			exit(1);
		case 0:
#endif
			thread_name("ftp server");
			server_main();
#ifdef INTERACTIVE_MODE
			break;
		default:
			usleep(30); //wait for main thread initialization
			log_infos("server thread ID: %i\n", pid);
			server_cli(pid);
			wait(&status);
			break;
	}
	//cfg_exit();
	if(pid==0)
		kill(0, SIGKILL);
	else 
		wait(&status);
#endif
	return 0;
}
// vim: noai:ts=8:sw=8
