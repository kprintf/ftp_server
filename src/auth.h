#ifndef AUTH_H
#define AUTH_H

#define USER_ID_NOT_EXISTS_OR_ERROR -1
extern void auth_init		  (void);
extern int  user_id		  (char *name);
extern int  user_login		  (char *name, char *pswd);
extern char*user_home		  (int id);
//extern int  users_count		  (void);
//extern int  user_get_storage_used (char *name);
//extern int  user_get_storage_limit(char *name);
#endif
// vim: noai:ts=8:sw=8
