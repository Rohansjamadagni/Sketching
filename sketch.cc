#include "count_min_sketch.h"
#include "sc_min_heap.h"
#include "count_sketch.h"
#include <cstdio>
#include <functional>
#include <map>
#include "sketch.h"
#include "uthash.h"
#include "misra_gries.h"


Sketch::Sketch(u64 N, double phi, SketchType type) : N(N), phi(phi), type(type) {
  switch(type) {
    case SketchType::CMS: backend = cms_init(N, phi); break;
    case SketchType::CS: backend = cs_init(N, phi); break;
    case SketchType::MG: backend = mg_init(N, phi); break;
  }
}

void Sketch::Add(u64 item) {
  switch(type) {
    case SketchType::CMS: cms_add(static_cast<CountMinSketch*>(backend), item); break;
    case SketchType::CS:  cs_add(static_cast<CountSketch*>(backend), item); break;
    case SketchType::MG:  mg_add(static_cast<MisraGries*>(backend), item); break;
  }
}

u64 Sketch::Estimate(u64 item) {
  switch(type) {
    case SketchType::CMS: return cms_estimate(static_cast<CountMinSketch*>(backend), item);
    case SketchType::CS:  return cs_estimate(static_cast<CountSketch*>(backend), item);
    case SketchType::MG: return mg_estimate(static_cast<MisraGries*>(backend), item);
  }
  return 0;
}

u64 Sketch::Size() {
  switch(type) {
    case SketchType::CMS: return cms_size(static_cast<CountMinSketch*>(backend));
    case SketchType::CS:  return cs_size(static_cast<CountSketch*>(backend));
    case SketchType::MG:  return mg_size(static_cast<MisraGries*>(backend));
  }
  return 0;
}

std::multimap<u64, u64, std::greater<u64>> Sketch::HeavyHitters(double phi) {
  // u64 phin = static_cast<u64>(phi * (double)N);
  struct sc_heap* min_heap;
  std::multimap<u64, u64, std::greater<u64>> topK;
  switch(type) {
    case SketchType::CMS: {
                            min_heap =  cms_get_topk(static_cast<CountMinSketch*>(backend));
                            struct sc_ht_entry* ht = min_heap->ht;
                            sc_ht_entry *current, *tmp;
                            HASH_ITER(hh, ht, current, tmp) {
                                topK.insert({
                                    min_heap->elems[current->heap_index].key, // Count
                                    current->data, // Value
                                    });
                            }
                            break;
                          }
    case SketchType::CS: {
                           CountSketch *cs = static_cast<CountSketch*>(backend);
                           for (size_t i = 0; i < cs->heap->size(); ++i) {
                             if (cs->heap->at(i) == NULL) continue;
                             topK.insert({
                                    cs->heap->at(i)->count,
                                    cs->heap->at(i)->key
                                 });
                           }
                          break;
                         }
    case SketchType::MG: {
                           MisraGries *mg = static_cast<MisraGries*>(backend);
                           for (auto it = mg->map->begin(); it != mg->map->end(); ++it) {
                             topK.insert({
                                    it->second,
                                    it->first,
                                 });
                           }
                           break;
                         }
  }


  return topK;
}

Sketch::~Sketch() {
  switch(type) {
    case SketchType::CMS: cms_free(static_cast<CountMinSketch*>(backend)); break;
    case SketchType::CS:  cs_free(static_cast<CountSketch*>(backend)); break;
    case SketchType::MG:  mg_free(static_cast<MisraGries*>(backend)); break;
  }
}
