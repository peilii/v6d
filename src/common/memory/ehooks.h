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

#ifndef SRC_COMMON_MEMORY_EXTENT_HOOKS_H_
#define SRC_COMMON_MEMORY_EXTENT_HOOKS_H_

#if defined(WITH_JEMALLOC)

namespace vineyard {

namespace memory {

static inline void* AllocHook(extent_hooks_t* extent_hooks, void* new_addr,
                             size_t size, size_t alignment, bool* zero,
                             bool* commit, unsigned arena_index);

static inline bool DallocHook(extent_hooks_t *extent_hooks, void *addr, 
                             size_t size, bool committed, unsigned arena_ind);

static inline void DestroyHook(extent_hooks_t *extent_hooks, void *addr, size_t size,
                            bool committed, unsigned arena_ind);

static inline bool CommitHook(extent_hooks_t *extent_hooks, void *addr, size_t size,
                            size_t offset, size_t length, unsigned arena_ind);

static inline bool DecommitHook(extent_hooks_t *extent_hooks, void *addr, size_t size,
                                 size_t offset, size_t length, unsigned arena_ind);

static inline bool PurgeLazyHook(tsdn_t *tsdn, ehooks_t *ehooks, void *addr, size_t size,
                                 size_t offset, size_t length);

static inline bool PurgeForcedHook(tsdn_t *tsdn, ehooks_t *ehooks, void *addr, size_t size,
                                 size_t offset, size_t length);

static inline bool MergeHook(tsdn_t *tsdn, ehooks_t *ehooks, void *addr_a, size_t size_a,
                                 bool head_a, void *addr_b, size_t size_b, bool head_b, bool committed);

static inline void ZeroHook(tsdn_t *tsdn, ehooks_t *ehooks, void *addr, size_t size);

}  // namespace memory

}  // namespace vineyard

#endif  // WITH_JEMALLOC

#endif  // SRC_COMMON_MEMORY_EXTENT_HOOKS_H_