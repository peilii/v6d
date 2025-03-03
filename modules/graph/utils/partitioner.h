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

#ifndef MODULES_GRAPH_UTILS_PARTITIONER_H_
#define MODULES_GRAPH_UTILS_PARTITIONER_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "flat_hash_map/flat_hash_map.hpp"
#if defined(EXPERIMENTAL_ON) || defined(NETWORKX)
#include "folly/dynamic.h"
#endif

#include "graph/fragment/property_graph_types.h"

namespace vineyard {

// TODO(lxj): check if identical to the file in libgrape-lite
template <typename OID_T>
class HashPartitioner {
 public:
  using oid_t = OID_T;

  HashPartitioner() : fnum_(1) {}

  void Init(fid_t fnum) { fnum_ = fnum; }

  inline fid_t GetPartitionId(const OID_T& oid) const {
    return static_cast<fid_t>(static_cast<uint64_t>(oid) % fnum_);
  }

  HashPartitioner& operator=(const HashPartitioner& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

  HashPartitioner(const HashPartitioner& other) { fnum_ = other.fnum_; }

  HashPartitioner& operator=(HashPartitioner&& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

 private:
  fid_t fnum_;
};

template <>
class HashPartitioner<std::string> {
 public:
  using oid_t = std::string;

  HashPartitioner() : fnum_(1) {}

  void Init(fid_t fnum) { fnum_ = fnum; }

  inline fid_t GetPartitionId(const oid_t& oid) const {
    return static_cast<fid_t>(
        static_cast<uint64_t>(std::hash<std::string>()(oid)) % fnum_);
  }

  HashPartitioner& operator=(const HashPartitioner& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

  HashPartitioner(const HashPartitioner& other) { fnum_ = other.fnum_; }

  HashPartitioner& operator=(HashPartitioner&& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

 private:
  fid_t fnum_;
};

#if defined(EXPERIMENTAL_ON) || defined(NETWORKX)
template <>
class HashPartitioner<folly::dynamic> {
 public:
  using oid_t = folly::dynamic;

  HashPartitioner() : fnum_(1) {}

  void Init(fid_t fnum) { fnum_ = fnum; }

  inline fid_t GetPartitionId(const oid_t& oid) const {
    size_t hash_value;
    if (oid.isArray() && oid.size() == 2) {
      // the oid is (label, id) format, just use id to get hash value.
      hash_value = hash(oid[1]);
    } else {
      hash_value = hash(oid);
    }
    return static_cast<fid_t>(static_cast<uint64_t>(hash_value) % fnum_);
  }

  HashPartitioner& operator=(const HashPartitioner& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

  HashPartitioner& operator=(HashPartitioner&& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    return *this;
  }

 private:
  size_t hash(const oid_t& oid) const {
    if (oid.isString()) {
      return std::hash<std::string>()(oid.getString());
    } else {
      return std::hash<folly::dynamic>()(oid);
    }
  }

  fid_t fnum_;
};
#endif  // NETWORKX || EXPERIMENTAL_ON

template <typename OID_T>
class SegmentedPartitioner {
 public:
  using oid_t = OID_T;

  SegmentedPartitioner() : fnum_(1) {}

  void Init(fid_t fnum, const std::vector<OID_T>& oid_list) {
    fnum_ = fnum;
    size_t vnum = oid_list.size();
    size_t frag_vnum = (vnum + fnum_ - 1) / fnum_;
    o2f_.reserve(vnum);
    for (size_t i = 0; i < vnum; ++i) {
      fid_t fid = static_cast<fid_t>(i / frag_vnum);
      o2f_.emplace(oid_list[i], fid);
    }
  }

  inline fid_t GetPartitionId(const OID_T& oid) const { return o2f_.at(oid); }

  SegmentedPartitioner& operator=(const SegmentedPartitioner& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    o2f_ = other.o2f_;
    return *this;
  }

  SegmentedPartitioner& operator=(SegmentedPartitioner&& other) {
    if (this == &other) {
      return *this;
    }
    fnum_ = other.fnum_;
    o2f_ = std::move(other.o2f_);
    return *this;
  }

 private:
  fid_t fnum_;
  ska::flat_hash_map<OID_T, fid_t> o2f_;
};

}  // namespace vineyard

#endif  // MODULES_GRAPH_UTILS_PARTITIONER_H_
