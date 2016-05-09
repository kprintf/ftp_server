#include <stdio.h>
#include <stdlib.h>
#include "configuration.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <shadow.h>
#include <pwd.h>
//TODO: check _GNU_SOURCE and __USE_GNU macroses about portability...
#define _GNU_SOURCE
#define __USE_GNU
#include <crypt.h>
#include <sys/types.h>
#include <unistd.h>
#include "log.h"

#ifdef INTERACTIVE_MODE
#	define os_users (cfg_read_int("os_users", 0))
#else
static int os_users = 0;
#endif
void auth_init(void)
{
#ifndef INTERACTIVE_MODE
	os_users = cfg_read_int("os_users", 0);
#endif
}

int user_id(char *name)
{
	if(os_users)
	{
		int res = -1;
		struct passwd uinfo;
		char *strbuf;
		void *chk;

		if(!name)
			return -1;

		strbuf = malloc(4096);
		if(getpwnam_r(name, &uinfo, strbuf, 4096, (struct passwd**)&chk)==0 && chk )
			res = uinfo.pw_uid;
		free(strbuf);

		return res;	
	}
	else
	{
		char buf[20+strlen(name)];
		int res;

		sprintf(buf,"user[%s]", name);

		res = cfg_read_int(buf,-1);

		return res;
	}
}

char *user_home(int id)
{
	char *res=NULL;
	struct passwd uinfo;
	char *strbuf;
	void *unused;
	strbuf = malloc(4096);


	if(getpwuid_r(id, &uinfo, strbuf, 4096, (struct passwd**)&unused)==0)
		res = strdup(uinfo.pw_dir);
	free(strbuf);

	return res;

}
#if 0
int user_get_storage_limit(char *name)
{	/*TODO check manuals...*/
	int sz;
	char request[200];

	sprintf(request, "user[%s].st_limit", name);

	sz = cfg_read_int(request, 0x7FFFFFFF); //2GB. Maximum signed int. lol XD

	return sz;
}
#endif

int user_login(char *name, char *pswd)
{
	if(os_users)
	{
		struct passwd pw;
		struct spwd *sp;
		char *enc, *chk, *strbuf;
		struct crypt_data *cr;
		int tmp;
		if(!name)
			return -1;

		cr = malloc(sizeof(struct crypt_data));
		cr->initialized = 0;

		strbuf = malloc(4096);
		//setpwent();
		tmp = getpwnam_r(name, &pw,strbuf,4096, (struct passwd**)&enc);
		endpwent();

		if(tmp) 
			return free(strbuf), -1;

		//setspent();
		sp = getspnam(pw.pw_name);
		endspent();

		chk = sp ? sp->sp_pwdp : pw.pw_passwd;
		enc = crypt_r(pswd, chk, cr);
		if(enc)
			if(strcmp(enc, chk)) 
			{
				log_infos("Got wrong password while trying to login as %s", name);
				return free(strbuf), free(cr), -1;
			}
			else
			{
				setgid(pw.pw_gid);
				setuid(pw.pw_uid);			
				return free(strbuf), free(cr), pw.pw_uid;
			}
		else
		{
			char msg_buf[256];
			strerror_r(errno, msg_buf, 256);
			log_errors("crypt() error %i: %s", errno, msg_buf);
			return free(strbuf), free(cr), -1;
		}
	}
	else
	{
		char buf[40+strlen(name)];
		int id;
		sprintf(buf,"user[%s]", name);
		if((id=cfg_read_int(buf,-1))==-1)
			return -1;
		else
		{
			char *str;
			strcat(buf,".password");
			if((str=cfg_read_str(buf,NULL)))
			{
				if(pswd != NULL)
				{
					if(strcmp(str,pswd))
					{
						log_infos("Got wrong password while trying to login as %s", name);
					}
					else
						return free(str), id;
				}
				return free(str), -1;
			}	
			else	
				return free(str), id;	
		}
		return -1;
	}
}

#if 0
int users_count(void)
{
	if(os_users)
	{
		return -1;/*TODO should i even write this function??*/
	}
	else
	{
		int cnt = 0, res=0;
		char *ptr;
		while( (ptr=cfg_find_key("user[",cnt++,NULL)))
		{
			if(ptr[strlen(ptr)-1]==']')
				res++;
			free(ptr);
		}

		return res;
	}
}
#endif

char *user_name(int id)
{
	if(os_users)
	{
		char *res=NULL;
		struct passwd uinfo;
		char *strbuf;
		void *unused;
		strbuf = malloc(4096);

		if(getpwuid_r(id, &uinfo, strbuf, 4096, (struct passwd**)&unused)==0)
			res = strdup(uinfo.pw_name);
		free(strbuf);

		return res;
	}
	else
	{
		int cnt = 0;
		char *ptr, *res=NULL;
		while( (ptr=cfg_find_key("user[",cnt++,NULL)))
		{
			if(ptr[strlen(ptr)-1]==']')
			{
				if(cfg_read_int(ptr,-1)==id)
				{
					ptr[strlen(ptr)-1]=0;
					res = strdup(&ptr[5]); /*after '['*/
					free(ptr);
					break;
				}
			}
			free(ptr);
		}
		return res;
	}
}


// vim: noai:ts=8:sw=8
