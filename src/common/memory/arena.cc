/** Copyright 2020-2021 Alibaba Group Holding Limited.
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

// #if defined(WITH_JEMALLOC)

#include "common/memory/arena.h"
#include <thread>

namespace vineyard {

namespace memory {

ArenaAllocator::ArenaAllocator()
    : num_arenas_(std::thread::hardware_concurrency()),
      empty_arenas_(num_arenas_, 0) {
  preAllocateArena();
}

ArenaAllocator::~ArenaAllocator() {
  if (extent_hooks_) {
    free(extent_hooks_);
  }
}

void* ArenaAllocator::Allocate(size_t size) {
  std::thread::id id = std::this_thread::get_id();
  unsigned arena_index;
  if (thread_arena_map_.find(id) == thread_arena_map_.end()) {
    arena_index = requestArena();
    if (arena_index == -1)
      return nullptr;
  } else {
    arena_index = thread_arena_map_[id];
  }

  return vineyard_je_mallocx(size, 0);
}

unsigned int ArenaAllocator::LookUp(void* ptr) {
  unsigned arena_index;
  size_t sz = sizeof(unsigned);
  if (auto ret = vineyard_je_mallctl("arenas.lookup", &arena_index, &sz, &ptr,
                                     sizeof(ptr))) {
    LOG(ERROR) << "failed to lookup arena";
  }
  return arena_index;
}

void ArenaAllocator::Free(void* ptr, size_t) {
  if (pointer) {
    vineyard_je_dallocx(pointer, 0);
  }
}

unsigned ArenaAllocator::ThreadTotalAllocatedBytes() {
  uint64_t allocated;
  size_t sz = sizeof(allocated);
  if (auto ret = vineyard_je_mallctl("thread.allocated",
                                     reinterpret_cast<void*> & allocated, &sz,
                                     NULL, 0)) {
    return -1;
  }
  return allocated;
}

unsigned ArenaAllocator::ThreadTotalDeallocatedBytes() {
  uint64_t deallocated;
  size_t sz = sizeof(deallocated);
  if (auto ret = vineyard_je_mallctl("thread.deallocated",
                                     reinterpret_cast<void*> & deallocated, &sz,
                                     NULL, 0)) {
    return -1;
  }
  return deallocated;
}

unsigned ArenaAllocator::requestArena() {
  std::thread::id id = std::this_thread::get_id();

  unsigned arena_index;
  {
    std::lock_guard<std::mutex> guard(arena_mutex);
    if (empty_arenas.empty()) {
      LOG(ERROR) << "All arenas used.";
      // TODO: recycle arena here
      return -1;
    }
    arena_index = empty_arenas.front();
    empty_arenas.pop_front();
  }
  LOG(INFO) << "Arena " << arena_index << " requested for thread " << id;
  {
    std::lock_guard<std::mutex> guard(thread_map_mutex_);
    thread_arena_map[id] = arena_index;
  }

  if (auto ret = vineyard_je_mallctl("thread.arena", NULL, NULL, &arena_index,
                                     sizeof(arena_index))) {
    LOG(ERROR) << "failed to bind arena " << arena_index << "for thread " << id;
    return -1;
  }

  return arena_index;
}

void ArenaAllocator::returnArena(unsigned arena_index) {
  std::thread::id id = std::this_thread::get_id();
  {
    std::lock_guard<std::mutex> guard(arena_mutex);
    empty_arenas.push_back(arena_index);
  }

  {
    std::lock_guard<std::mutex> guard(thread_map_mutex_);
    if (thread_arena_map.find(id) != thread_arena_map.end())
      thread_arena_map.erase(thread_arena_map.find(id));
  }
}

unsigned ArenaAllocator::doCreateArena() {
  unsigned arena_index;
  size_t sz = sizeof(unsigned);
  if (auto ret = vineyard_je_mallctl(
          "arenas.create", &arena_index, &sz,
          reinterpret_cast<void*>(extent_hooks_ != nullptr ? &extent_hooks_
                                                           : nullptr),
          (extent_hooks_ != nullptr ? sizeof(extent_hooks_) : 0))) {
    LOG(ERROR) << "failed to create arena";
  }
  return arena_index;
}

int ArenaAllocator::doDestroyArena(unsigned arena_index) {
  size_t mib[3];
  size_t miblen;

  miblen = sizeof(mib) / sizeof(size_t);
  if (auto ret =
          vineyard_je_mallctlnametomib("arena.0.destroy", mib, &miblen)) {
    LOG(ERROR) << "Unexpected mallctlnametomib() failure";
    return -1;
  }

  mib[1] = arena_index;
  if (auto ret = vineyard_je_mallctlbymib(mib, miblen, NULL, NULL, NULL, 0)) {
    LOG(ERROR) << "failed to destroy arena " << arena_index;
    return -1;
  }
  returnArena(arena_index);
  return 0;
}

int ArenaAllocator::doResetArena(unsigned arena_index) {
  size_t mib[3];
  size_t miblen;

  miblen = sizeof(mib) / sizeof(size_t);
  if (auto ret = vineyard_je_mallctlnametomib("arena.0.reset", mib, &miblen)) {
    LOG(ERROR) << "Unexpected mallctlnametomib() failure";
    return -1;
  }

  mib[1] = (size_t) arena_index;
  if (auto ret = vineyard_je_mallctlbymib(mib, miblen, NULL, NULL, NULL, 0)) {
    LOG(ERROR) << "failed to destroy arena";
    return -1;
  }
  return 0;
}

void ArenaAllocator::destroyAllArenas() {
  for (auto index : empty_arenas_) {
    doDestroyArena(index);
  }
  std::lock_guard<std::mutex> guard(arena_mutex_);
  empty_arenas_.clear();
  LOG(INFO) << "Arenas destroyed.";
}

void ArenaAllocator::resetAllArenas() {
  for (auto index : empty_arenas_) {
    doResetArena(index);
  }

  LOG(INFO) << "Arenas reseted.";
}

void ArenaAllocator::preAllocateArena() {
  for (int i = 0; i < num_arenas_; i++) {
    unsigned arena_index = doCreateArena();
    empty_arenas_[i] = arena_index;
    LOG(INFO) << "Arena " << arena_index << " created";
    // TODO: create TCACHE for each arena
  }
}

}  // namespace memory

}  // namespace vineyard
