#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "map_lib.h"
#include "log.h"
#include "common.h"
/*!!FIXME!! should i remove this weird mechanism? */

//internal syntax:
//R[name] -- read variable.
//W[name]#[value] -- set it
//F -- shutdown
//S[skip]#[str] -- search key starting with 'str' noumbered as #skip
//answer for reading:
//N - no data
//D[...]<EOF> - variable content

static char *config_path = NULL;
static char *socket_path = NULL;
#define SOCK_PATH (socket_path?socket_path:"/tmp/ftpmod_config")
#define FILE_PATH (config_path?config_path:"ftpmod.cfg")

static struct
{
	int changes;
	map_t *map;
} data;

static void load_data(void)
{
	data.map = map_create();
	data.changes = 0;
	if(access(FILE_PATH, R_OK)!=-1)
	{
		FILE *fp = fopen(FILE_PATH, "r");
		if(fp)
		{
			char buf[400], *svptr, *nm, *val;
			while(!feof(fp))
			{
				char *tmp;
				fgets(buf,200,fp);
				tmp = strchr(buf,'\n');
				if(tmp)
					*tmp = 0;
				val= strchr(buf,':');
				nm = strtok_r(buf," :",&svptr);
				if(nm==NULL || val==NULL)
					break;
				val+=2; /*':' and space*/
				map_set(data.map, nm, val);		                        
			}
			fclose(fp);
		}
		else printf("Error while opening logs\n");
	}
	else printf("Cannot access config\n");
}

static void save_data(void)
{
	if(data.changes && data.map)
	{
		map_t *it;                
		FILE *fp = fopen(FILE_PATH, "w");

		for(it=data.map; it; it=it->nxt)
			fprintf(fp, "%s : %s\n", it->name, it->value);

		fclose(fp);
		data.changes = 0;
	}
}

int cfg_offline(void)
{
	return access(SOCK_PATH, F_OK)==-1;
}


void cfg_set_config_path(char *str)
{
	config_path = strdup(str);
}

void cfg_set_config_sock(char *str)
{
	socket_path = strdup(str);
}

void cfg_init()
{
#ifdef INTERACTIVE_MODE
	int id;
	if( access(SOCK_PATH, F_OK)!=-1)
	{
		printf("Another version is already runned.\n");
		exit(1);
	}

	if((id=fork())==0)
	{
		struct sockaddr_un local, remote;
		int l, conn;
		unsigned int t;
		int sck = socket(AF_UNIX, SOCK_STREAM, 0);
		thread_name("ftp config");
		log_info("Initializing...");
		load_data();
		local.sun_family = AF_UNIX;
		strcpy(local.sun_path, SOCK_PATH);
		/*unlink(local.sun_path);*/
		l = strlen(local.sun_path) + sizeof(local.sun_family);
		t = sizeof(remote);
		bind(sck, (struct sockaddr*) &local, l);
		listen(sck,10);
		while((conn = accept(sck, (struct sockaddr*) &remote, &t)))
		{
			char in_buf[200], *tmp;
			in_buf[read(conn, in_buf, 200)] = 0;
			switch(in_buf[0])
			{
				case 'F':
					close(conn);
					close(sck);
					save_data();
					map_free(data.map);
					data.map = NULL;
					unlink(local.sun_path);
					//log_info("config thread finalizing");
					exit(0);
					break;
				case 'R':
					tmp = map_get(data.map, &in_buf[1]);
					if(*tmp)
					{
						dprintf(conn,"D%s%c", tmp, 0);
					}
					else
						write(conn,"N",2);
					close(conn);
					break;
				case 'S':
					{
						map_t *it = data.map;
						char *val=strchr(in_buf,'#')+1;
						char *res=NULL;
						int counter;

						sscanf(&in_buf[1],"%i", &counter);

						for(; it; it = it->nxt)
						{
							if(!val || strstr(it->name,val)==it->name)
							{
								if(counter > 0)
									counter --;
								else
								{
									res = it->name;
									break;
								}
							}
						}
						if(res)
						{
							dprintf(conn, "D%s%c", res, 0);
						}
						else
							write(conn, "N", 2);
					}
					break;
				case 'W':
					{
						char *name, *val;
						data.changes = 1;
						val = strchr(in_buf,'#');
						*val++ = 0;
						name = &in_buf[1];
						map_set(data.map, name, val);
						printf("SETTING: %s = %s\n", name, val);
					}
					close(conn);
					break;
				default:
					break;
			}
		}

		
	}
	else usleep(30);
#else
	load_data();
#endif
}

int cfg_read_int(char *name, int def)
{
#ifdef INTERACTIVE_MODE
	int sck, res;
	char int_buf[40]="N";


	sck = open_file_socket(SOCK_PATH);
	dprintf(sck,"R%s%c", name, 0);
	recv(sck, int_buf, 40, 0);
	close(sck);
	if(int_buf[0]=='N')
		return def;
	else
	{
		sscanf(&int_buf[1],"%i", &res);
		return res;
	}	
#else
	char *tmp;
	tmp = map_get(data.map, name);
	if(tmp)
	{
		int ret;
		sscanf(tmp,"%i", &ret);
		return ret;
	}
	else
		return def;
#endif
}


char* cfg_read_str(char *name, char *def)
{
#ifdef INTERACTIVE_MODE
	int sck;
	char *str_buf;



	str_buf = malloc(512);
	strcpy(str_buf,"N ");
	sck = open_file_socket(SOCK_PATH);
	dprintf(sck,"R%s%c", name, 0);
	recv(sck, str_buf, 512, 0);
	close(sck);
	if(str_buf[0]=='N')
		return free(str_buf), def;
	else
	{
		char *res_str = strdup(&str_buf[1]);
		return free(str_buf), res_str;
	}
#else
	char *tmp;
	tmp = map_get(data.map, name);
	if(tmp)
		return strdup(tmp);
	else
		return def;
#endif	
}

char *cfg_find_key(char *beg, int skip, char *def)
{
#ifdef INTERACTIVE_MODE
	int sck;
	char *str_buf;



	str_buf = malloc(512);
	sck = open_file_socket(SOCK_PATH);
	dprintf(sck,"S%i#%s%c",skip, beg, 0);
	recv(sck, str_buf, 512, 0);
	close(sck);
	if(str_buf[0]=='N')
		return free(str_buf),def;
	else
	{
		char *res_str = strdup(&str_buf[1]);
		free(str_buf);

		return res_str;
	}	
#else
	map_t *it = data.map;
	char *res=NULL;
	for(; it; it = it->nxt)
		if(strstr(it->name,beg)==it->name)
		{
			if(skip > 0)
				skip --;
			else
			{
				res = it->name;
				break;
			}
		}
	if(res)
		return strdup(res);
	else
		return def;

#endif
}

#ifdef INTERACTIVE_MODE
void cfg_write_int(char *name, int val)
{
	int sck;

	sck = open_file_socket(SOCK_PATH);
	dprintf(sck, "W%s#%i%c", name, val, 0);
	close(sck);
}
void cfg_write_str(char *name, char *val)
{
	int sck;

	sck = open_file_socket(SOCK_PATH);
	dprintf(sck, "W%s#%s%c", name, val, 0);
	close(sck);
}
#endif

void cfg_exit(void)
{
#ifdef INTERACTIVE_MODE
	int sck;

	sck = open_file_socket(SOCK_PATH);
	write(sck, "F", 2);
	close(sck);
#endif
}
// vim: noai:ts=8:sw=8
