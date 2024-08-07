/*
 * utils.h - prototypes and definitions for proxy lab.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include "csapp.h"

/* proxy error-handling functions */
void unix_error_proxy(char *msg);
void unix_error_proxy_longjmp(char *msg, jmp_buf error, int error_code);
void posix_error_proxy(int code, char *msg);

/* Wrappers for Rio package */
void Rio_writen_proxy(int fd, void *usrbuf, size_t n,
                      jmp_buf error, int error_code);
ssize_t Rio_readnb_proxy(rio_t *rp, void *usrbuf, size_t n,
                         jmp_buf error, int error_code);
ssize_t Rio_readlineb_proxy(rio_t *rp, void *usrbuf, size_t maxlen,
                            jmp_buf error, int error_code);

#endif