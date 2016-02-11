#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <sys/socket.h>

typedef union
{
	uint16_t u16;
	uint8_t u8[2];
} parted_uint16_t;

typedef union
{
	uint32_t u32;
	uint16_t u16[2];
	uint8_t  u8[4];
} parted_uint32_t;

extern char* reimpl_realpath(char *path, char *buf);
extern uint16_t get_socket_port(int sck);
extern uint32_t get_socket_addr(int sck);
extern int open_net_socket(uint32_t ip, uint16_t port, typeof(&bind) sck_func);
extern int open_file_socket(char *fname);
extern void thread_name(char *name);
extern void thread_get_name(char *name);
extern char* fperms_str(int perms, char *buf);
extern void close_socket(int sd);
extern void* mfree(void *ptr);
extern void str_tailfix(char *str, char *set);
#endif
// vim: noai:ts=8:sw=8
