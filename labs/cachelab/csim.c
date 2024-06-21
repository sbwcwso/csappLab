#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define ADDRESS_BIT 64

typedef struct line_tag
{
    unsigned long tag;
    unsigned long last_used_time;
    int valid_flag;
} line_t;

enum status_t
{
    HIT,
    MISS,
    EVICTION
} status;

static int hit_count, miss_count, eviction_count;
static unsigned set_bits, block_bits;
static unsigned long S, E;
/* cache_visit_counter may overflow, but it's very unlikely to happen. */
static unsigned long cache_visit_counter;
static char *file_name;
static int verbose_flag;

void print_help()
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n\n"
           "Examples:\n"
           "linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void get_opt(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
        switch (opt)
        {
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        case 'v':
            verbose_flag = TRUE;
            break;
        case 's':
            set_bits = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 't':
            file_name = optarg;
            break;
        case '?':
            print_help();
            exit(EXIT_FAILURE);
        }
}

line_t *init_cache()
{
    line_t *cache;
    cache = calloc(S * E, sizeof(*cache));
    if (cache == NULL)
        exit(EXIT_FAILURE);
    return cache;
}

void find_target_in_set_and_update_status(enum status_t *status_p, line_t **target_p, line_t *set, unsigned long tag)
{
    *status_p = EVICTION;
    *target_p = set;
    for (int e = 0; e < E; e++)
    {
        line_t *line = set + e;

        if (line->valid_flag == FALSE) // cold miss
        {
            *status_p = MISS;
            *target_p = line;
            break;
        }

        if (line->tag == tag)
        {
            *status_p = HIT;
            *target_p = line;
            break;
        }

        if (line->last_used_time < (*target_p)->last_used_time) // leatst recently used
            *target_p = line;
    }

    (*target_p)->last_used_time = ++cache_visit_counter;
}

void update_and_log_cache_status(enum status_t status, char instruction, line_t *target, unsigned long tag)
{
    switch (status)
    {
    case HIT:
        hit_count++;
        if (verbose_flag)
            printf(" hit");
        break;

    case MISS:
        target->tag = tag;
        target->valid_flag = TRUE;
        miss_count++;
        if (verbose_flag)
            printf(" miss");
        break;

    case EVICTION:
        target->tag = tag;
        miss_count++;
        eviction_count++;
        if (verbose_flag)
            printf(" miss eviction");
        break;

    default:
        break;
    }
    if (instruction == 'M')
    {
        hit_count++;
        if (verbose_flag)
            printf(" hit");
    }
}

void process_instruction(line_t *cache, char instruction, unsigned long address)
{

    if (instruction != 'L' && instruction != 'S' && instruction != 'M')
        exit(EXIT_FAILURE);

    unsigned long tag, s, block_offset;
    unsigned tag_bits;
    tag_bits = ADDRESS_BIT - (set_bits + block_bits);
    block_offset = address << (tag_bits + set_bits) >> (ADDRESS_BIT - block_bits);
    s = address << tag_bits >> (ADDRESS_BIT - set_bits);
    tag = address >> (ADDRESS_BIT - tag_bits);

    enum status_t status;
    line_t *target; // The target element for the input address.
    find_target_in_set_and_update_status(&status, &target, cache + s * E, tag);
    update_and_log_cache_status(status, instruction, target, tag);

    if (verbose_flag)
        // For part B use
        printf(" s = %lu, b= %lu\n", s, block_offset);
}

void sim_cache(line_t *cache)
{
    FILE *fp;
    char instruction;
    unsigned long address;
    unsigned size;

    fp = fopen(file_name, "r");
    while (fscanf(fp, " %c %lx,%u", &instruction, &address, &size) != EOF)
    {
        if (instruction != 'I')
        {
            if (verbose_flag == TRUE)
                printf("%c %lx,%u", instruction, address, size);
            process_instruction(cache, instruction, address);
        }
    }
    fclose(fp);
}

int main(int argc, char **argv)
{
    get_opt(argc, argv);
    if (set_bits == 0 || E == 0 || block_bits == 0 || file_name == NULL)
    {
        printf("%s: Missing required command line argrment\n", argv[0]);
        print_help();
        exit(EXIT_FAILURE);
    }

    S = 1 << set_bits;
    line_t *cache = init_cache();
    sim_cache(cache);
    free(cache);
    printSummary(hit_count, miss_count, eviction_count);

    return EXIT_SUCCESS;
}
