#include "count_min_sketch.h"
#include "min_heap.h"
#include "count_sketch.h"
#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include "sketch.h"
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
  std::multimap<u64, u64, std::greater<u64>> topK;
  switch(type) {
    case SketchType::CMS: {
                           CountMinSketch *cms = static_cast<CountMinSketch*>(backend);
                           std::vector<HeapElement> items = cms->heap->getTopK();
                           for (size_t i = 0; i < items.size(); ++i) {
                             topK.insert({
                                    items.at(i).item,
                                    items.at(i).count
                                 });
                           }
                            break;
                          }
    case SketchType::CS: {
                           CountSketch *cs = static_cast<CountSketch*>(backend);
                           std::vector<HeapElement> items = cs->heap->getTopK();
                           for (size_t i = 0; i < items.size(); ++i) {
                             topK.insert({
                                    items.at(i).item,
                                    items.at(i).count
                                 });
                           }
                          break;
                         }
    case SketchType::MG: {
                           MisraGries *mg = static_cast<MisraGries*>(backend);
                           std::vector<std::pair<u64, u64>> pairs;
                           for (const auto& pair : *mg->map) {
                             pairs.push_back(pair);
                           }

                           std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
                             return a.second > b.second;
                           });
                           for (size_t i = 0; i < mg->k; ++i) {
                             topK.insert({
                                    pairs.at(i).first,
                                    pairs.at(i).second,
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
