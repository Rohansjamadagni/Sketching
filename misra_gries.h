#include <stdint.h>
#include <unordered_map>

#ifndef _MG_H_
#define _MG_H_

#define START_SEED 42069
#define u64 uint64_t

#ifndef MG_MULT_FACTOR
#define MG_MULT_FACTOR 100
#endif

#define MIN(X, Y) X < Y ? X : Y

typedef struct {
  std::unordered_map<u64, u64> *map;
  u64 k;
  u64 k2;
} MisraGries;

MisraGries* mg_init(u64 N, double phi);

bool mg_add(MisraGries* sketch, u64 item);

u64 mg_estimate(MisraGries* sketch, u64 item);

// MisraGries* mg_get_topk(MisraGries* sketch);

void mg_free(MisraGries* sketch);

// void mg_print_sketch_table(MisraGries* sketch);

u64 mg_size(MisraGries* sketch);

#endif
