// Copyright (c) 2014, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include <gflags/gflags.h>

// nameserver
DEFINE_string(namedb_path, "./db", "Namespace database");
DEFINE_int64(namedb_cache_size, 1024L, "Namespace datebase memery cache size");
DEFINE_string(nameserver, "127.0.0.1", "Nameserver host");
DEFINE_string(nameserver_port, "8828", "Nameserver port");
DEFINE_int32(keepalive_timeout, 30, "Chunkserver keepalive timeout");
DEFINE_int32(default_replica_num, 3, "Default replica num of data block");
DEFINE_int32(nameserver_log_level, 2, "Nameserver log level");
DEFINE_int32(nameserver_safemode_time, 120, "Nameserver leave safemode time in ms");

// chunkserver
DEFINE_string(block_store_path, "./data", "Data path");
DEFINE_string(chunkserver_port, "8825", "Chunkserver port");
DEFINE_int32(heartbeat_interval, 1, "Heartbeat interval");
DEFINE_int32(blockreport_interval, 10, "blockreport_interval");
DEFINE_int32(blockreport_size, 2000, "blockreport_size");
DEFINE_int32(chunkserver_log_level, 4, "Nameserver log level");
DEFINE_string(chunkserver_warninglog, "./wflog", "Warning log file");
DEFINE_int32(write_buf_size, 1024*1024, "Block write buffer size, bytes");
DEFINE_int32(chunkserver_max_pending_buffers, 10240, "Buffer num wait flush to disk");
DEFINE_int32(chunkserver_work_thread_num, 10, "Chunkserver work thread num");
DEFINE_int32(chunkserver_read_thread_num, 20, "Chunkserver work thread num");
DEFINE_int32(chunkserver_write_thread_num, 10, "Chunkserver work thread num");
DEFINE_int32(chunkserver_file_cache_size, 1000, "Chunkserver file cache size");

// SDK
DEFINE_int32(sdk_thread_num, 10, "Sdk thread num");
DEFINE_int32(sdk_file_reada_len, 1024*1024, "Read ahead buffer len");
DEFINE_string(sdk_write_mode, "chains", "Sdk write mode: chains/fan-out");

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
