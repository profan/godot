#ifndef GODOT_BENCH_H
#define GODOT_BENCH_H

#include <thread>
#include <vector>
#include <unordered_map>

extern std::unordered_map<size_t, size_t> cowdata_sizes;
extern std::mutex cowdata_sizes_lock;
void add_cowdata_size(int p_size);

#endif