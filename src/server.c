#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "configuration.h"
#include "log.h"
#include "ftp_codes.h"
#include "auth.h"
#include "common.h"
/*TODO: controlling sessions count and collecting their PIDs*/

#define MAX_DATA_BUF 4096

typedef struct
{
	char		*in_buf,
			*cmd,
			*arg,
			*svptr, 
			*name,
			*pswd,
			*base_dir, 
			*frepr,
			*home_dir;
	int 		 logged,
				 quit;
	uint32_t	 data_ip;
	uint16_t 	 data_port;
	int 		 data_socket;
	int			 control_socket;
	char 		 fstruct;
	char 		 ftrans;
	int  		 representation_local_bytes;
	int 		 connection_socket;
	int 		 connection_pid;
	char		*rnfr; /*we need to save it here :<*/
} session_t;

void session_init(session_t *s)
{
	s->in_buf = malloc(300);
	s->logged = s->quit = 0;
	s->fstruct = 'F';
	s->ftrans = 'S';
	s->base_dir = strdup("/");
	s->home_dir = strdup("/");
	s->connection_socket = s->data_socket = -1;
	s->frepr = strdup("A N             ");
}

void session_clean(session_t *s)
{
	s->in_buf = mfree(s->in_buf);
	s->frepr = mfree(s->frepr);
	s->name = mfree(s->name);
	s->pswd = mfree(s->pswd);
	s->base_dir = mfree(s->base_dir);
	s->home_dir = mfree(s->home_dir);
	/*TODO: closing sockets and transmission threads if alive*/
}

typedef struct
{
	char command[13];
	uint8_t flags;
#define OP_FLAG_LOGIN		0x01 /*User must be logged in for doing that command*/
#define OP_FLAG_ARGUMENT	0x02 /*Command must have argument*/
#define OP_FLAG_FLEXISTS	0x04 /*...and this argument is really existing file*/
#define OP_FLAG_FLCREATE	0x08 /*...or user must have perms for create it*/
	int (*callback) (session_t*);
} op_t;


void save_trans_pid(char *name, int pid)
{
	char buf[200] /*, str[200]*/;

	sprintf(buf, "transmission[%s]%c", name, 0);
	/*sprintf(str, "%i%c", socket, 0); ??? */
	printf("WRITE: %s\n", buf);

	cfg_write_int(buf, pid);
}

void kill_trans(char *name)
{
	char buf[200];
	int id;

	sprintf(buf, "transmission[%s]", name);

	id=cfg_read_int(buf, -1);

	if(id!=-1)
	{
		close(id);
	}
}

void server_data_send(int sck, FILE *fp, char *repr, char trans)
/*TODO: дописать compressed блджад*/
{
	char *buf;
	uint16_t sz;
	uint8_t code;
	int traffic_used = 0;

	buf = malloc(MAX_DATA_BUF+3);

	switch(trans)
	{
		case 'B':
			while(!feof(fp))
			{
				code = 0;
				sz = fread(&buf[3], sizeof(uint8_t), MAX_DATA_BUF, fp);
				if(sz < MAX_DATA_BUF)
				{
					if(feof(fp))
						code |= FTP_B_EOF;
					else
						code |= FTP_B_ERR;
				}
				*buf = code;
				memcpy(&buf[1], &sz, 2);
	
				if(write(sck, buf, sz+3)==-1)
					goto __exit;

				traffic_used += 3 + sz;
			}
			break;
		case 'C':
			break;
		case 'S':
		default:
			while(!feof(fp))
			{
				sz = fread(buf, 1, MAX_DATA_BUF, fp);
				traffic_used += sz;
				if(write(sck, buf, sz)==-1)
				{
					printf("Error sending..");
					goto __exit;
				}
			}
			break;
	}

__exit:
	cfg_write_int("traffic", cfg_read_int("traffic",0)+traffic_used);
	free(buf);
}

void server_data_receive(int sck, FILE *fp, char *repr, char trans)
{
	char *buf;
	uint16_t sz;
	int traffic_used = 0;

	buf = malloc(MAX_DATA_BUF+3);

	switch(trans)
	{
		case 'B':
			while((sz=read(sck, buf, MAX_DATA_BUF+3)))
			{
				traffic_used += sz;
				memcpy(&sz, &buf[1], 2);
				if(!(*buf & FTP_B_EOR))
					fwrite(&buf[3], 1, sz, fp);
				if(*buf & FTP_B_EOF)
					break;
			}
			break;
		case 'C':
			break;
		case 'S':
		default:
			while((sz=read(sck, buf, MAX_DATA_BUF)))
			{
				traffic_used += sz;
				fwrite(buf,1,sz,fp);
			}
			break;
	}

	cfg_write_int("traffic", cfg_read_int("traffic",0)+traffic_used);
	free(buf);
}

#include "commands/base.c"

op_t operations_table[] = 
{
	{"ABOR",	OP_FLAG_LOGIN,						ftp_op_abor },
	{"SYST",	OP_FLAG_LOGIN,						ftp_op_syst },
	{"TYPE",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN,				ftp_op_type },
	{"MODE",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN,				ftp_op_mode },
	{"USER",	OP_FLAG_ARGUMENT,					ftp_op_user },
	{"PASS",	OP_FLAG_ARGUMENT,					ftp_op_pass },
	{"DELE",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_dele },
	{"RETR",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_retr },
	{"PWD" ,	OP_FLAG_LOGIN,						ftp_op_pwd  },
	{"CWD" ,	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_cwd  },
	{"CDUP",	OP_FLAG_LOGIN,						ftp_op_cdup },
	{"MKD" ,	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLCREATE,	ftp_op_mkd  },
	{"RMD" ,	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_rmd  },
	{"LIST",	OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_list },
	{"NLST",	OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_nlst },
	{"PORT",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN,				ftp_op_port },
	{"LPRT",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN,				ftp_op_lprt },	
	{"PASV",	OP_FLAG_LOGIN,						ftp_op_pasv },
	{"LPSV",	OP_FLAG_LOGIN,						ftp_op_lpsv },
	{"RNFR",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLEXISTS,	ftp_op_rnfr },
	{"RNTO",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLCREATE,	ftp_op_rnto },
	{"STOR",	OP_FLAG_ARGUMENT|OP_FLAG_LOGIN|OP_FLAG_FLCREATE,	ftp_op_stor },
	{"STOU",	OP_FLAG_LOGIN,						ftp_op_stor },
	{"APPE",	OP_FLAG_LOGIN|OP_FLAG_ARGUMENT,				ftp_op_appe },
	{"\0",}
};

void server_session(int sck)
{
	session_t *session;
	
	session = malloc(sizeof(session_t));

	session_init(session);
	prctl(PR_SET_NAME,"SESSION",0,0,0);

	ftp_code_send(sck,FTP_C_READY_FOR_NEWFAG);

	session->control_socket = sck;
	session->data_ip = get_socket_addr(sck);
	session->data_port = 51200;


	while(! session->quit)
	{
		int i;
		int cmd_n = -1;
		char *tmp;
		
		session->svptr = NULL;
		bzero(session->in_buf, 300);
		if(read(sck, session->in_buf, 290)<=0)
			break;

		if(cfg_offline()) /*That means server has shutted down while we were reading command*/
			break;

		session->data_port++;

		tmp = strchr(session->in_buf,'\n');
		if(tmp)
			*tmp='\0';

		log_infos("<--- %s", session->in_buf);

		session->cmd = strtok_r(session->in_buf, " \r\n", &(session->svptr));

		session->arg = NULL;

		while(! isprint(*(session->cmd)))
			session->cmd++;
			
			
		if(!strcmp(session->cmd, "NOOP"))
		{
			ftp_code_send(sck, FTP_C_OKAY);
			continue;
		}
		else if(!strcmp(session->cmd, "QUIT"))
		{
			break;
		}	

		for(i=0;operations_table[i].command[0];i++)
		{
			if(!strcasecmp(operations_table[i].command, session->cmd))
			{
				cmd_n = i;
				break;
			}
		}
		
		if(cmd_n == -1)
		{
			ftp_code_send(sck, FTP_C_NOT_IMPLEMENTED);
			continue;
		}
		
		if(operations_table[cmd_n].flags & OP_FLAG_LOGIN && !session->logged)
		{
			ftp_code_send(sck, FTP_C_NOT_LOGGED_IN);
			continue;
		}
		
		if(operations_table[cmd_n].flags & OP_FLAG_ARGUMENT)
		{
			/*if(!(session->arg=strtok_r(NULL," \r\n", &session->svptr)))*/
			if(!(session->arg=strtok_r(NULL,"\r\n", &session->svptr)))
			{
				ftp_code_send(sck, FTP_C_SYNTAX_ARG_ERROR);
				continue;
			}
			str_tailfix(session->arg," \t\r\n");	
			if(operations_table[cmd_n].flags & OP_FLAG_FLEXISTS)
			{
				char *tmp;
				if(!(tmp=realpath(session->arg, NULL)))
				{
					ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
					continue;
				}
				if(strstr(tmp,session->base_dir)!=tmp)
				{
					mfree(tmp);
					ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
					continue;
				}
			}
			else if(operations_table[cmd_n].flags & OP_FLAG_FLCREATE)
			{
				char *tmp;
				tmp = reimpl_realpath(session->arg, NULL);
				if(strstr(tmp,session->base_dir)!=tmp)
				{
					mfree(tmp);
					ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
					continue;
				}
			}
		}
		
		ftp_code_send(sck, operations_table[cmd_n].callback(session));
	}

	ftp_code_send(sck, FTP_C_CLOSING);
	session_clean(session);
	free(session);
	//close(sck);
	exit(0);
}

void server_sigchld(int sig)
{
	int sv=errno;
    	while(waitpid((pid_t)(-1),0,WNOHANG)>0);
	errno=sv;
}

void server_main(void)
{
	int sck, new_socket;
	struct sockaddr_in srv;
	struct sockaddr client;
	socklen_t c=sizeof(struct sockaddr_in);
	char msg_buf[256];	
	struct sigaction sa;
	sa.sa_handler = &server_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_NOCLDWAIT;
	sigaction(SIGCHLD,&sa,0);
	log_info("server_main()...");

	cfg_write_int("pid", getpid());

	sck = socket(AF_INET, SOCK_STREAM, 0);

	if(sck == -1)
	{
		log_error("socket() failed:");
		strerror_r(errno, msg_buf, 256);
		log_error(msg_buf);
		return;
	}

	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = INADDR_ANY;
	srv.sin_port = htons(cfg_read_int("port",8841));

	log_info("listening...");
	if(bind(sck, (struct sockaddr*)&srv, sizeof(srv)) < 0)
	{
		return;
	}

	if(listen(sck, 8) < 0)
	{
		strerror_r(errno, msg_buf, 256);
		log_errors("listen() failed: %s", msg_buf);
		return;
	}

	while((new_socket = accept(sck, (struct sockaddr*)&client,&c)))
	{
		int pid = fork();
		printf("New connection\n");
		if(pid==0)
		{
			server_session(new_socket);
			exit(0);
		}
	}

	printf("This line should never be printed.\n");

}

void server_cli(int server_pid)
{
#ifdef INTERACTIVE_MODE
	//command-line-interface for admin
	char buf[200], *cmd=NULL, *svptr=NULL;
	printf("Admin/Debug CLI for ftp server\n");
	prctl(PR_SET_NAME,"CLI",0,0,0);
	//printf("Users registered: %i\n", users_count());
	while(1==1)
	{
		printf("> ");
		fgets(buf,200,stdin);
		if(strchr(buf,'\n'))
			*strchr(buf,'\n') = 0;
		if(*buf==0) continue;
		cmd = strtok_r(buf," ", &svptr);
		if(cmd==NULL) continue;
		if(!strcmp(cmd,"exit") || !strcmp(cmd,"quit") || !strcmp(cmd,"q"))
		{
			kill(server_pid, SIGTERM);
			cfg_exit();
			exit(0);
		}
		else if(!strcmp(cmd, "get"))
		{
			char *str, *nm, *def="(None)";
			nm = strtok_r(NULL," ", &svptr);
			if(nm == NULL)
				printf("Need variable name\n");
			else
			{
				str = cfg_read_str(nm, def);
				printf("\t%s=%s \n", nm, str);
				if(str!=def)
					free(str);
			}
		}
		else if(!strcmp(cmd, "find"))
		{
			char *beg, *ptr;
			int cnt=0;
			beg = strtok_r(NULL," ", &svptr);
			if(!beg)
				beg = "";

			while((ptr=cfg_find_key(beg,cnt,NULL)))
			{
				printf("%4i %s = %s\n", ++cnt, ptr, cfg_read_str(ptr,"(none)"));
				free(ptr);
			}
			
			
		}
		else if(!strcmp(cmd, "set"))
		{
			char *nm, *vl;
			nm = strtok_r(NULL," ", &svptr);
			vl = strtok_r(NULL," =", &svptr);
			if(nm == NULL || vl == NULL)
				printf("Need variable name and value\n");
			else
				cfg_write_str(nm, vl);
		}

	}
#endif
}
// vim: noai:ts=8:sw=8
// vim: noai:ts=8:sw=8
