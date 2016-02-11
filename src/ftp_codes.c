#define __IMPL
#include "ftp_codes.h"
#undef __IMPL
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ftp_code_send(int sck, int code)
{
        char *msg_append;
        switch(code)
        {
		case FTP_C_NOSIGNAL:
			return;
		case FTP_C_READY_FOR_NEWFAG:
			msg_append = "Service ready";		break;
		case FTP_C_OKAY:
			msg_append = "Okay";			break;
		case FTP_C_NOT_LOGGED_IN:
			msg_append = "Not logged in";		break;
		case FTP_C_NOT_IMPLEMENTED:
			msg_append = "Not implemented";		break;
		case FTP_C_LOGGED_IN:
			msg_append = "Logged in normaly";	break;
		case FTP_C_NEED_PASSWORD:
			msg_append = "User OK. Need password";	break;
		case FTP_C_SYSNAME:
			msg_append = "UNIX system type";	break;
		default:
			msg_append = "";
			break;
	}
	dprintf(sck, "%i %s\r\n%c", code, msg_append, 0);

}
// vim: noai:ts=8:sw=8
