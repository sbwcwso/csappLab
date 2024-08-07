/*
 * cache.h - prototypes and definitions for proxy cache.
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

typedef struct cache
{
    char *host;
    char *port;
    char *uri;
    struct cache *next;

    char *response;

    size_t response_size;
    unsigned long least_used_time;
} cache_t;

cache_t *read_cache(const char *host, const char *port, const char *uri);
void write_cache(const char *host, const char *port, const char *uri,
                 const char *response, size_t response_size);

#endif
