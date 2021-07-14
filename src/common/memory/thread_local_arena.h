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

#ifndef SRC_COMMON_MEMORY_THREAD_LOCAL_ARENA_H_
#define SRC_COMMON_MEMORY_THREAD_LOCAL_ARENA_H_

#if defined(WITH_THREAD_LOCAL_ARENA_)

#include "server/memory/malloc.h"
#include "common/memory/jemalloc.h"

namespace vineyard {

namespace memory {

class ThreadLocalArena {
public:
    ThreadLocalArena();
    ~ThreadLocalArena();

    int Init(size_t size);

    void* Reserve(size_t size, size_t alignment);

    bool Owns(void* address);

    size_t FreeSpace() const;

    size_t TotalSpace() const;

    unsigned arenaIndex();

    void* Allocate(size_t size);

    void Deallocate(void* address, size_t size = 0);

    void Merge();

    void Clear();

private:
    bool IsAligned(void* address) const;
    size_t RoundUp(size_t size) const;

private:
    uintptr_t start_{0};
    uintptr_t end_{0};
    uintptr_t free_ptr_{0};
    unsigned arena_index_{0};
    extent_hooks_t extent_hooks_;
};

}  // namespace memory

}  // namespace vineyard

#endif  // WITH_THREAD_LOCAL_ARENA_

#endif  // SRC_COMMON_MEMORY_THREAD_LOCAL_ARENA_H_