#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "count_sketch.h"
#include "hashutil.h"
#include "min_heap.h"

#define ZETA_1_5 2.6123

CountSketch* cs_init(u64 N, double phi) {
  CountSketch *cs = (CountSketch*) malloc(sizeof(CountSketch));
  std::random_device rd;
  for (size_t i = 0; i < NUM_HASH_FUNCTION_PAIRS; ++i) {
    cs->seeds[i].seed_main = rd();
    cs->seeds[i].seed_sign = rd();
  }

  // K value is caluclated based on the reinmann's zeta function zeta(1.5), which
  // is our zipfian parameter is equal to 2.6123, we assume the universe size is
  // large here >> 10^5.
  cs->k = (u64) ceil(pow( 1.0 / (phi * ZETA_1_5), 2.0/3.0));
  printf("estimated k: %ld\n", cs->k);

  memset(cs->slots, 0, sizeof(cs->slots));
  cs->heap = new MinHeap(cs->k);
  return cs;
}

void cs_hash(HashPair* pair, u64 item, size_t *bucket, i64 *sign) {
  if (item > static_cast<u64>(std::numeric_limits<i64>::max())) {
    fprintf(stderr, "Unsigned value is out of range for int64 %ld", item);
    return;
  }

  *bucket = MurmurHash64A(&item, sizeof(u64), pair->seed_main) % CS_NUM_BUCKETS;
  if(MurmurHash64A(&item, sizeof(u64), pair->seed_sign) % 2 == 0) {
    *sign = -1;
  } else {
    *sign = 1;
  }
}

bool cs_add(CountSketch* sketch, u64 item) {
  size_t bucket;
  i64 sign = 1;
  for (size_t i = 0; i < NUM_HASH_FUNCTION_PAIRS; ++i) {
    cs_hash(&sketch->seeds[i], item, &bucket, &sign);
    if(bucket > CS_NUM_BUCKETS) {
      fprintf(stderr, "Couldn't add item to count sketch\n");
      return false;
    }
    sketch->slots[i][bucket] += sign;
  }
  u64 count = cs_estimate(sketch, item);
  sketch->heap->insertOrUpdate(item, count);
  return true;
}

int cs_i64_compare(const void* a, const void* b) {
    const i64 ai = *( const i64* )a;
    const i64 bi = *( const i64* )b;

    if(ai < bi)
    {
        return -1;
    }
    else if(ai > bi)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

u64 cs_estimate(CountSketch* sketch, u64 item) {
  size_t bucket;
  i64 sign = 1;
  i64 counts[NUM_HASH_FUNCTION_PAIRS];
  for (size_t i = 0 ; i < NUM_HASH_FUNCTION_PAIRS; ++i) {
    cs_hash(&sketch->seeds[i], item, &bucket, &sign);
    if(bucket > CS_NUM_BUCKETS) {
      fprintf(stderr, "Couldn't estimate item count\n");
      return false;
    }
    counts[i] = sketch->slots[i][bucket];
  }
  std::nth_element(counts, counts + NUM_HASH_FUNCTION_PAIRS/2,
                   counts + NUM_HASH_FUNCTION_PAIRS);

  i64 median = counts[NUM_HASH_FUNCTION_PAIRS/2];
  if (median > 0){
    return (u64) median;
  }

  return 0;
}

void cs_free(CountSketch* sketch) {
  delete sketch->heap;
  free(sketch);
}

u64 cs_size(CountSketch* sketch) {
  u64 base = sizeof(CountSketch);
  printf("Size of Sketch without heap: %ld\n", base);
  base += sketch->heap->size();
  return base;
}
