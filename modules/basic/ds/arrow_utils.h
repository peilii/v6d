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

#ifndef MODULES_BASIC_DS_ARROW_UTILS_H_
#define MODULES_BASIC_DS_ARROW_UTILS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "arrow/array.h"
#include "arrow/array/builder_binary.h"
#include "arrow/array/builder_nested.h"
#include "arrow/array/builder_primitive.h"
#include "arrow/buffer.h"
#include "arrow/io/memory.h"
#include "arrow/ipc/reader.h"
#include "arrow/ipc/writer.h"
#include "arrow/table.h"
#include "arrow/table_builder.h"
#include "arrow/util/config.h"
#include "glog/logging.h"

#include "basic/ds/types.h"
#include "common/util/status.h"

namespace vineyard {

namespace arrow_types {
using OIDT = std::string;
using VIDT = uint64_t;
using EIDT = uint64_t;
}  // namespace arrow_types

struct RefString;

#define CHECK_ARROW_ERROR(expr) \
  VINEYARD_CHECK_OK(::vineyard::Status::ArrowError(expr))

#define CHECK_ARROW_ERROR_AND_ASSIGN(lhs, expr) \
  do {                                          \
    auto status = (expr);                       \
    CHECK_ARROW_ERROR(status.status());         \
    lhs = std::move(status).ValueOrDie();       \
  } while (0)

#define RETURN_ON_ARROW_ERROR(expr)                  \
  do {                                               \
    auto status = (expr);                            \
    if (!status.ok()) {                              \
      return ::vineyard::Status::ArrowError(status); \
    }                                                \
  } while (0)

#define RETURN_ON_ARROW_ERROR_AND_ASSIGN(lhs, expr)           \
  do {                                                        \
    auto result = (expr);                                     \
    if (!result.status().ok()) {                              \
      return ::vineyard::Status::ArrowError(result.status()); \
    }                                                         \
    lhs = std::move(result).ValueOrDie();                     \
  } while (0)

template <typename T>
struct ConvertToArrowType {};

#define CONVERT_TO_ARROW_TYPE(type, array_type, builder_type, type_value)      \
  template <>                                                                  \
  struct ConvertToArrowType<type> {                                            \
    using ArrayType = array_type;                                              \
    using BuilderType = builder_type;                                          \
    static std::shared_ptr<arrow::DataType> TypeValue() { return type_value; } \
  };

CONVERT_TO_ARROW_TYPE(bool, arrow::BooleanArray, arrow::BooleanBuilder,
                      arrow::boolean())
CONVERT_TO_ARROW_TYPE(int8_t, arrow::Int8Array, arrow::Int8Builder,
                      arrow::int8())
CONVERT_TO_ARROW_TYPE(uint8_t, arrow::UInt8Array, arrow::UInt8Builder,
                      arrow::uint8())
CONVERT_TO_ARROW_TYPE(int16_t, arrow::Int16Array, arrow::Int16Builder,
                      arrow::int16())
CONVERT_TO_ARROW_TYPE(uint16_t, arrow::UInt16Array, arrow::UInt16Builder,
                      arrow::uint16())
CONVERT_TO_ARROW_TYPE(int32_t, arrow::Int32Array, arrow::Int32Builder,
                      arrow::int32())
CONVERT_TO_ARROW_TYPE(uint32_t, arrow::UInt32Array, arrow::UInt32Builder,
                      arrow::uint32())
CONVERT_TO_ARROW_TYPE(int64_t, arrow::Int64Array, arrow::Int64Builder,
                      arrow::int64())
CONVERT_TO_ARROW_TYPE(uint64_t, arrow::UInt64Array, arrow::UInt64Builder,
                      arrow::uint64())
CONVERT_TO_ARROW_TYPE(float, arrow::FloatArray, arrow::FloatBuilder,
                      arrow::float32())
CONVERT_TO_ARROW_TYPE(double, arrow::DoubleArray, arrow::DoubleBuilder,
                      arrow::float64())
CONVERT_TO_ARROW_TYPE(RefString, arrow::LargeStringArray,
                      arrow::LargeStringBuilder, arrow::large_utf8())
CONVERT_TO_ARROW_TYPE(std::string, arrow::LargeStringArray,
                      arrow::LargeStringBuilder, arrow::large_utf8())
CONVERT_TO_ARROW_TYPE(arrow::TimestampType, arrow::TimestampArray,
                      arrow::TimestampBuilder,
                      arrow::timestamp(arrow::TimeUnit::MILLI))
CONVERT_TO_ARROW_TYPE(arrow::Date32Type, arrow::Date32Array,
                      arrow::Date32Builder, arrow::date32())
CONVERT_TO_ARROW_TYPE(arrow::Date64Type, arrow::Date64Array,
                      arrow::Date64Builder, arrow::date64())

std::shared_ptr<arrow::DataType> FromAnyType(AnyType type);

/**
 * @brief PodArrayBuilder is designed for constructing Arrow arrays of POD data
 * type
 *
 * @tparam T
 */
template <typename T>
class PodArrayBuilder : public arrow::FixedSizeBinaryBuilder {
 public:
  explicit PodArrayBuilder(
      arrow::MemoryPool* pool = arrow::default_memory_pool())
      : arrow::FixedSizeBinaryBuilder(arrow::fixed_size_binary(sizeof(T)),
                                      pool) {}

  T* MutablePointer(int64_t i) {
    return reinterpret_cast<T*>(
        arrow::FixedSizeBinaryBuilder::GetMutableValue(i));
  }

  // the bahavior of `arrow::FixedSizeBinaryBuilder` has been changed in
  // https://github.com/apache/arrow/commit/e990d177, and hopeful to be
  // fixed in arrow 0.6.0.

  arrow::Status Resize(int64_t capacity) override {
#if defined(ARROW_VERSION) && ARROW_VERSION < 5000000
    return arrow::FixedSizeBinaryBuilder::Resize(capacity);
#else
    auto status = arrow::FixedSizeBinaryBuilder::Resize(capacity);
    if (!status.ok()) {
      return status;
    }
    return arrow::FixedSizeBinaryBuilder::AppendEmptyValues(capacity);
#endif
  }

  arrow::Status Advance(int64_t elements) {
#if defined(ARROW_VERSION) && ARROW_VERSION < 5000000
    return arrow::FixedSizeBinaryBuilder::Advance(elements);
#else
    return arrow::Status::OK();
#endif
  }
};

/**
 * Similar to arrow's `GetRecordBatchSize`, but considering the schema.
 *
 * Used for pre-allocate buffer for NewStreamWriter's `WriteRecordBatch`.
 */
Status GetRecordBatchStreamSize(const arrow::RecordBatch& batch, size_t* size);

Status SerializeRecordBatchesToAllocatedBuffer(
    const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
    std::shared_ptr<arrow::Buffer>* buffer);

Status SerializeRecordBatches(
    const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
    std::shared_ptr<arrow::Buffer>* buffer);

Status DeserializeRecordBatches(
    const std::shared_ptr<arrow::Buffer>& buffer,
    std::vector<std::shared_ptr<arrow::RecordBatch>>* batches);

Status RecordBatchesToTable(
    const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
    std::shared_ptr<arrow::Table>* table);

Status CombineRecordBatches(
    const std::vector<std::shared_ptr<arrow::RecordBatch>>& batches,
    std::shared_ptr<arrow::RecordBatch>* batch);

Status TableToRecordBatches(
    std::shared_ptr<arrow::Table> table,
    std::vector<std::shared_ptr<arrow::RecordBatch>>* batches);

Status SerializeTableToAllocatedBuffer(std::shared_ptr<arrow::Table> table,
                                       std::shared_ptr<arrow::Buffer>* buffer);

Status SerializeTable(std::shared_ptr<arrow::Table> table,
                      std::shared_ptr<arrow::Buffer>* buffer);

Status DeserializeTable(std::shared_ptr<arrow::Buffer> buffer,
                        std::shared_ptr<arrow::Table>* table);

struct EmptyTableBuilder {
  static Status Build(const std::shared_ptr<arrow::Schema>& schema,
                      std::shared_ptr<arrow::Table>& table);
};

/**
 * @brief Concatenate multiple arrow tables into one.
 */
std::shared_ptr<arrow::Table> ConcatenateTables(
    std::vector<std::shared_ptr<arrow::Table>>& tables);

/**
 * @brief Convert type name in string to arrow type.
 *
 */
std::shared_ptr<arrow::DataType> type_name_to_arrow_type(
    const std::string& name);

}  // namespace vineyard

#endif  // MODULES_BASIC_DS_ARROW_UTILS_H_
