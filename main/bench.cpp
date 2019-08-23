#include "bench.h"

/* Benchmarking */
std::unordered_map<size_t, size_t> cowdata_sizes;
std::mutex cowdata_sizes_lock;

void add_cowdata_size(int p_size) {

	cowdata_sizes_lock.lock();

	auto v = cowdata_sizes.find(p_size);
	if (v != cowdata_sizes.end()) {
		v->second++;
	} else {
		v[p_size] = 1;
	}

	cowdata_sizes_lock.unlock();

}