#ifndef FTP_CODES_H
#define FTP_CODES_H

#define FTP_C_NOSIGNAL -1
#define FTP_C_RESTART_MARKER_REPLY	110
#define FTP_C_OPEN_DATA_NOW		150
#define FTP_C_OKAY			200
#define FTP_C_SUPERFLUOUS		202
#define FTP_C_SYSTEM_STATUS		211
#define FTP_C_DIRECTORY_STATUS		212
#define FTP_C_FILE_STATUS		213
#define FTP_C_HELPMSG			214
#define FTP_C_SYSNAME			215
#define FTP_C_READY_FOR_NEWFAG		220
#define FTP_C_CLOSING			221
#define FTP_C_DATA_CLOSE_SUCCESS	226
#define FTP_C_PASSIVE_MODE		227
#define FTP_C_LOGGED_IN			230
#define FTP_C_FILE_OKAY			250
#define FTP_C_CREATED			257
#define FTP_C_NEED_PASSWORD		331
#define FTP_C_NEED_MORE_INFO		350
#define FTP_C_CANT_OPEN_DATA		421
#define FTP_C_UNRECOGINZED		500
#define FTP_C_SYNTAX_ARG_ERROR		501
#define FTP_C_NOT_IMPLEMENTED		502
#define FTP_C_BAD_SEQUENCE		503
#define FTP_C_ARG_NOT_IMPLEMENTED	504
#define FTP_C_NOT_LOGGED_IN		530
#define FTP_C_FILE_UNAVAILABLE		550
#define FTP_S_EOR 0xFF01
#define FTP_S_EOF 0xFF02
#define FTP_B_EOR 0x80
#define FTP_B_EOF 0x40
#define FTP_B_ERR 0x20
#define FTP_B_RST 0x10

#ifndef __IMPL
extern void ftp_code_send(int socket_id, int code);
#endif

#endif
// vim: noai:ts=8:sw=8
