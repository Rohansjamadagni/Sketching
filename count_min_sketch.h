#include "min_heap.h"
#include <stdint.h>

#ifndef _CMS_H_
#define _CMS_H_

#ifndef NUM_HASH_FUNCTIONS
#define NUM_HASH_FUNCTIONS 5
#endif

#ifndef NUM_BUCKETS
#define NUM_BUCKETS 2048 // Must be power of two
#endif

#define START_SEED 42069
#define HEAP_START_CAP NUM_BUCKETS
#define u64 uint64_t

#define MIN(X, Y) X < Y ? X : Y

typedef struct {
  u64 m[NUM_HASH_FUNCTIONS]; // hash seeds array
  u64 k; // used for storing k heavy hitters
  u64 slots[NUM_HASH_FUNCTIONS][NUM_BUCKETS]; // slot values can be negative
  MinHeap *heap;
} CountMinSketch;


CountMinSketch* cms_init(u64 N, double phi);

bool cms_add(CountMinSketch* sketch, u64 item);

u64 cms_estimate(CountMinSketch* sketch, u64 item);

void cms_free(CountMinSketch* sketch);

void cms_print_sketch_table(CountMinSketch* sketch);

u64 cms_size(CountMinSketch* sketch);

#endif
