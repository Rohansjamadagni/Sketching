#include <stdint.h>
#include <unordered_map>
#include <vector>

#ifndef _CS_H_
#define _CS_H_

#define START_SEED 1337
#define NUM_HASH_FUNCTION_PAIRS 5 // Must be odd for easy median calculation
#define CS_NUM_BUCKETS 10000
#define u64 uint64_t
#define i64 int64_t

#define MIN(X, Y) X < Y ? X : Y

typedef struct {
  u64 seed_main;
  u64 seed_sign;
} HashPair;

typedef struct {
  u64 count;
  u64 key;
} HeapItem;

typedef struct {
  HashPair seeds[NUM_HASH_FUNCTION_PAIRS];
  u64 k;
  i64 slots[NUM_HASH_FUNCTION_PAIRS][CS_NUM_BUCKETS];
  std::vector<HeapItem*> *heap;
  std::unordered_map<u64, HeapItem*> *map;
} CountSketch;

CountSketch* cs_init(u64 N, double phi);

bool cs_add(CountSketch* sketch, u64 item);

u64 cs_estimate(CountSketch* sketch, u64 item);

// MisraGries* mg_get_topk(MisraGries* sketch);

void cs_free(CountSketch* sketch);

// void mg_print_sketch_table(MisraGries* sketch);

u64 cs_size(CountSketch* sketch);

#endif
