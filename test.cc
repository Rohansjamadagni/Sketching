// Author: Prashant Pandey <prashant.pandey@utah.edu>
// For use in CS6968 & CS5968

#include <cstring>
#include <iostream>
#include <cassert>
#include <chrono>
#include <openssl/rand.h>
#include <map>
#include <unordered_map>
#include <math.h>

#include "zipf.h"
#include "sketch.h"

using namespace std::chrono;

#define UNIVERSE 1ULL << 30
#define EXP 1.5
#define COUNT_ERROR_THRESHOLD 0.05

double elapsed(high_resolution_clock::time_point t1, high_resolution_clock::time_point t2) {
	return (duration_cast<duration<double> >(t2 - t1)).count();
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "Specify the number of items N and phi.\n";
		exit(1);
	}
	uint64_t N = atoi(argv[1]);
	double phi = atof(argv[2]);
  SketchType sketch_type = SketchType::MG;

  if (argc == 4) {
    if (strncmp(argv[3], "cms", 2) == 0) {
      std::cout << "Sketch Type: Count Min Sketch\n";
      sketch_type = SketchType::CMS;
    } else if (strncmp(argv[3], "cs", 2) == 0) {
      std::cout << "Sketch Type: Count Sketch\n";
      sketch_type = SketchType::CS;
    } else {
      std::cout << "Sketch Type: Misra Gries\n";
      sketch_type = SketchType::MG;
    }
  }
	uint64_t *numbers = (uint64_t *)malloc(N * sizeof(uint64_t));
	if(!numbers) {
		std::cerr << "Malloc numbers failed.\n";
		exit(0);
	}
	high_resolution_clock::time_point t1, t2;
	t1 = high_resolution_clock::now();
	generate_random_keys(numbers, UNIVERSE, N, EXP);
	t2 = high_resolution_clock::now();
	std::cout << "Time to generate " << N << " items: " << elapsed(t1, t2) << " secs\n";

	std::unordered_map<uint64_t, uint64_t> map(N);

	t1 = high_resolution_clock::now();
	for (uint64_t i = 0; i < N; ++i)
		map[numbers[i]]++;
	t2 = high_resolution_clock::now();
	std::cout << "Time to count " << N << " items: " << elapsed(t1, t2) << " secs\n";

	// Compute heavy hitters
	uint64_t total = 0;
  uint64_t numK = 0;
	double threshold = phi * N;
	std::multimap<uint64_t, uint64_t, std::greater<uint64_t> > topK;
	t1 = high_resolution_clock::now();
	for (auto it = map.begin(); it != map.end(); ++it) {
		if (it->second >= threshold) {
			topK.insert(std::make_pair(it->second, it->first));
      numK++;
		}
		/*
		if (topK.size() < K) {
			topK.insert(std::make_pair(it->second, it->first));
		} else {
			if (std::prev(topK.end())->first < it->second) {
				// the last item has lower freq than the new item
				topK.erase(std::prev(topK.end())); // delete the last item
				topK.insert(std::make_pair(it->second, it->first));
			}
		}
		*/
		total += it->second;
	}
	t2 = high_resolution_clock::now();
	std::cout << "Time to compute phi-heavy hitter items: " << elapsed(t1, t2) << " secs\n";
	std::cout << "Real K value: " <<  numK << "\n";
	assert(total == N);

	// free(map);

	Sketch s = Sketch(N, phi, sketch_type);

	t1 = high_resolution_clock::now();
	for (uint64_t i = 0; i < N; ++i) {
		s.Add(numbers[i]);
	}
	t2 = high_resolution_clock::now();
	std::cout << "Time to stream items into sketch: " << elapsed(t1, t2) << " secs\n";
	free(numbers); // free stream

	t1 = high_resolution_clock::now();
	std::multimap<uint64_t, uint64_t, std::greater<uint64_t> > sketch_topK = s.HeavyHitters(phi);
	t2 = high_resolution_clock::now();
	std::cout << "Time to compute phi heavy hitters: " << elapsed(t1, t2) << " secs\n";

    std::unordered_map<uint64_t, uint64_t> true_counts;
    for (const auto& pair : topK) {
        true_counts[pair.second] = pair.first;  // element -> true count
    }

    std::unordered_map<uint64_t, uint64_t> estimated_counts;
    for (const auto& pair : sketch_topK) {
        estimated_counts[pair.second] = pair.first;  // element -> estimated count
    }

    double tp = 0, fp = 0, fn = 0;

    // Calculate True Positives (matching elements with count error < threshold)
    for (const auto& [element, est_count] : estimated_counts) {
        if (true_counts.count(element)) {
            uint64_t true_count = true_counts[element];
            double error = std::abs((double)est_count - (double)true_count) / true_count;
            if (error <= COUNT_ERROR_THRESHOLD) {
                tp++;
            }
        }
    }

    // False Positives = total estimated - TP
    fp = estimated_counts.size() - tp;

    // False Negatives = total true - TP
    fn = true_counts.size() - tp;

    double precision = tp / (tp + fp);
    double recall = tp / (tp + fn);

    printf("True Positives: %f\t False Positives: %f\t"
           "False Negatives: %f\n", tp, fp, fn);

    printf("Size of Sketch in Bytes: %ld\n", s.Size());
    printf("precision: %0.02f percent\n", precision*100);
    printf("recall: %0.02f percent\n", recall*100);

	return 0;
}

