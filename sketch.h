#ifndef SKETCH_H
#define SKETCH_H

#include "count_min_sketch.h"
#include <cstdint>
#include <functional>
#include <map>

enum class SketchType { CMS, CS, MG };

class Sketch {
private:
    void* backend;
    u64 N;
    double phi;
    SketchType type;

public:
    Sketch(u64 N, double phi, SketchType type);
    void Add(u64 item);
    u64 Estimate(u64 item);
    u64 Size();
    std::multimap<u64, u64, std::greater<u64>> HeavyHitters(double phi);
    ~Sketch();
};

#endif
