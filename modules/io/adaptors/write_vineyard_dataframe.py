#! /usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2020-2021 Alibaba Group Holding Limited.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import json
import sys

import pyarrow as pa
import vineyard


def print_vineyard_id(vineyard_id):
    ret = {"type": "return", "content": repr(vineyard_id)}
    print(json.dumps(ret))


def write_vineyard_dataframe(vineyard_socket, stream_id, proc_num, proc_index):
    client = vineyard.connect(vineyard_socket)
    streams = client.get(stream_id)
    if len(streams) != proc_num or streams[proc_index] is None:
        raise ValueError(
            f"Fetch stream error with proc_num={proc_num},proc_index={proc_index}"
        )
    instream = streams[proc_index]
    stream_reader = instream.open_reader(client)

    idx = 0
    while True:
        try:
            content = stream_reader.next()
        except:
            break
        buf_reader = pa.ipc.open_stream(pa.py_buffer(content))
        while True:
            try:
                batch = buf_reader.read_next_batch()
            except StopIteration:
                break
            df = batch.to_pandas()
            df_id = client.put(df, partition_index=[proc_index, 0], row_batch_index=idx)
            client.persist(df_id)
            idx += 1
            print_vineyard_id(df_id)


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print(
            "usage: ./write_vineyard_dataframe <ipc_socket> <stream_id> <proc_num> <proc_index>"
        )
        exit(1)
    ipc_socket = sys.argv[1]
    stream_id = sys.argv[2]
    proc_num = int(sys.argv[3])
    proc_index = int(sys.argv[4])
    write_vineyard_dataframe(ipc_socket, stream_id, proc_num, proc_index)
