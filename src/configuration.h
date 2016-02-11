#ifndef CONFIGURATION_H
#define CONFIGURATION_H

extern void 	cfg_set_config_path(char *str);
extern void  	cfg_set_config_sock(char *str);
extern void	cfg_init();
extern int	cfg_read_int(char *name, int def);
extern char*	cfg_read_str(char *name, char *def);
extern void	cfg_write_int(char *name, int val);
extern void	cfg_write_str(char *name, char *val);
extern char*    cfg_find_key(char *beg, int skip, char *def);
extern int	cfg_offline(void);
extern void	cfg_exit();


#endif
// vim: noai:ts=8:sw=8
