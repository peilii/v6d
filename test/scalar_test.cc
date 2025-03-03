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

#include <memory>
#include <string>
#include <thread>

#include "arrow/status.h"
#include "arrow/util/io_util.h"
#include "arrow/util/logging.h"

#include "basic/ds/scalar.h"
#include "basic/ds/types.h"
#include "client/client.h"
#include "client/ds/object_meta.h"
#include "common/util/logging.h"

using namespace vineyard;  // NOLINT(build/namespaces)

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("usage ./scalar_test <ipc_socket>");
    return 1;
  }
  std::string ipc_socket = std::string(argv[1]);

  Client client;
  VINEYARD_CHECK_OK(client.Connect(ipc_socket));
  LOG(INFO) << "Connected to IPCServer: " << ipc_socket;

  {
    ScalarBuilder<int32_t> scalar_builder(client);
    scalar_builder.SetValue(1234);

    auto scalar =
        std::dynamic_pointer_cast<Scalar<int32_t>>(scalar_builder.Seal(client));
    VINEYARD_CHECK_OK(client.Persist(scalar->id()));

    CHECK_EQ(scalar->Type(), AnyType::Int32);
    CHECK_EQ(scalar->Value(), 1234);
  }

  {
    ScalarBuilder<double> scalar_builder(client);
    scalar_builder.SetValue(1234.5678);

    auto scalar =
        std::dynamic_pointer_cast<Scalar<double>>(scalar_builder.Seal(client));
    VINEYARD_CHECK_OK(client.Persist(scalar->id()));

    CHECK_EQ(scalar->Type(), AnyType::Double);
    CHECK_DOUBLE_EQ(scalar->Value(), 1234.5678);
  }

  {
    ScalarBuilder<std::string> scalar_builder(client);
    scalar_builder.SetValue("1234_5678");

    auto scalar = std::dynamic_pointer_cast<Scalar<std::string>>(
        scalar_builder.Seal(client));
    VINEYARD_CHECK_OK(client.Persist(scalar->id()));

    CHECK_EQ(scalar->Type(), AnyType::String);
    CHECK_EQ(scalar->Value(), "1234_5678");
  }

  LOG(INFO) << "Passed scalar tests...";

  client.Disconnect();

  return 0;
}
