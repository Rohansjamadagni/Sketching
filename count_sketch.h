#include "min_heap.h"
#include <stdint.h>
#include <unordered_map>
#include <vector>

#ifndef _CS_H_
#define _CS_H_

#define START_SEED 42069

#ifndef NUM_HASH_FUNCTION_PAIRS
#define NUM_HASH_FUNCTION_PAIRS 5 // Must be odd for easy median calculation
#endif

#ifndef CS_NUM_BUCKETS
#define CS_NUM_BUCKETS 2048
#endif

#define u64 uint64_t
#define i64 int64_t

#define MIN(X, Y) X < Y ? X : Y

typedef struct {
  u64 seed_main;
  u64 seed_sign;
} HashPair;

typedef struct {
  HashPair seeds[NUM_HASH_FUNCTION_PAIRS];
  u64 k;
  i64 slots[NUM_HASH_FUNCTION_PAIRS][CS_NUM_BUCKETS];
  MinHeap* heap;
} CountSketch;

CountSketch* cs_init(u64 N, double phi);

bool cs_add(CountSketch* sketch, u64 item);

u64 cs_estimate(CountSketch* sketch, u64 item);

// MisraGries* mg_get_topk(MisraGries* sketch);

void cs_free(CountSketch* sketch);

// void mg_print_sketch_table(MisraGries* sketch);

u64 cs_size(CountSketch* sketch);

#endif
