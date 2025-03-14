#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define PROGRAM_NAME    argv[0]
#define ARG_NSETS		argv[1]
#define ARG_BSIZE		argv[2]
#define ARG_ASSOC		argv[3]
#define ARG_REPL        argv[4]
#define ARG_FLAG        argv[5]
#define ARG_INPUT_FILE  argv[6]
#define N_BITS_OFFSET   (int)(log2(c->block_size))
#define N_BITS_INDEX    (int)(log2(c->n_sets))
#define N_BITS_TAG      (32 - N_BITS_OFFSET - N_BITS_INDEX)
#define CACHE_SIZE		(nsets * bsize * assoc)
#define RATE(n)         (double)((double)(n) / (double)(cache->accesses))
#define RATE_MISS(n)	(double)((double)(n) / (double)(cache->misses))

typedef enum { LRU, FIFO, RANDOM } Replacement;

typedef struct LRU_Node {
    int line_index;
    struct LRU_Node* prev;
    struct LRU_Node* next;
} LRU_Node;

typedef struct {
    uint32_t *queue;
    int front;
    int rear;
    int size;
    int capacity;
} FIFO_Queue;

typedef struct {
    bool valid;
    uint32_t tag;
    LRU_Node* lru_node;
} Line;

typedef struct {
    Line* lines;
    LRU_Node* lru_head;
    LRU_Node* lru_tail;
    FIFO_Queue* fifo;
} Set;

typedef struct {
    Set* sets;
    uint32_t n_sets;
    uint32_t block_size;
    uint32_t assoc;
    Replacement repl;
	uint32_t total_valid_lines;
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
LRU_Node* lru_create_node(int line_index);
void lru_append(Set* set, LRU_Node* node);
void lru_remove(Set* set, LRU_Node* node);
FIFO_Queue* fifo_create(int capacity);
void fifo_free(FIFO_Queue* q);
int fifo_enqueue(FIFO_Queue* q, int line_index);
int fifo_dequeue(FIFO_Queue* q);
void usage(char* program_name);
uint32_t swap_endian(uint32_t value);
uint32_t convtoul(char *argv);


int main(int argc, char** argv)
{
    if (argc != 7) {
        usage(PROGRAM_NAME);
        return -1;
    }

    uint32_t nsets = convtoul(ARG_NSETS);
    uint32_t bsize = convtoul(ARG_BSIZE);
    uint32_t assoc = convtoul(ARG_ASSOC);

	// TODO this does not work
    if (CACHE_SIZE > UINT32_MAX ) {
        printf("\nerror: 32bit addressed cache should be smaller than %u\n\n", UINT32_MAX);
        return -1;
    }

    Replacement repl;
    if 		(!strcmp(ARG_REPL, "L")) { repl = LRU; }
    else if (!strcmp(ARG_REPL, "F")) { repl = FIFO; }
    else if (!strcmp(ARG_REPL, "R")) { repl = RANDOM; }
    else {
        printf("\nerror: \"%s\" is not a valid replacement policy.\n\n", ARG_REPL);
        return -1;
    }

    int flag = strcmp(ARG_FLAG, "0");

    FILE* input = fopen(ARG_INPUT_FILE, "rb");
    if (!input) {
        printf("\nerror: failed to open \"%s\"\n\n", ARG_INPUT_FILE);
        return -1;
    }

    Cache* cache = create_cache(nsets, bsize, assoc, repl);
    if (!cache) {
        fclose(input);
        printf("\nerror: failed to create cache\n\n");
        return -1;
    }

    uint32_t address;
    while (fread(&address, sizeof(uint32_t), 1, input))
        access_cache(cache, swap_endian(address), flag);

    fclose(input);

    printf("%u ", cache->accesses);
    printf("%.4lf ", RATE(cache->hits));
	printf("%.4lf ", RATE(cache->misses));
  	printf("%.2lf ", RATE_MISS(cache->miss_compulsory));
	printf("%.2lf ", RATE_MISS(cache->miss_capacity));
    printf("%.2lf\n", RATE_MISS(cache->miss_conflict));

    free_cache(cache);
    return 0;
}

Cache* create_cache(uint32_t ns, uint32_t bs, uint32_t a, Replacement r)
{
    Cache *c = malloc(sizeof(Cache));

    if (!c) return NULL;

    c->n_sets = ns;
    c->block_size = bs;
    c->assoc = a;
    c->repl = r;
	c->total_valid_lines = 0;
    c->accesses = 0;
    c->hits = 0;
    c->misses = 0;
    c->miss_compulsory = 0;
    c->miss_capacity = 0;
    c->miss_conflict = 0;

    c->sets = malloc(ns * sizeof(Set));

    if (!c->sets) { free(c); return NULL; }

    for (uint32_t i = 0; i < ns; i++) {
        c->sets[i].lines = malloc(a * sizeof(Line));

        if (!c->sets[i].lines) {
            for (uint32_t j = 0; j < i; j++)
                free(c->sets[j].lines);
            free(c->sets);
            free(c);

            return NULL;
        }

        for (uint32_t j = 0; j < a; j++) {
            c->sets[i].lines[j].valid = false;
            c->sets[i].lines[j].tag = 0;
            c->sets[i].lines[j].lru_node = NULL;
        }

		c->sets[i].lru_head = NULL;
		c->sets[i].lru_tail = NULL;
		c->sets[i].fifo = (r == FIFO) ? fifo_create(a) : NULL;
    }
    return c;
}

void access_cache(Cache *c, uint32_t address, int flag) 
{
    uint32_t tag = address >> (N_BITS_OFFSET + N_BITS_INDEX);
    uint32_t index = (address >> N_BITS_OFFSET) & ((int)pow(2, N_BITS_INDEX) - 1);
    uint32_t offset = address % c->block_size;
    c->accesses++;

    Set *set = &c->sets[index];

	// hit
    for (uint32_t i = 0; i < c->assoc; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            if (!flag)
                printf("HIT  | address=%u, tag=%u, index=%u, offset=%u\n", address, tag, index, offset);
            if (c->repl == LRU && set->lines[i].lru_node) {
				lru_remove(set, set->lines[i].lru_node);
				lru_append(set, set->lines[i].lru_node);
			}
            c->hits++;
            return;
        }
    }

    if (!flag)
        printf("MISS | address=%u, tag=%u, index=%u, offset=%u, ", address, tag, index, offset);

    c->misses++;
	// handle compulsory misses 
    for (uint32_t i = 0; i < c->assoc; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = true;
            set->lines[i].tag = tag;
            
			if (c->repl == LRU) {
                LRU_Node* node = lru_create_node(i);
				if (!node) { 
					printf("\nerror: failed to create LRU node"); 
					exit(-1); 
				}
                set->lines[i].lru_node = node;
                lru_append(set, node);
            } else if (c->repl == FIFO) {
                if (fifo_enqueue(set->fifo, i) < 0) {
					printf("\nerror: failed to enqueue index\n\n");
                    exit(-1);
                }
            }

            if (!flag)
                printf("compulsory\n");
            c->miss_compulsory++;
			c->total_valid_lines++;
            return;
        }
    }

	// which line index to be replaced
    int victim_index = -1;
	switch (c->repl) {
		case RANDOM:
			victim_index = rand() % c->assoc;
			break;
		case LRU:
			if (set->lru_head)
				victim_index = set->lru_head->line_index;
			break;
		case FIFO:
			victim_index = fifo_dequeue(set->fifo);
			if (victim_index < 0) {
				printf("\nerror: FIFO failed\n\n");
				exit(-1);
			}
			break;
		default:
			break;
	}

	// actual repĺacement
    set->lines[victim_index].tag = tag;
    if (c->repl == LRU) {
        if (set->lines[victim_index].lru_node) {
    		lru_remove(set, set->lines[victim_index].lru_node);
    		lru_append(set, set->lines[victim_index].lru_node);
		}
    } else if (c->repl == FIFO) {
        if (fifo_enqueue(set->fifo, victim_index) < 0) { 
            printf("\n\nerror: FIFO enqueue\n\n"); 
			exit(-1);
        }
    }

	// when all lines are valid every miss is capacity miss
    if (c->total_valid_lines == c->n_sets * c->assoc) {
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
    for (uint32_t i = 0; i < c->n_sets; i++) {
        if (c->repl == LRU) {
            LRU_Node* node = c->sets[i].lru_head;
            while (node) {
                LRU_Node* tmp = node;
                node = node->next;
                free(tmp);
            }
        } else if (c->repl == FIFO && c->sets[i].fifo) {
            fifo_free(c->sets[i].fifo);
        }
        free(c->sets[i].lines);
    }
    free(c->sets);
    free(c);
}

LRU_Node* lru_create_node(int line_index) 
{
    LRU_Node* node = malloc(sizeof(LRU_Node));
    
	if (!node) return NULL;
    
	node->line_index = line_index;
    node->prev = NULL;
	node->next = NULL;
    return node;
}

void lru_append(Set* set, LRU_Node* node) 
{
    if (!set->lru_head) {
        set->lru_head = node; 
		set->lru_tail = node;
    } else {
        set->lru_tail->next = node;
        node->prev = set->lru_tail;
        set->lru_tail = node;
    }
}

void lru_remove(Set* set, LRU_Node* node) 
{
    if (node->prev)
        node->prev->next = node->next;
    else
        set->lru_head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        set->lru_tail = node->prev;

    node->prev = NULL;
	node->next = NULL;
}

FIFO_Queue* fifo_create(int capacity) 
{
    FIFO_Queue* q = malloc(sizeof(FIFO_Queue));

    if (!q) return NULL;
    
	q->queue = malloc(capacity * sizeof(int));
    
	if (!q->queue) { 
		free(q); 
		return NULL; 
	}
    
	q->front = 0;
    q->rear = -1;
    q->size = 0;
    q->capacity = capacity;
    return q;
}

void fifo_free(FIFO_Queue* q) 
{
    if (q) {
        free(q->queue);
        free(q);
    }
}

int fifo_enqueue(FIFO_Queue* q, int line_index) 
{
    q->rear = (q->rear + 1) % q->capacity;
    q->queue[q->rear] = line_index;
    q->size++;
    return 0;
}

int fifo_dequeue(FIFO_Queue* q) 
{
    int victim = q->queue[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return victim;
}

void usage(char* program_name)
{
	printf("\nUsage: %s [nsets] [bsize] [assoc] ", program_name);
	printf("[repl] [output flag] [input file]\n\n");
}

uint32_t swap_endian(uint32_t value) 
{
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

uint32_t convtoul(char *argv) 
{
	char *endptr;
	unsigned long value;

    value = strtoul(argv, &endptr, 10);

    return (uint32_t)value;
}
