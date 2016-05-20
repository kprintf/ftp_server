
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <limits.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "map_lib.h"
#include "log.h"
#include "common.h"

void thread_name(char *name)
{
	prctl(PR_SET_NAME,name,0,0,0);
}

void thread_get_name(char *dst)
{
	prctl(PR_GET_NAME,dst,0,0,0);
}

void *mfree(void *ptr)
{
	if(ptr)
		free(ptr);
	return NULL;
}

char* fperms_str(int perms, char *buf)
{
	if(buf==NULL)
		buf = malloc(10);

	memset(buf,'-',9);
	buf[9]=0;
	if(perms & S_IRUSR)
		buf[0]='r';
	if(perms & S_IWUSR)
		buf[1]='w';
	if(perms & S_IXUSR)
		buf[2]='x';
	if(perms & S_IRGRP)
		buf[3]='r';
	if(perms & S_IWGRP)
		buf[4]='w';
	if(perms & S_IXGRP)
		buf[5]='x';
	if(perms & S_IROTH)
		buf[6]='r';
	if(perms & S_IWOTH)
		buf[7]='w';
	if(perms & S_IXOTH)
		buf[8]='x';

	return buf;
}	

char* reimpl_realpath(char *path, char *buf)
/*	Standart C realpath doesnt work with imaginary file paths
 *	SO we need to reimpliment that function
 *	Blyad'
 *	When we have a guarantee that pathed file/dir exists, it'd better to use standart realpath
 *	as more reliable and (maybe) fast
 * */
{
	char *ptr, *sv;

	if(path==NULL)
		return NULL;

	if(buf == NULL)
		buf = malloc(PATH_MAX), buf[0]=0;


	if(*path!='/')
		getcwd(buf, strlen(buf)==0?PATH_MAX:(strlen(buf)+1));
	
	
	sv = NULL;
	ptr = strtok_r(path,"/", &sv);

	do
	{
		if(ptr)
		{
			if(*ptr=='.' && *(1+strrchr(ptr,'.'))==0)
			{
				while(*(++ptr)=='.')
				{
					char *tmp;

					tmp = strrchr(buf,'/');
					if(tmp)
						*tmp = 0;
				}
			}
			else
			{
				strcat(buf,"/");
				strcat(buf, ptr);
			}
		}
	}
	while((ptr = strtok_r(NULL,"/", &sv)));

	return buf;
}

int open_net_socket(uint32_t ip, uint16_t port, typeof(&bind) fnc)
{
	int sck;
	/*int vl1 = 1;*/
	struct sockaddr_in sockaddr;

	if(fnc == NULL)
		fnc = bind;

	ip   = htonl(ip);
	port = htons(port);
	
	sck = socket(AF_INET, SOCK_STREAM, 0);
	if(sck < 0)
		return -1;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = ip;
	sockaddr.sin_port = port;

	if(fnc(sck, (struct sockaddr*) &sockaddr, sizeof(sockaddr))<0)
	{
		close(sck);
		return -1;
	}
/*
	setsockopt(sck, SOL_SOCKET, SO_REUSEADDR, &vl1, sizeof(int));
#ifdef SO_REUSERPORT
	setsockopt(sck, SOL_SOCKET, SO_REUSEPORT, &vl1, sizeof(int));
#endif */



	return sck;
}

uint16_t get_socket_port(int sck)
{
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	if(getsockname(sck,(struct sockaddr*)&sin, &slen)==-1)
		return 0;
	else
		return ntohs(sin.sin_port);
}

uint32_t get_socket_addr(int sck)
{
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	if(getsockname(sck,(struct sockaddr*)&sin, &slen)==-1)
		return 0;
	else
		return ntohl(sin.sin_addr.s_addr);
}

int open_file_socket(char *fname)
{
	int s, l;
	struct sockaddr_un remote;
	s = socket(AF_UNIX, SOCK_STREAM, 0);

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, fname);
	l = strlen(remote.sun_path) + sizeof(remote.sun_family);
	connect(s, (struct sockaddr*) &remote, l);

	return s;
}

void close_socket(int sd)
{
	char null[200];
	shutdown(sd, 2);
	while(read(sd, null, 200));
	close(sd);
}

void str_tailfix(char *str, char *set)
{
	char *ptr=&str[strlen(str)-1];
	while(ptr>=str&&strchr(set,*ptr))*ptr-- =0;
}
// vim: noai:ts=8:sw=8
