/*
 * utils.c - Functions for proxy lab
 */

#include "utils.h"

/*
** unix_error_proxy - unix error for proxy, doesn't exit
*/
void unix_error_proxy(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

/*
** posix_error_proxy - posix error for proxy, doesn't exit
*/
void posix_error_proxy(int code, char *msg) /* Posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
}

/*
** unix_error_proxy_longjmp - unix error for proxy, but use longjmp
*/
void unix_error_proxy_longjmp(char *msg, jmp_buf error, int error_code)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    longjmp(error, error_code);
}

/*
 * Rio_writen_proxy - used for proxy, use longjmp to handle error
 */
void Rio_writen_proxy(int fd, void *usrbuf, size_t n,
                      jmp_buf error, int error_code)
{
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error_proxy_longjmp("Rio_written_proxy error", error, error_code);
}

/*
 * Rio_readnb_proxy - used for proxy, use longjmp to handle error
 */
ssize_t Rio_readnb_proxy(rio_t *rp, void *usrbuf, size_t n,
                         jmp_buf error, int error_code)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error_proxy_longjmp("Rio_readnb error", error, error_code);
    return rc;
}

/*
** Rio_readlineb_proxy - used for proxy, use longjmp to handle error
*/
ssize_t Rio_readlineb_proxy(rio_t *rp, void *usrbuf, size_t maxlen,
                            jmp_buf error, int error_code)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error_proxy_longjmp("Rio_readlineb error", error, error_code);
    return rc;
}

/*
 * Malloc_p - malloc wrapper for proxy
 */
void *Malloc_p(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL)
        unix_error_proxy("Malloc error");
    return p;
}
