#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "misra_gries.h"
#include <math.h>

#define ZETA_1_5 2.6123

MisraGries* mg_init(u64 N, double phi) {
  MisraGries* mg = (MisraGries*) malloc(sizeof(MisraGries));
  // K value is caluclated based on the reinmann's zeta function zeta(1.5), which
  // is our zipfian parameter is equal to 2.6123, we assume the universe size is
  // large here >> 10^5.
  mg->k = (u64) ceil(pow(1.0 / (phi * ZETA_1_5), 2.0/3.0));
  mg->k2 = mg->k * MG_MULT_FACTOR;
  printf("estimated k: %ld\n", mg->k);
  mg->map = new std::unordered_map<u64, u64>();
  return mg;
}

bool mg_add(MisraGries* sketch, u64 item) {
  // If there is space, or the element exists add one to counter.
  if (sketch->map->size() <= sketch->k2 || sketch->map->find(item) != sketch->map->end()){
    (*sketch->map)[item]++;
    return true;
  }
  // decrement all counters
  for (auto it = sketch->map->begin(); it != sketch->map->end(); ++it) {
    it->second--;
  }

  // remove keys who's counter reaches 0
  for (auto it = sketch->map->begin(); it != sketch->map->end();) {
    if (it->second == 0) {
      it = sketch->map->erase(it);
    } else {
      ++it;
    }
  }
  return true;
}

u64 mg_estimate(MisraGries* sketch, u64 item) {
  if (auto pair = sketch->map->find(item); pair != sketch->map->end()) {
    return pair->second;
  }
  return 0;
}

void mg_free(MisraGries* sketch) {
  delete sketch->map;
  free(sketch);
}

u64 mg_size(MisraGries* sketch) {
  u64 base = sizeof(MisraGries);
  return base + (sizeof(u64) * 2 * sketch->map->size());
}
