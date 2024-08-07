/*
 * proxy.c - A concurrent http proxy that support GET method
 */

#include "cache.h"
#include "sbuf.h"
#include "utils.h"

/* Recommended max cache and object sizes */
#define MAX_OBJECT_SIZE 102400

/* Arguments for Prethreading */
#define NTHREADS 8
#define SUBFSIZE 16

void *process_client(void *vargp);
void proxyerror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg,
                jmp_buf error, int error_code);

void parse_request_line(int fd, rio_t *rio_p, char *host, char *port, char *uri,
                        jmp_buf error, int error_code);
void request_server(int fd, rio_t *rio_p, char *host, char *port, char *uri, jmp_buf error, int error_code);
void read_requesthdrs(rio_t *rp);

/* Used for cache */
sem_t mutex_cache_reader, mutex_cache_writer; /* Both initially = 1 */

/* Shared buffer of connected descriptors */
sbuf_t sbuf;

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

    Signal(SIGPIPE, SIG_IGN);
    /* Init for cache */
    Sem_init(&mutex_cache_reader, 0, 1);
    Sem_init(&mutex_cache_writer, 0, 1);

    /* Init for prethread */
    sbuf_init(&sbuf, SUBFSIZE);
    /* Create worker threads and allocated cache buffer */
    for (int i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, process_client, NULL);
    while (1)
    {
        char hostname[MAXLINE], port[MAXLINE];
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;

        clientlen = sizeof(clientaddr);
        int connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (connfd < 0)
        {
            fprintf(stderr, "%s: %s\n", "accept error", strerror(errno));
            continue;
        }

        int rc;
        if ((rc = getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0)) != 0)
        {
            Close(connfd);
            fprintf(stderr, "%s: %s\n", "getnameinfo error", gai_strerror(rc));
            continue;
        }

        printf("Accepted connection from (%s, %s)\n", hostname, port);

        sbuf_insert(&sbuf, connfd);
    }
}

/*
 * process_client - handle one HTTP request transaction
 */
void *process_client(void *vargp)
{
    Pthread_detach(pthread_self());

    char *cache_buffer = Malloc(MAX_OBJECT_SIZE);
    while (1)
    {

        int fd_client_proxy = sbuf_remove(&sbuf);

        jmp_buf proxy_error;
        int fd_proxy_server;
        switch (setjmp(proxy_error))
        {
        case 0:
            break;
        case 1:
            fprintf(stderr, "fd(%d): Error in request header\n", fd_client_proxy);
            Close(fd_client_proxy);
            continue;
        case 2:
            fprintf(stderr, "fd(%d): Error in requeset server\n", fd_client_proxy);
            Close(fd_client_proxy);
            Close(fd_proxy_server);
            continue;
        case 3:
            fprintf(stderr, "fd(%d): Error in Get responds from server and write to client\n", fd_client_proxy);
            Close(fd_client_proxy);
            Close(fd_proxy_server);
            continue;
        default:
            continue;
        }

        /* Read request line and headers */
        rio_t rio_client_proxy;
        Rio_readinitb(&rio_client_proxy, fd_client_proxy);

        char host[MAXLINE], port[MAXLINE], uri[MAXLINE];
        parse_request_line(fd_client_proxy, &rio_client_proxy, host, port, uri, proxy_error, 1);
        // printf("%s %s %s\n", host, port, uri);

        /* find in cache */
        const cache_t *cache_p = read_cache(host, port, uri);
        if (cache_p != NULL)
        {
            printf("cache hit\n");
            read_requesthdrs(&rio_client_proxy);
            Rio_writen_proxy(fd_client_proxy, cache_p->response, cache_p->response_size, proxy_error, 1);
            Close(fd_client_proxy);
            continue;
        }

        printf("cache miss\n");
        /* Not find in cache, connect to sever for response*/
        /* Connect to server */
        if ((fd_proxy_server = open_clientfd(host, port)) < 0)
        {
            unix_error_proxy("Open_clientfd error");
            fprintf(stderr, "fd(%d): Connect to http://%s:%s failed\n",
                    fd_client_proxy, host, port);
            Close(fd_client_proxy);
            continue;
        }

        /* Request server */

        request_server(fd_proxy_server, &rio_client_proxy, host, port, uri, proxy_error, 2);

        /* Get responds from server and write to client */
        rio_t rio_proxy_server;
        char buf[MAXLINE];
        Rio_readinitb(&rio_proxy_server, fd_proxy_server);
        ssize_t num;
        size_t total = 0;
        // char *cache_buffer_p = ;
        while ((num = Rio_readnb_proxy(&rio_proxy_server,
                                       buf, MAXLINE, proxy_error, 3)) > 0)
        {
            Rio_writen_proxy(fd_client_proxy, buf, num, proxy_error, 3);
            if (total + num <= MAX_OBJECT_SIZE)
                memcpy(cache_buffer + total, buf, num);
            total += num;
        }

        Close(fd_client_proxy);
        Close(fd_proxy_server);

        if (total <= MAX_OBJECT_SIZE)
            write_cache(host, port, uri, cache_buffer, total);
    }
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
    /* Watch out User-Agent have \r\n inside it*/
    sprintf(buf, "User-Agent: %s", user_agent_hdr);
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "Connection: close\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);

    // read other header from client
    Rio_readlineb_proxy(rio_p, buf, MAXLINE, error, error_code);
    while (strcmp(buf, "\r\n"))
    {
        if (strncasecmp(buf, "Host", strlen("Host")) &&
            strncasecmp(buf, "User-Agent", strlen("User-Agent")) &&
            strncasecmp(buf, "Connection", strlen("Connection")) &&
            strncasecmp(buf, "Proxy-Connection", strlen("Proxy-Connection")))
        {
            Rio_writen_proxy(fd, buf, strlen(buf), error, error_code);
        }
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

/*
 * read_requesthdrs - read HTTP request headers and do nothing
 */
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
        Rio_readlineb(rp, buf, MAXLINE);
    return;
}