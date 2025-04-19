#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "count_sketch.h"
#include "hashutil.h"

CountSketch* cs_init(u64 N, double phi) {
  CountSketch *cs = (CountSketch*) malloc(sizeof(CountSketch));
  u64 start_seed = START_SEED;
  for (size_t i = 0; i < NUM_HASH_FUNCTION_PAIRS; ++i) {
    cs->seeds[i].seed_main = start_seed++;
    cs->seeds[i].seed_sign = start_seed++;
  }

  cs->k = 52;
  memset(cs->slots, 0, sizeof(cs->slots));
  cs->heap = new std::vector<HeapItem*>();
  cs->map = new std::unordered_map<u64, HeapItem*>(cs->k);
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

bool cs_heap_compare(const HeapItem* a, const HeapItem* b) {
  if(a == NULL) return false;
  if(b == NULL) return true;
  return a->count > b->count;
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
  if (!count) return false;
  if (sketch->heap->size() <= sketch->k) {
    if (auto it = sketch->map->find(item); it != sketch->map->end()){
      // the item already exists, we need to increment counter and reheapify
      HeapItem *existingItem = (it->second);
      existingItem->count++;
      std::make_heap(sketch->heap->begin(), sketch->heap->end(), cs_heap_compare);
    } else {
      HeapItem* newItem = (HeapItem*)malloc(sizeof(HeapItem));
      newItem->count = count;
      newItem->key = item;
      (*sketch->map)[item] = newItem; // put it into map
      sketch->heap->push_back(newItem); // push and heapify the element
      std::push_heap(sketch->heap->begin(), sketch->heap->end(), cs_heap_compare);
    }
  } else {
    HeapItem* smallest = *(sketch->heap->begin());
    if (smallest == NULL) return false;
    if (count > smallest->count) {
      // we need to pop the top, remove it from map, free it, and push the new
      // item into heap and map.
      std::pop_heap(sketch->heap->begin(), sketch->heap->end(), cs_heap_compare);
      smallest = sketch->heap->back();
      sketch->heap->pop_back();

      sketch->map->erase(smallest->key); //TODO: may not work.

      // We could have freed and malloced, but I'm just going to reuse.
      smallest->count = count;
      smallest->key = item;

      sketch->heap->push_back(smallest); // push and heapify the element
      std::push_heap(sketch->heap->begin(), sketch->heap->end(), cs_heap_compare);
      (*sketch->map)[item] = smallest; // put it into map
    }
  }
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
  qsort(counts, NUM_HASH_FUNCTION_PAIRS, sizeof(u64*), cs_i64_compare);

  i64 median = counts[NUM_HASH_FUNCTION_PAIRS/2];
  if (median > 0){
    return (u64) median;
  }

  return 0;
}

// CountSketch* mg_get_topk(CountSketch* sketch);

void cs_free(CountSketch* sketch) {
  // TODO: we should free each item in the heap
  delete sketch->heap;
  delete sketch->map;
  free(sketch);
}

// void mg_print_sketch_table(CountSketch* sketch);

u64 cs_size(CountSketch* sketch) {
  u64 base = sizeof(CountSketch);
  // return base + (sizeof(u64) * 2  sketch->slots->size());
  // TODO: fix size here
  return base;
}
