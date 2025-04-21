#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "min_heap.h"
#include "sketch.h"
#include "count_min_sketch.h"
#include "hashutil.h"

#define ZETA_1_5 2.6123

CountMinSketch* cms_init(u64 N, double phi) {
  CountMinSketch* sketch = (CountMinSketch*)malloc(sizeof(CountMinSketch));
  if (!sketch) {
    fprintf(stderr, "Unable to allocate memory for sketch");
    exit(1);
  }
  if (phi == 0.0) {
    fprintf(stderr, "Phi value can not be zero");
    exit(1);
  }
  // K value is caluclated based on the reinmann's zeta function zeta(1.5), which
  // is our zipfian parameter is equal to 2.6123, we assume the universe size is
  // large here >> 10^5.
  sketch->k = (u64) floor(pow( 1.0 / (phi * ZETA_1_5), 2.0/3.0));
  printf("estimated k: %ld\n", sketch->k);

  for (u64 i = 0; i < NUM_HASH_FUNCTIONS; ++i) sketch->m[i] = i + START_SEED;

  memset(sketch->slots, 0 , sizeof(sketch->slots));

  sketch->heap = new MinHeap(sketch->k);
  return sketch;
}

bool cms_add(CountMinSketch* sketch, u64 item) {
  u64 count = UINT64_MAX;
  for (size_t i = 0 ; i < NUM_HASH_FUNCTIONS; ++i) {
    u64 index = MurmurHash64A(&item, sizeof(u64), sketch->m[i]) % NUM_BUCKETS;
    // printf("Key: %ld Seed: %ld Index: %ld\n", item, sketch->m[i], index);
    if(index > NUM_BUCKETS) {
      fprintf(stderr, "Couldn't add item to count sketch\n");
      return false;
    }
    sketch->slots[i][index] += 1;
    count = MIN(count, sketch->slots[i][index]);
  }

  sketch->heap->insertOrUpdate(item, count);
  return true;
}

u64 cms_estimate(CountMinSketch* sketch, u64 item) {
  u64 min = UINT64_MAX;
  for (size_t i = 0 ; i < NUM_HASH_FUNCTIONS; ++i) {
    u64 index = MurmurHash64A(&item, sizeof(u64), sketch->m[i]) % NUM_BUCKETS;
    if(index > NUM_BUCKETS) {
      fprintf(stderr, "Couldn't estimate item count\n");
      return false;
    }
    min = MIN(min, sketch->slots[i][index]);
  }

  return min;
}

void cms_free(CountMinSketch* sketch) {
  delete sketch->heap;
  free(sketch);
}

void cms_print_sketch_table(CountMinSketch* sketch) {
  for (size_t i = 0; i < NUM_HASH_FUNCTIONS ; ++i) {
    for (size_t j = 0; j < NUM_BUCKETS; ++j)
      printf("%ld    " , sketch->slots[i][j]);
    printf("\n");
  }
}

u64 cms_size(CountMinSketch* sketch) {
  u64 base = sizeof(*sketch);
  printf("Size of Sketch without heap: %ld\n", base);
  base += sketch->heap->size();
  return base;
}

// int main(int argc, char** argv) {
//   if (argc != 3) {
//     fprintf(stderr, "Usage: ./sketch N PHI\n");
//     exit(1);
//   }
//
//   u64 n = atoll(argv[1]);
//   double phi = atof(argv[2]);
//   printf("PHI: %f\n", phi);
//   CountMinSketch *sketch = cms_init(n, phi);
//   u64 start = START_SEED;
//
//   for (size_t i = 0; i < n; ++i) {
//     cms_add(sketch, i*START_SEED*START_SEED);
//   }
//   printf("\n\n");
//   for (size_t i = 0; i < n; ++i) {
//     cms_estimate(sketch, START_SEED*START_SEED);
//   }
//
//   u64 hh[10] = {0};
//   u64 numh = cms_heavy_hitters(sketch, phi*n, hh, 10);
//   printf("Heavy hitters: \n");
//   for (size_t i = 0; i < numh; ++i) {
//     printf("%ld ",hh[i]);
//   }
//   printf("\n");
//
//   cms_print_sketch_table(sketch);
//
//   printf("Size in bytes: %ld\n", cms_size(sketch));
//
//   cms_free(sketch);
//
//   return 0;
// }
