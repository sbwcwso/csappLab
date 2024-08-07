/*
 * proxy.c - A concurrent http proxy that support GET method
 */

#include "utils.h"

void *process_client(void *vargp);
void proxyerror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg,
                jmp_buf error, int error_code);

void parse_request_line(int fd, rio_t *rio_p, char *host, char *port, char *uri,
                        jmp_buf error, int error_code);
void request_server(int fd, rio_t *rio_p, char *host, char *port, char *uri, jmp_buf error, int error_code);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    /* Check command line args */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_t tid;
    int listenfd;
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        int *connfdp;
        char hostname[MAXLINE], port[MAXLINE];
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;

        clientlen = sizeof(clientaddr);
        if ((connfdp = malloc(sizeof(*connfdp))) == NULL)
        {
            unix_error_proxy("Malloc error");
            continue;
        }
        *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (*connfdp < 0)
        {
            Free(connfdp);
            fprintf(stderr, "%s: %s\n", "accept error", strerror(errno));
            continue;
        }

        int rc;
        if ((rc = getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0)) != 0)
        {
            Close(*connfdp);
            Free(connfdp);
            fprintf(stderr, "%s: %s\n", "getnameinfo error", gai_strerror(rc));
            continue;
        }

        printf("Accepted connection from (%s, %s)\n", hostname, port);

        int rc_t;
        if ((rc_t = pthread_create(&tid, NULL, process_client, connfdp)) != 0)
        {
            Close(*connfdp);
            Free(connfdp);
            posix_error_proxy(rc_t, "pthread_create error");
            continue;
        }
    }
}

/*
 * process_client - handle one HTTP request transaction
 */
void *process_client(void *vargp)
{
    int fd_client_proxy = *((int *)vargp);
    Free(vargp);
    int rc;
    if ((rc = pthread_detach(pthread_self())) != 0)
    {
        posix_error_proxy(rc, "pthread_detach error");
        Close(fd_client_proxy);
        return NULL;
    }

    jmp_buf proxy_error;
    int fd_proxy_server;
    switch (setjmp(proxy_error))
    {
    case 0:
        break;
    case 1:
        fprintf(stderr, "fd(%d): Error in request header\n", fd_client_proxy);
        Close(fd_client_proxy);
        return NULL;
    case 2:
        fprintf(stderr, "fd(%d): Error in requeset server\n", fd_client_proxy);
        Close(fd_client_proxy);
        Close(fd_proxy_server);
        return NULL;
    case 3:
        fprintf(stderr, "fd(%d): Error in Get responds from server and write to client\n", fd_client_proxy);
        Close(fd_client_proxy);
        Close(fd_proxy_server);
        return NULL;
    default:
        break;
    }

    /* Read request line and headers */
    rio_t rio_client_proxy;
    Rio_readinitb(&rio_client_proxy, fd_client_proxy);

    char host[MAXLINE], port[MAXLINE], uri[MAXLINE];
    parse_request_line(fd_client_proxy, &rio_client_proxy, host, port, uri, proxy_error, 1);
    // printf("%s %s %s\n", host, port, uri);

    /* Connect to server */
    if ((fd_proxy_server = open_clientfd(host, port)) < 0)
    {
        unix_error_proxy("Open_clientfd error");
        fprintf(stderr, "fd(%d): Connect to http://%s:%s failed\n",
                fd_client_proxy, host, port);
        Close(fd_client_proxy);
        return NULL;
    }

    /* Request server */

    request_server(fd_proxy_server, &rio_client_proxy, host, port, uri, proxy_error, 2);

    /* Get responds from server and write to client */
    rio_t rio_proxy_server;
    char buf[MAXLINE];
    Rio_readinitb(&rio_proxy_server, fd_proxy_server);
    ssize_t num;
    while ((num = Rio_readnb_proxy(&rio_proxy_server, buf, MAXLINE, proxy_error, 3)) > 0)
        Rio_writen_proxy(fd_client_proxy, buf, num, proxy_error, 3);

    Close(fd_client_proxy);
    Close(fd_proxy_server);
    return NULL;
}

/*
 * request_server - send request to server
 */
void request_server(int fd, rio_t *rio_p, char *host, char *port, char *uri, jmp_buf error, int error_code)
{
    char buf[MAXLINE];
    sprintf(buf, "GET %s HTTP/1.0\r\n", uri);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "Host: %s:%s\r\n", host, port);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "User-Agent: %s\r\n", user_agent_hdr);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "Connection: close\r\n");
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);

    // read other header from client
    Rio_readlineb_proxy(rio_p, buf, MAXLINE, error, error_code);
    while (strcmp(buf, "\r\n"))
    {
        if (strcasecmp(buf, "Host") && strcasecmp(buf, "User-Agent") &&
            strcasecmp(buf, "Connection") && strcasecmp(buf, "Proxy-Connection"))
            Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
        Rio_readlineb_proxy(rio_p, buf, MAXLINE, error, error_code);
    }
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
}

/*
 * parse_request_line - parse request line, set host, port, and uri if succees
 *
 */
void parse_request_line(int fd, rio_t *rio_p, char *host, char *port, char *uri, jmp_buf error, int error_code)
{
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    if (!Rio_readlineb_proxy(rio_p, buf, MAXLINE, error, error_code))
        longjmp(error, 1);

    sscanf(buf, "%s %s %s", method, url, version);

    // check method
    if (strcasecmp(method, "GET"))
    {
        proxyerror(fd, method, "501", "Not Implemented",
                   "Tiny does not implement this method", error, error_code);
        longjmp(error, 1);
    }

    // check http version
    if (strcmp(version, "HTTP/1.1") != 0 && strcmp(version, "HTTP/1.0") != 0)
    {
        proxyerror(fd, url, "505", "HTTP version not supported",
                   "Proxy does not support version in request", error, error_code);
        longjmp(error, 1);
    }

    // only support http request
    if (strncmp(url, "http://", 7) != 0)
    {
        proxyerror(fd, url, "500", "Proxy Error",
                   "Proxy only support http requests.", error, error_code);
        longjmp(error, 1);
    }

    // get host and uri
    char *host_start = url + 7;
    char *port_start = strchr(host_start, ':');
    char *uri_start = strchr(host_start, '/');
    if (uri_start != NULL)
    {
        strcpy(uri, uri_start);
        strncpy(host, host_start, uri_start - host_start);
        host[uri_start - host_start] = '\0';
    }
    else
    {
        strcpy(host, host_start);
        strcpy(uri, "/");
    }

    if (port_start != NULL)
    {
        strcpy(port, host + (port_start - host_start) + 1);
        host[port_start - host_start] = '\0';
    }
    else
    {
        strcpy(port, "80");
    }
}

/*
 * proxyerror - returns an error message to the client
 */
void proxyerror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg,
                jmp_buf error, int error_code)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
}
