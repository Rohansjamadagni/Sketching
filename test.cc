// Author: Prashant Pandey <prashant.pandey@utah.edu>
// For use in CS6968 & CS5968

#include <iostream>
#include <cassert>
#include <chrono>
#include <openssl/rand.h>
#include <map>
#include <unordered_map>

#include "zipf.h"
#include "sketch.h"

using namespace std::chrono;

#define UNIVERSE 1ULL << 30
#define EXP 1.5

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

	Sketch s = Sketch(N, phi, SketchType::CS);

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

	auto iterator1 = topK.begin();
	auto iterator2 = sketch_topK.begin();

	double tp = 0, fp = 0, fn = 0;

	while (iterator1 != topK.end() && iterator2 != sketch_topK.end()) {
		if (iterator1->second == iterator2->second) {
			tp++;
			iterator1++;
			iterator2++;
		} else if (iterator1->first > iterator2->first) {
			fn++;
			iterator1++;
		} else if (iterator1->first < iterator2->first) {
			fp++;
			iterator2++;
		} else {
			if (sketch_topK.find(iterator1->second) != sketch_topK.end()) {
				tp++;
				iterator1++;
			} else {
				fn++;
				iterator1++;
			}
		}
	}

	double precision = tp / (tp + fp), recall = tp / (tp + fn);

  printf("precision: %0.02f percent\nrecall: %0.02f percent\n", precision*100, recall*100);

	return 0;
}

