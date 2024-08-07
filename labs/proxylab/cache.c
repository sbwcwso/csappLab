/*
 * cache.c - Functions for proxy cache
 */

#include "cache.h"
#include <limits.h>

static cache_t head = {NULL, NULL, NULL, NULL, NULL, 0, 0};
static int readcnt = 0;
extern sem_t mutex_cache_reader, mutex_cache_writer; /* Both initially = 1 */
static unsigned long cache_visit_times = 0;
static size_t cache_size = 0;

/* Recommended max cache sizes */
#define MAX_CACHE_SIZE 1049000

void free_cache(cache_t *item)
{
    Free(item->response);
    Free(item);
}

void evict_cache()
{
    unsigned long least_used_time = ULONG_MAX;
    cache_t *eviction = NULL;
    cache_t *eviction_prev = NULL;
    cache_t *item = &head;
    while (item->next != NULL)
    {
        cache_t *prev = item;
        item = item->next;
        if (item->least_used_time < least_used_time)
        {
            least_used_time = item->least_used_time;
            eviction = item;
            eviction_prev = prev;
        }
    }

    if (eviction != NULL)
    {
        printf("cache evictied\n");
        eviction_prev->next = eviction->next;
        cache_size -= eviction->response_size;
        free_cache(eviction);
    }
}

cache_t *find_cache(const char *host, const char *port, const char *uri)
{
    cache_t *cache_p = &head;
    while (cache_p->next != NULL)
    {
        cache_p = cache_p->next;
        if (strcasecmp(host, cache_p->host) == 0 &&
            strcmp(port, cache_p->port) == 0 &&
            strcmp(uri, cache_p->uri) == 0)
            return cache_p;
    }
    return NULL;
}

/*
 * read_cache - find in cache
 *
 * @return: if find return the response, if not find, return NULL
 */

cache_t *read_cache(const char *host, const char *port, const char *uri)
{
    P(&mutex_cache_reader);
    readcnt++;
    if (readcnt == 1) /* First in */
        P(&mutex_cache_writer);
    V(&mutex_cache_reader);

    cache_t *res = find_cache(host, port, uri);

    P(&mutex_cache_reader);
    if (res != NULL)
        res->least_used_time = ++cache_visit_times;
    readcnt--;
    if (readcnt == 0) /* Last out */
        V(&mutex_cache_writer);
    V(&mutex_cache_reader);
    return res;
}

/*
 * write_cache - write the given resource to cache
 */
void write_cache(const char *host, const char *port, const char *uri,
                 const char *response, size_t response_size)
{
    P(&mutex_cache_writer);

    // Check if size is exists in the cache
    if (find_cache(host, port, uri) == NULL)
    {
        // Check cache size, if excedes the size limit, chose a suitalbe evicted cache
        while (cache_size + response_size > MAX_CACHE_SIZE)
            evict_cache();

        cache_t *item;
        item = Malloc(sizeof(*item));

        item->host = Malloc(strlen(host) + 1);
        strncpy(item->host, host, strlen(host) + 1);

        item->port = Malloc(strlen(port) + 1);
        strncpy(item->port, port, strlen(port) + 1);

        item->uri = Malloc(strlen(uri) + 1);
        strncpy(item->uri, uri, strlen(uri) + 1);

        item->response_size = response_size;
        item->response = Malloc(response_size);
        memcpy(item->response, response, response_size);
        item->least_used_time = ++cache_visit_times;

        item->next = head.next;
        head.next = item;

        cache_size += response_size;
    }
    V(&mutex_cache_writer);
}
