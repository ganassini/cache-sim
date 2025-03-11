#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "util.h"

#define PROGRAM_NAME 	argv[0]
#define ARG_REPL 		argv[4]
#define ARG_FLAG 		argv[5]
#define ARG_INPUT_FILE	argv[6]
#define N_BITS_OFFSET	(int)(log2(c->block_size))
#define N_BITS_INDEX	(int)(log2(c->n_sets))
#define N_BITS_TAG		32 - N_BITS_OFFSET - N_BITS_INDEX
#define RATE(n) 		(double)((double)(n) / (double)(cache->accesses))

typedef enum { LRU, FIFO, RANDOM } Replacement;

typedef struct {
    bool valid;
    uint32_t tag;
} Line;

typedef struct {
    Line* lines;
    uint32_t* lru;
} Set;

typedef struct {
    Set* sets;
    uint32_t n_sets;
    uint32_t block_size;
    uint32_t assoc;
    Replacement repl;
	uint32_t accesses;
	uint32_t hits;
	uint32_t misses;
	uint32_t miss_compulsory;
	uint32_t miss_capacity;
	uint32_t miss_conflict;
} Cache;

Cache* create_cache(uint32_t ns, uint32_t bs, uint32_t a, Replacement r);
void free_cache(Cache* cache);
void access_cache(Cache *cache, uint32_t address, int flag);

int main(int argc, char** argv) 
{
    if (argc != 7) {
        usage(PROGRAM_NAME);
        return -1;
    }

    uint32_t nsets = convtoul(argv[1]);
    uint32_t bsize = convtoul(argv[2]);
    uint32_t assoc = convtoul(argv[3]);

	if ((nsets * bsize * assoc) > UINT32_MAX) { printf("\ncache should be smaller\n\n"); return -1; }

    Replacement repl;

	if 		(!strcmp(ARG_REPL, "L")) { repl = LRU; } 
	else if (!strcmp(ARG_REPL, "F")) { repl = FIFO; } 
	else if (!strcmp(ARG_REPL, "R")) { repl = RANDOM; } 
	else 	{ printf("\n\"%s\" is not a valid replacement policy.\n\n", ARG_REPL); return -1; }

    int flag = strcmp(ARG_FLAG, "0");

    FILE* input = fopen(ARG_INPUT_FILE, "rb");
    if (!input) {
        printf("\nfailed to open \"%s\"\n\n", ARG_INPUT_FILE);
        return -1;
    }

    Cache* cache = create_cache(nsets, bsize, assoc, repl);
    if (!cache) { 
        fclose(input);
        printf("\nfailed to create cache\n\n");
        return -1; 
    }

    uint32_t address;

    while (fread(&address, sizeof(uint32_t), 1, input))
        access_cache(cache, swap_endian(address), flag);

    fclose(input);

	printf("%u, %.4lf, %.4lf, %.4lf, %.4lf, %.4lf\n", cache->accesses
													, RATE(cache->hits) 
													, RATE(cache->misses)
												    , RATE(cache->miss_compulsory)
												    , RATE(cache->miss_capacity)
												    , RATE(cache->miss_conflict));

    free_cache(cache);

    return 0;
}

Cache* create_cache(uint32_t ns, uint32_t bs, uint32_t a, Replacement r)
{
    Cache *c = (Cache*)malloc(sizeof(Cache));
    
	if (!c) return NULL;

    c->n_sets = ns;
    c->block_size = bs;
    c->assoc = a;
    c->repl = r;
	c->accesses = 0;
	c->hits = 0;
	c->misses = 0;
	c->miss_compulsory = 0;
	c->miss_capacity = 0;
	c->miss_conflict = 0;

    c->sets = (Set*)malloc(ns * sizeof(Set));
    
	if (!c->sets) { free(c); return NULL; }

    for (uint32_t i = 0; i < ns; i++) {
        c->sets[i].lines = (Line*)malloc(a * sizeof(Line));

        if (!c->sets[i].lines) {
            for (uint32_t j = 0; j < i; j++) {
                free(c->sets[j].lines);
                if (r == LRU || r == FIFO) 
					free(c->sets[j].lru);
            }
            free(c->sets);
            free(c);
            return NULL;
        }

        for (uint32_t j = 0; j < a; j++) {
            c->sets[i].lines[j].valid = false;
            c->sets[i].lines[j].tag = 0;
        }

        if (r == LRU || r == FIFO) {
            c->sets[i].lru = (uint32_t*)malloc(a * sizeof(uint32_t));
            if (!c->sets[i].lru) {
                for (uint32_t j = 0; j <= i; j++) {
                    free(c->sets[j].lines);
                    if (j < i && (r == LRU || r == FIFO)) free(c->sets[j].lru);
                }
                free(c->sets);
                free(c);
                return NULL;
            }

            for (uint32_t j = 0; j < a; j++) {
                c->sets[i].lru[j] = 0;
            }
        } else {
            c->sets[i].lru = NULL;
        }
    }

    return c;
}

void access_cache(Cache *c, uint32_t address, int flag) {
    uint32_t tag = address >> (N_BITS_OFFSET + N_BITS_INDEX);
    uint32_t index = (address >> N_BITS_OFFSET) & ((int)pow(2, N_BITS_INDEX) - 1);
    uint32_t offset = address % c->block_size;
    c->accesses++;

    Set *set = &c->sets[index];

    for (uint32_t i = 0; i < c->assoc; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
			c->hits++;
            if (!flag) 
				printf("H -> address=%u, tag=%u, index=%u, offset=%u\n", address, tag, index, offset);
            if (c->repl == LRU)
                set->lru[i] = c->accesses;
            return;
        }
    }

    if (!flag) 
		printf("M -> address=%u, tag=%u, index=%u, offset=%u, ", address, tag, index, offset);

	c->misses++;

    for (uint32_t i = 0; i < c->assoc; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = true;
            set->lines[i].tag = tag;
            if (c->repl == LRU || c->repl == FIFO)
                set->lru[i] = c->accesses;
            if (!flag) 
				printf("compulsory\n");
			c->miss_compulsory++;
            return;
        }
    }

    int address_to_replace;
    if (c->repl == RANDOM) { 
		address_to_replace = rand() % c->assoc; 
	} else {
        address_to_replace = 0;
        for (uint32_t i = 1; i < c->assoc; i++) {
            if (set->lru[i] < set->lru[address_to_replace]) address_to_replace = i;
		}
    }

    set->lines[address_to_replace].tag = tag;
    if (c->repl == LRU || c->repl == FIFO) {
        set->lru[address_to_replace] = c->accesses;
    }

	// TODO this is not well implemented
	if (c->n_sets == 1) {
		if (!flag) 
			printf("capacity\n");
		c->miss_capacity++;
	} else {
		if (!flag)
			printf("conflict\n");
		c->miss_conflict++;
	}
}

void free_cache(Cache* c)
{
    if (c) {
        if (c->sets) {
            for (uint32_t i = 0; i < c->n_sets; i++) {
                free(c->sets[i].lines);
                free(c->sets[i].lru);
            }
            free(c->sets);
        }
        free(c);
    }
}

