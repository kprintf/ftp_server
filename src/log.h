#ifndef LOG_H
#define LOG_H

extern void log_init(void);
extern void log_info(char *msg);
extern void log_infos(char *msg, ...);
extern void log_warn(char *msg);
extern void log_warns(char *msg, ...);
extern void log_error(char *msg);
extern void log_errors(char *msg, ...);

#endif
// vim: noai:ts=8:sw=8
