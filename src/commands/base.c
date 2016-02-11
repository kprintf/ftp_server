/* This files and others in this dir will be inserted directly into server.c / server.o module
 * */

int ftp_op_user(session_t *s)
{
	s->name = strdup(s->arg);
	
	if(user_id(s->name) == USER_ID_NOT_EXISTS_OR_ERROR)
		return FTP_C_NOT_LOGGED_IN;
	else
	{
		/*char *tmp1;
		tmp1 = malloc(strlen(s->name)+30);
		
		sprintf(tmp1,"user[%s].base", s->name);
		if(!(s->base_dir = cfg_read_str(tmp1,NULL)))
			s->base_dir = strdup("/tmp");

		sprintf(tmp1,"user[%s].home", s->name);
		if((s->home_dir=cfg_read_str(tmp1,NULL)))
			chdir(s->home_dir);
		else
			chdir(s->base_dir);

		free(tmp1);
			
			
		if(user_login(s->name,NULL) == USER_ID_NOT_EXISTS_OR_ERROR)*/
			return FTP_C_NEED_PASSWORD;
		/*else
			return s->logged = 1, FTP_C_LOGGED_IN;*/
	}
}

int ftp_op_pass(session_t *s)
{
	s->pswd = strdup(s->arg);
	
	if(user_login(s->name,s->pswd) != USER_ID_NOT_EXISTS_OR_ERROR)
	{
		char buf[32];
		char *home;
		sprintf(buf,"ftp_%s",s->name);
		thread_name(buf);
		home = user_home(user_id(s->name));
		chdir(home);
		free(home);
		return s->logged = 1, FTP_C_LOGGED_IN;
	}
	else
		return FTP_C_NOT_LOGGED_IN;
}

int ftp_op_dele(session_t *s)
{
	int status;

	status = unlink(s->arg);
	
	if(status)
		return FTP_C_FILE_UNAVAILABLE;
	else
		return FTP_C_FILE_OKAY;
			
}

int ftp_op_stou(session_t *s)
{
	FILE *fp;
	int here_we_fork = -1;
	char nmbuf[30];

	sprintf(nmbuf,"TMP%sXXXXXX", s->name);

	if(! (fp = fdopen(mkstemp(nmbuf), "w")))
		return FTP_C_FILE_UNAVAILABLE;
	


	if(s->data_socket == -1)
		if((s->data_socket = open_net_socket(s->data_ip, s->data_port, connect))<0)
			return fclose(fp), FTP_C_CANT_OPEN_DATA;



	if((here_we_fork=fork())==0)
	{
		int we_fork_again=0;
		if((we_fork_again=fork())==0)
		{
			s->connection_socket =  accept(s->data_socket,NULL,NULL);
			close(s->data_socket);
			ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
			server_data_receive(s->connection_socket, fp, s->frepr, s->ftrans);
			exit(0);
		}
		save_trans_pid(s->name, we_fork_again);
		wait(NULL);
                close(s->connection_socket);
                ftp_code_send(s->control_socket,FTP_C_DATA_CLOSE_SUCCESS);
                fclose(fp);
                printf("FINALE\n");
                exit(0);

	}

	printf("transmission pid:%i\n", here_we_fork);


	s->connection_pid = here_we_fork;
	
	close(s->data_socket);
	s->data_socket = -1;
	return FTP_C_NOSIGNAL;
}

int ftp_op_appe(session_t *s)
{
	char *pathname;
	FILE *fp;
	int here_we_fork = -1;

	pathname = s->arg;

	if(! (fp = fopen(pathname, "a")))
		return FTP_C_FILE_UNAVAILABLE;
	


	if(s->data_socket == -1)
		if((s->data_socket = open_net_socket(s->data_ip, s->data_port, connect))<0)
			return fclose(fp), FTP_C_CANT_OPEN_DATA;

	printf("\nreceiving %s\n", pathname);


	if((here_we_fork=fork())==0)
	{
		int we_fork_again=0;
		if((we_fork_again=fork())==0)
		{
			s->connection_socket =  accept(s->data_socket,NULL,NULL);
			close(s->data_socket);
			ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
			server_data_receive(s->connection_socket, fp, s->frepr, s->ftrans);
			exit(0);
		}
		save_trans_pid(s->name, we_fork_again);
		wait(NULL);
                close(s->connection_socket);
                ftp_code_send(s->control_socket,FTP_C_DATA_CLOSE_SUCCESS);
                fclose(fp);
                printf("FINALE\n");
                exit(0);

	}

	printf("transmission pid:%i\n", here_we_fork);


	s->connection_pid = here_we_fork;
	
	close(s->data_socket);
	s->data_socket = -1;
	return FTP_C_NOSIGNAL;
}

int ftp_op_stor(session_t *s)
{
	char *pathname;
	FILE *fp;
	int here_we_fork = -1;

	pathname = s->arg;

	if(! (fp = fopen(pathname, "w")))
		return FTP_C_FILE_UNAVAILABLE;
	


	if(s->data_socket == -1)
		if((s->data_socket = open_net_socket(s->data_ip, s->data_port, connect))<0)
			return fclose(fp), FTP_C_CANT_OPEN_DATA;

	printf("\nreceiving %s\n", pathname);


	if((here_we_fork=fork())==0)
	{
		int we_fork_again=0;
		if((we_fork_again=fork())==0)
		{
			s->connection_socket =  accept(s->data_socket,NULL,NULL);
			close(s->data_socket);
			ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
			server_data_receive(s->connection_socket, fp, s->frepr, s->ftrans);
			exit(0);
		}
		save_trans_pid(s->name, we_fork_again);
		wait(NULL);
                close(s->connection_socket);
                ftp_code_send(s->control_socket,FTP_C_DATA_CLOSE_SUCCESS);
                fclose(fp);
                printf("FINALE\n");
                exit(0);

	}

	printf("transmission pid:%i\n", here_we_fork);


	s->connection_pid = here_we_fork;
	
	close(s->data_socket);
	s->data_socket = -1;
	return FTP_C_NOSIGNAL;
}

int ftp_op_retr(session_t *s)
{
	char *pathname;
	FILE *fp;
	int here_we_fork = -1;

	pathname = s->arg;

	if(! (fp = fopen(pathname, "r")))
		return FTP_C_FILE_UNAVAILABLE;
	


	if(s->data_socket == -1)
		if((s->data_socket = open_net_socket(s->data_ip, s->data_port, connect))<0)
			return fclose(fp), FTP_C_CANT_OPEN_DATA;

	printf("\nsending %s\n", pathname);


	if((here_we_fork=fork())==0)
	{
		int we_fork_again=0;
		if((we_fork_again=fork())==0)
		{
			s->connection_socket =  accept(s->data_socket,NULL,NULL);
			close(s->data_socket);
			ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
			server_data_send(s->connection_socket, fp, s->frepr, s->ftrans);
			exit(0);
		}
		save_trans_pid(s->name, we_fork_again);
		wait(NULL);
                close(s->connection_socket);
                ftp_code_send(s->control_socket,FTP_C_DATA_CLOSE_SUCCESS);
                fclose(fp);
                printf("FINALE\n");
                exit(0);

	}

	printf("transmission pid:%i\n", here_we_fork);


	s->connection_pid = here_we_fork;
	
	close(s->data_socket);
	s->data_socket = -1;
	return FTP_C_NOSIGNAL;
}

int ftp_op_pwd(session_t *s)
{
	char *path;

	path = realpath("./",NULL);

	dprintf(s->control_socket,"%i '%s' \r\n", FTP_C_CREATED, path);

	free(path);
	
	return FTP_C_NOSIGNAL;
}

int ftp_op_rmd(session_t *s)
{
	int status;

	status=rmdir(s->arg);
	
	if(!status)
		return FTP_C_FILE_OKAY;
	else
		return FTP_C_FILE_UNAVAILABLE;
}

int ftp_op_mkd(session_t *s)
{
	int status;

	status=mkdir(s->arg, S_IRWXU | S_IRWXG | S_IROTH );
	
	if(!status)
		return FTP_C_CREATED;
	else
		return FTP_C_FILE_UNAVAILABLE;
}

int ftp_op_abor(session_t *s)
{
	int pid;
       	char buf[100];

	sprintf(buf,"transmission[%s]", s->name);

	pid = cfg_read_int(buf,-1);
	if(pid != -1)
		kill(pid, SIGKILL);
	return FTP_C_DATA_CLOSE_SUCCESS;
}

int ftp_op_cwd(session_t *s)
{
	int status;

	status=chdir(s->arg);
	
	if(!status)
		return FTP_C_OKAY;
	else
		return FTP_C_FILE_UNAVAILABLE;
}

int ftp_op_list(session_t *s)
{
	DIR *hp;
	int status;
	char *nm;

	if((s->arg=strtok_r(NULL," \r\n", &s->svptr)) == NULL)
		nm = "./";
	else
		nm = s->arg;

	if((hp = opendir(nm)))
	{
		struct dirent entry, *res=NULL;
		struct stat st;
		char tmp[10];
		char tm[40];


		if(s->data_socket == -1)
			if((s->data_socket = open_net_socket(	s->data_ip, s->data_port, 
													connect))<0)
				return closedir(hp), FTP_C_CANT_OPEN_DATA;
			


		s->connection_socket =  accept(s->data_socket,NULL,NULL);

		ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
		
		for(	status = readdir_r(hp, &entry, &res);
			res != NULL && status == 0;
			status = readdir_r(hp, &entry, &res))
		{
			stat(entry.d_name, &st);
			strftime(	tm, 40, "%b %d %H:%M", 
					localtime(&(st.st_mtime)));

			dprintf(s->connection_socket, "%c%s %5d %4d %8d %10ld %s %s\r\n",
				(entry.d_type==DT_DIR?'d':'-'),
				fperms_str(st.st_mode, tmp),
				st.st_nlink,
				st.st_uid,
				st.st_gid,
				st.st_size,
				tm,
				entry.d_name);
		}


		close(s->connection_socket);
		return closedir(hp), FTP_C_DATA_CLOSE_SUCCESS;

	}
	else
		return FTP_C_FILE_UNAVAILABLE;		
}

int ftp_op_nlst(session_t *s)
{
	DIR *hp;
	int status;
	char *nm;

	if((s->arg=strtok_r(NULL," \r\n", &s->svptr)) == NULL)
		nm = "./";
	else
		nm = s->arg;

	if((hp = opendir(nm)))
	{
		struct dirent entry, *res=NULL;

		if(s->data_socket == -1)
			if((s->data_socket = open_net_socket(	s->data_ip, s->data_port, 
													connect))<0)
				return closedir(hp), FTP_C_CANT_OPEN_DATA;
			


		s->connection_socket =  accept(s->data_socket,NULL,NULL);

		ftp_code_send(s->control_socket, FTP_C_OPEN_DATA_NOW);
		
		for(	status = readdir_r(hp, &entry, &res);
			res != NULL && status == 0;
			status = readdir_r(hp, &entry, &res))
			dprintf(s->connection_socket, "%s\r\n",
				entry.d_name);
		

		close(s->connection_socket);
		return closedir(hp), FTP_C_DATA_CLOSE_SUCCESS;

	}
	else
		return FTP_C_FILE_UNAVAILABLE;		
}

int ftp_op_port(session_t *s)
{

	parted_uint32_t ip;
	parted_uint16_t port;

	sscanf(	s->arg, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", 
			&ip.u8[0],&ip.u8[1],&ip.u8[2],&ip.u8[3], &port.u8[0], &port.u8[1]);

	s->data_ip = ip.u32;
	s->data_port = port.u16;

	return FTP_C_OKAY;		

}

int ftp_op_pasv(session_t *s)
{
	parted_uint32_t tmp1;
	parted_uint16_t tmp2;

	tmp1.u32 = htonl(s->data_ip);
	tmp2.u16 = htons(s->data_port);


	if(s->data_socket != -1)
	{
		shutdown(s->data_socket, 2);
		close(s->data_socket);
	}

	s->data_socket = open_net_socket(INADDR_ANY, s->data_port, bind);


	listen(s->data_socket, 4);
	dprintf(s->control_socket,"%i Entering Passive Mode (%i,%i,%i,%i,%i,%i)\r\n",
			FTP_C_PASSIVE_MODE,
			tmp1.u8[0],
			tmp1.u8[1],
			tmp1.u8[2],
			tmp1.u8[3],
			tmp2.u8[0],
			tmp2.u8[1]);

	return FTP_C_NOSIGNAL;
}

int ftp_op_rnfr(session_t *s)
{
	s->rnfr = strdup(s->arg);
	return  FTP_C_NEED_MORE_INFO;
}

int ftp_op_rnto(session_t *s)
{
	
	int status;
	
	if(s->rnfr == NULL)
		return FTP_C_BAD_SEQUENCE;
	
	status = rename(s->rnfr, s->arg); 
	
	s->rnfr = mfree(s->rnfr);
	
	if(!status)
		return FTP_C_CREATED;
	else
		return FTP_C_FILE_UNAVAILABLE;
}


int ftp_op_syst(session_t *s)
{
	return FTP_C_SYSNAME;
}

int ftp_op_type(session_t *s)
{
	if(strchr("AEIL",*s->arg))
	{
		char *tmp1;
		strcpy(s->frepr, s->arg);
		if((tmp1=strtok_r(NULL," \r\n", &s->svptr)))
			strcat(s->frepr, tmp1);

		printf("REPR:%s\n", s->frepr);

		return FTP_C_OKAY;

	}
	else
		return FTP_C_SYNTAX_ARG_ERROR;

}

int ftp_op_cdup(session_t *s)
{
	char *tmp1;
	int status;

	tmp1 = realpath("./", NULL);
	
	if(strcmp(tmp1, s->base_dir))
	{
		free(tmp1);
		status = chdir("..");
		if(!status)
			return FTP_C_FILE_OKAY;
		else
			return FTP_C_FILE_UNAVAILABLE;
	}
	else
		return free(tmp1), FTP_C_FILE_OKAY;
}

int ftp_op_mode(session_t *s)
{
	if(strchr("SBC",*s->arg))
	{
		s->ftrans = *s->arg;
		return FTP_C_OKAY;
	}
	else
		return FTP_C_SYNTAX_ARG_ERROR;
}

int ftp_op_stru(session_t *s)
{
	if(*s->arg=='F') /*lol.*/
	{
		s->fstruct = *s->arg;
		return FTP_C_OKAY;
	}
	else
		return FTP_C_SYNTAX_ARG_ERROR;
}

int ftp_op_lprt(session_t *s)
{
	int unused[3];
	parted_uint32_t ip;
	parted_uint16_t port;

	sscanf(	s->arg, "%i,%i,%hhu,%hhu,%hhu,%hhu,%i,%hhu,%hhu", 
			&unused[0], &unused[1],
			&ip.u8[0],&ip.u8[1],&ip.u8[2],&ip.u8[3], 
			&unused[2],
			&port.u8[0], &port.u8[1]);

	assert(unused[0] == 4);
	assert(unused[1] == 4);
	assert(unused[2] == 2);
	s->data_ip = ip.u32;
	s->data_port = port.u16;

	return FTP_C_OKAY;		

}

int ftp_op_lpsv(session_t *s)
{
	parted_uint32_t tmp1;
	parted_uint16_t tmp2;

	tmp1.u32 = htonl(s->data_ip);
	tmp2.u16 = htons(s->data_port);


	if(s->data_socket != -1)
	{
		shutdown(s->data_socket, 2);
		close(s->data_socket);
	}

	s->data_socket = open_net_socket(INADDR_ANY, s->data_port, bind);


	listen(s->data_socket, 4);
	dprintf(s->control_socket,"%i Entering Long Passive Mode (4,4,%i,%i,%i,%i,2,%i,%i)\r\n",
			FTP_C_PASSIVE_MODE,
			tmp1.u8[0],
			tmp1.u8[1],
			tmp1.u8[2],
			tmp1.u8[3],
			tmp2.u8[0],
			tmp2.u8[1]);

	return FTP_C_NOSIGNAL;
}
