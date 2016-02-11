
#if 0
#define CMD_SPLIT " ,\r\n"

typedef struct
{
	int 	 control_pid;
	int 	 control_socket;
	int 	 transmission_pid;
	int	 transmission_socket;
	uint32_t data_ip;
	uint16_t data_port;
	int	 login;
	char   	*user_name;	
	int    	 user_id;
	char   	*user_pswd; /*YES, no cypher. Yet.*/
	char   	*dir_home;
	char   	*dir_base;
	char	 representation[20];
	char	 structure;
	char	 transmission;
	char     input[200];
	char	*cmd_tokenizer;
} session_t;

static void session_free(session_t *s)
/* Should be called from control thread
 * */
{
	if(s->user_name)
		free(s->user_name);
	if(s->user_pswd)
		free(s->user_pswd);
	if(s->dir_home)
		free(s->dir_home);
	if(s->dir_base)
		free(s->dir_base);

	if(s->transmission_socket != -1)
		close(s->transmission_socket);

	if(s->transmission_pid != -1)
		kill(s->transmission_pid, SIGTERM);

	if(s->control_socket != -1)
		close(s->control_socket);
}

int session_check_path(session_t *s, char *fpath)
/* Returns 1 if defined path is in base directory zone and user have theoretical access there*/
{
	if(s->base)
	{
		char *str = reimpl_realpath(fpath, NULL);
		if(strstr(str, s->base) == str)
		{
			free(str);
			return 1;
		}
		else
		{
			free(str);
			return 0;
		}
	}
	else return 1;

}

void server_session(int sck)
{
	char in_buf[200], out_buf[200], *cmd, *svptr, *name=NULL, *pswd=NULL, *tmp;
	char *base_dir=NULL, *home_dir=NULL;
	int logged = 0;
	uint32_t data_ip  = get_socket_addr(sck);
	uint16_t data_port= 51200;
	int data_socket = -1;
	char representation[30]="AN";
	char fstruct = 'F';
	char ftrans  = 'S';
	int  representation_local_bytes = 0;
	int connection_socket=-1;
	
	prctl(PR_SET_NAME,"SESSION",0,0,0);

	ftp_code_send(sck,FTP_C_READY_FOR_NEWFAG);

#define LOGIN_CHECK     	if(!logged) \
				{ \
					ftp_code_send(sck, FTP_C_NOT_LOGGED_IN); \
					continue; \
				}

#define ARGUMENT_CHECK(dst)	dst = strtok_r(NULL," ,\r\n",&svptr); \
				if(! dst ) \
				{ \
					ftp_code_send(sck, FTP_C_SYNTAX_ARG_ERROR); \
					continue; \
				}

#define F_OP_BEGIN(path)	if(strlen( path )>strlen(base_dir))\
					if(strstr( path ,base_dir)== path )\
					{

#define F_OP_END(status)			if(! status )\
							ftp_code_send(sck, FTP_C_FILE_OKAY);\
						else\
							ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);\
					}\
					else\
					{\
						ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);\
					}\
				else\
					ftp_code_send(sck,FTP_C_FILE_UNAVAILABLE);

	while(1)
	{
		svptr = NULL;

		bzero(in_buf, 200);
		read(sck, in_buf, 200);


		tmp = strchr(in_buf,'\n');
		if(tmp)
			*tmp='\0';

		printf("<<< %s\n", in_buf);

		cmd = strtok_r(in_buf, " \r\n", &svptr);

		while(! isprint(*cmd))
			cmd++;

		printf("CMD %s\n", cmd);

		if(!strcmp(cmd, "USER"))
		{
			name = strdup(strtok_r(NULL," \r\n", &svptr));
			if(user_id(name) == USER_ID_NOT_EXISTS_OR_ERROR)
				ftp_code_send(sck, FTP_C_NOT_LOGGED_IN);
			else
			{
				char *tmp1, *tmp2;
				tmp1 = malloc(strlen(name)+30);
				tmp2 = malloc(strlen(name)+30);
				sprintf(tmp1,"user[%s].base", name);
				sprintf(tmp2,"data/%s/", name);


				if(user_login(name,NULL) != USER_ID_NOT_EXISTS_OR_ERROR)
				{
					logged = 1;
					ftp_code_send(sck, FTP_C_LOGGED_IN);
				}
				else
					ftp_code_send(sck, FTP_C_NEED_PASSWORD);
				base_dir = cfg_read_str(tmp1,NULL);
				if(!base_dir)
					base_dir = strdup(tmp2);


				{
					char *tmp = base_dir;

					base_dir = realpath(tmp, NULL);

					free(tmp);
				}

				sprintf(tmp1,"user[%s].home", name);

				if((home_dir=cfg_read_str(tmp1,NULL)))
				{
					chdir(home_dir);
				}

				free(tmp1);
				free(tmp2);
			}
		}
		else if(!strcmp(cmd, "ABOR"))
		{
			printf("Aborting transmission...\n");
			//kill_trans(name);
			//ftp_code_send(sck, FTP_C_DATA_CLOSE_SUCCESS); will be sent from another thread
			if(connection_socket != -1)
			{
				close(connection_socket);
				connection_socket = -1;
			}
		}
		else if(!strcmp(cmd, "DELE"))
		{
			char *tmp1;
			int status;

			LOGIN_CHECK;	
			ARGUMENT_CHECK(tmp1);

			tmp1 = reimpl_realpath(tmp1,NULL);
			F_OP_BEGIN(tmp1);
					status = unlink(tmp1);
			F_OP_END(status);

			free(tmp1);

			

		}
		else if(!strcmp(cmd, "LIST"))
		{
			char *dir;
			DIR *hp;
			int status;

			if(!(dir=strtok_r(NULL," \r\n", &svptr)))
				dir="./";
			dir = realpath(dir, NULL);

			printf("dir:%s\n", dir);

			F_OP_BEGIN(dir);
				if((hp = opendir(dir)))
				{
					struct dirent *res, entry;
					struct stat st;
					char tmp[10];
					char tm[40];


					if(data_socket == -1)
						if((data_socket = open_net_socket(	data_ip, data_port, 
										  	connect))<0)
						{
							ftp_code_send(sck, FTP_C_CANT_OPEN_DATA);
							closedir(hp);
							continue;
						}


					connection_socket =  accept(data_socket,NULL,NULL);

					while(!(status=readdir_r(hp, &entry, &res)))
					{
						stat(res->d_name, &st);
						strftime(	tm, 40, "%b %d %H:%M", 
								localtime(&(st.st_mtime)));

						printf("%c%s %5d %4d %8d %s %s\r\n",
							(res->d_type==DT_DIR?'d':'-'),
							fperms_str(st.st_mode, tmp),
							st.st_nlink,
							st.st_uid,
							st.st_gid,
							st.st_size,
							tm,
							res->d_name);
						dprintf(connection_socket, "%c%s %5d %4d %8d %s %s\r\n",
							(res->d_type==DT_DIR?'d':'-'),
							fperms_str(st.st_mode, tmp),
							st.st_nlink,
							st.st_uid,
							st.st_gid,
							st.st_size,
							tm,
							res->d_name);
					}

					close(connection_socket);


				}
				else
					status = 1;
				
			F_OP_END(status);

		}
		else if(!strcmp(cmd, "TYPE"))
		{
			char *tmp1;
			ARGUMENT_CHECK(tmp1);
			if(strchr("AEIL",*tmp1))
			{
				strcpy(representation, tmp1);
				if((tmp1=strtok_r(NULL," \r\n", &svptr)))
					strcat(representation, tmp1);

				printf("REPR:%s\n", representation);

				ftp_code_send(sck, FTP_C_OKAY);

			}
			else
				ftp_code_send(sck,FTP_C_SYNTAX_ARG_ERROR);

		}
		else if(!strcmp(cmd, "MODE"))
		{
			char *tmp1;
			ARGUMENT_CHECK(tmp1);

			if(strchr("SBC",*tmp1))
			{
				ftrans = *tmp1;
				ftp_code_send(sck, FTP_C_OKAY);
			}
			else
				ftp_code_send(sck, FTP_C_SYNTAX_ARG_ERROR);
		}
		else if(!strcmp(cmd, "STRU"))
		{
			char *tmp1;
			ARGUMENT_CHECK(tmp1);

			if(*tmp1=='F') /*lol.*/
			{
				fstruct = *tmp1;
				ftp_code_send(sck, FTP_C_OKAY);
			}
			else
				ftp_code_send(sck, FTP_C_SYNTAX_ARG_ERROR);
		}
		else if(!strcmp(cmd, "PASV"))
		{
			char buf[30];
			uint32_t tmp1;
			parted_uint16_t tmp2;

			tmp1 = htonl(data_ip);
			tmp2.u16 = htons(data_port);
		
			inet_ntop(AF_INET, &tmp1, buf, 30);

			for(tmp1=0;tmp1<3;tmp1++)
				if(strchr(buf,'.'))
					*strchr(buf,'.')=',';	

			if(data_socket != -1)
				close(data_socket);

			data_socket = open_net_socket(INADDR_ANY, data_port, bind);


			dprintf(	sck,"%i Entering Passive Mode (%s,%i,%i)\r\n",
					FTP_C_PASSIVE_MODE,
					buf,
					tmp2.u8[0],
					tmp2.u8[1]);

			listen(data_socket, 4);
		}
		else if(!strcmp(cmd, "RETR"))
		{
			char *pathname;
			char *ptr;
			FILE *fp;
			int here_we_fork = -1;

			LOGIN_CHECK;
			ARGUMENT_CHECK(pathname);


			if( ! (ptr = realpath(pathname,NULL)) )
			{
				ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
				continue;
			}
				else free(ptr);

			if(! (fp = fopen(pathname, "r")))
			{
				ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
				continue;
			}


			if(data_socket == -1)
				if((data_socket = open_net_socket(data_ip, data_port, connect))<0)
				{
					fclose(fp);
					ftp_code_send(sck, FTP_C_CANT_OPEN_DATA);
					continue;
				}

			printf("\nsending %s\n", pathname);

			connection_socket =  accept(data_socket,NULL,NULL);

			save_trans_socket(name, connection_socket);

			if((here_we_fork=fork())!=0)
			{
				ftp_code_send(sck, FTP_C_OPEN_DATA_NOW);
				server_data_send(connection_socket, fp, representation, ftrans);
				close(connection_socket);
				//close(data_socket);
				ftp_code_send(sck,FTP_C_DATA_CLOSE_SUCCESS);
				fclose(fp);
				printf("FINALE\n");			
				exit(0);
			}
			
			//data_socket = -1;
		}
		else if(!strcmp(cmd, "PORT"))
		/*TODO: rewrite!*/
		/*Hope it will work on not-intel endian systems*/
		{
			char *tmp1;
			int tmp2;

			parted_uint32_t ip;
			parted_uint16_t port;

			//LOGIN_CHECK;	


			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &ip.u8[0]);
			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &ip.u8[1]);
			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &ip.u8[2]);
			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &ip.u8[3]);


			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &port.u8[0]);
			ARGUMENT_CHECK(tmp1);
			sscanf(tmp1, "%i", &port.u8[1]);

			data_ip = ip.u32;
			data_port = port.u16;

			ftp_code_send(sck,FTP_C_OKAY);		

		}
		else if(!strcmp(cmd, "RMD"))
		{
			char *tmp1;
			int status;

			LOGIN_CHECK;	
			ARGUMENT_CHECK(tmp1);

			tmp1 = reimpl_realpath(tmp1,NULL);
			F_OP_BEGIN(tmp1);
					status = rmdir(tmp1);
			F_OP_END(status);

			free(tmp1);

			

		}
		else if(!strcmp(cmd, "PWD"))
		{
			char *tmp1;

			LOGIN_CHECK;

			tmp1 = realpath("./",NULL);

			dprintf(sck,"%i '%s' \r\n", FTP_C_CREATED, tmp1);

			free(tmp1);
		}
		else if(!strcmp(cmd, "CWD"))
		{
			char *tmp1;
			int status;

			LOGIN_CHECK;
			ARGUMENT_CHECK(tmp1);


			tmp1 = reimpl_realpath(tmp1,NULL);
			F_OP_BEGIN(tmp1);
					status=chdir(tmp1);
			F_OP_END(tmp1);

			free(tmp1);
		}
		else if(!strcmp(cmd, "CDUP"))
		{
			char *tmp1;
			int status;

			LOGIN_CHECK;

			tmp1 = reimpl_realpath("../",NULL);
			F_OP_BEGIN(tmp1);
					status = chdir(tmp1);
			F_OP_END(status);

			free(tmp1);


		}
		else if(!strcmp(cmd, "RNFR") || !strcmp(cmd,"RNTO"))
		{
			char *tmp1;
			static char *from=NULL, *to=NULL;

			LOGIN_CHECK;
			ARGUMENT_CHECK(tmp1);


			tmp1 = reimpl_realpath(tmp1, NULL);

			if(!strcmp(cmd, "RNFR"))
			{
				from = strdup(tmp1);
				ftp_code_send(sck, FTP_C_NEED_MORE_INFO);
			}
			else
			{
				int status;
				to = tmp1;

				if(strlen(tmp1)>strlen(base_dir))
					if(strstr(tmp1,base_dir)==tmp1)
					{
						status=rename(from, to);
						if(!status)
							ftp_code_send(sck, FTP_C_CREATED);
						else
							ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
					}
					else
					{
						ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
					}
				else
					ftp_code_send(sck,FTP_C_FILE_UNAVAILABLE);

				free(from);
				from = NULL;
				to = NULL;

			}


			free(tmp1);
		}
		else if(!strcmp(cmd, "MKD"))
		{
			char *tmp1;

			LOGIN_CHECK;
			ARGUMENT_CHECK(tmp1);


			tmp1 = reimpl_realpath(tmp1, NULL);

			if(strlen(tmp1)>strlen(base_dir))
				if(strstr(tmp1,base_dir)==tmp1)
				{
					int status;
					status=mkdir(tmp1, S_IRWXU | S_IRWXG | S_IROTH );
					if(!status)
						ftp_code_send(sck, FTP_C_CREATED);
					else
						ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
				}
				else
				{
					ftp_code_send(sck, FTP_C_FILE_UNAVAILABLE);
				}
			else
				ftp_code_send(sck,FTP_C_FILE_UNAVAILABLE);




			free(tmp1);
		}
		else if(!strcmp(cmd, "SYST"))
		{
			ftp_code_send(sck, FTP_C_SYSNAME);
		}
		else if(!strcmp(cmd, "PASS"))
		{
			pswd = strdup(strtok_r(NULL," \r\n", &svptr));
			printf("PSWD:'%s'\n",pswd);
			if(user_login(name,pswd) != USER_ID_NOT_EXISTS_OR_ERROR)
			{
				logged = 1;
				ftp_code_send(sck, FTP_C_LOGGED_IN);
			}
			else
				ftp_code_send(sck, FTP_C_NOT_LOGGED_IN);
		}
		else if(!strcmp(cmd, "NOOP"))
		{
			ftp_code_send(sck, FTP_C_OKAY);
			break;
		}
		else if(!strcmp(cmd, "QUIT"))
		{
			break;
		}
		else ftp_code_send(sck, FTP_C_NOT_IMPLEMENTED);

	}

	ftp_code_send(sck, FTP_C_CLOSING);
	if(name) free(name);
	if(pswd) free(pswd);
	if(home_dir) free(home_dir);
	if(base_dir) free(base_dir);
	close(sck);
	exit(0);
}

#endif
// vim: noai:ts=8:sw=8
