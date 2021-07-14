/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <algorithm>
#include <iterator>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <memory>
#include <string>
#include <benchmark/benchmark.h>
#include "gtest/gtest.h"

#include "common/util/env.h"
#include "server/memory/dlmalloc.h"
#include "server/memory/jemalloc.h"
#include "common/memory/thread_local_arena.h"
#include "common/memory/thread_local_arena_allocator.h"

#define N (1 << 10)
#define S (1 << 5)

using namespace vineyard;  // NOLINT(build/namespaces)

namespace memory {
class DLmallocAllocator;
class JemallocAllocator;
}  // namespace memory

namespace allocator_benchmark {

static void* NewDeleteSimple(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < N; ++i) {
            char *p = static_cast<char *>(::operator new(S));
            ++(*p);
            ::operator delete(p);
        }
    }
}

BENCHMARK(NewDeleteSimple);

static void* DLMallocSimple(benchmark::State& state) {
    memory::DLmallocAllocator allocator;
    for (auto _ : state) {
        for (int i = 0; i < N; ++i) {
            char *p = static_cast<char *>(allocator.allocate(S));
            ++(*p);
            ::operator delete(p);
        }
    }
}

BENCHMARK(DLMallocSimple);

static void* JeMallocSimple(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < N; ++i) {
            char *p = static_cast<char *>(allocator.allocate(S));
            ++(*p);
            ::operator delete(p);
        }
    }
}

BENCHMARK(JeMallocSimple);

BENCHMARK_MAIN();

}
