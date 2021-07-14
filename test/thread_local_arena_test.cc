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

#include "common/memory/thread_local_arena.h"
#include "common/memory/thread_local_arena_allocator.h"

using namespace vineyard;  // NOLINT(build/namespaces)

class ThreadLocalArenaTest {
public:
    explicit ThreadLocalArenaTest(ThreadLocalArena& arena) : arena_(arena) {}

    void Allocate(size_t size);

    void Deallocate(void* address);

    void Verify();

private:
    ThreadLocalArena arena_;

};


int main(int argc, char** argv) {
    return 0;
}