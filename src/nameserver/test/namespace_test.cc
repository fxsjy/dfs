// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#define private public
#include "nameserver/namespace.h"

#include "common/util.h"
#include <fcntl.h>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

DECLARE_string(namedb_path);

namespace baidu {
namespace bfs {

class NameSpaceTest : public ::testing::Test {
public:
    NameSpaceTest() {}
protected:
};

TEST_F(NameSpaceTest, EncodingStoreKey) {
    std::string key_start;
    NameSpace::EncodingStoreKey(5, "", &key_start);
    ASSERT_EQ(key_start.size(), 8);
    for (int i = 0; i < 7; i++) {
        ASSERT_EQ(key_start[i], 0);
    }
    ASSERT_EQ(key_start[7], '\5');

    NameSpace::EncodingStoreKey(3<<16, "/home", &key_start);
    ASSERT_EQ(key_start.size(), 13);
    ASSERT_EQ(key_start[5], 3);
    ASSERT_EQ(key_start.substr(8), std::string("/home"));
}

TEST_F(NameSpaceTest, SplitPath) {
    std::vector<std::string> element;
    ASSERT_TRUE(common::util::SplitPath("/home", &element));
    ASSERT_EQ(1U, element.size());
    ASSERT_EQ(element[0], std::string("home"));
    ASSERT_TRUE(common::util::SplitPath("/dir1/subdir1/file3", &element));
    ASSERT_EQ(3U, element.size());
    ASSERT_EQ(element[0], std::string("dir1"));
    ASSERT_EQ(element[1], std::string("subdir1"));
    ASSERT_EQ(element[2], std::string("file3"));
    ASSERT_TRUE(common::util::SplitPath("/", &element));
    ASSERT_EQ(0U, element.size());
    ASSERT_FALSE(common::util::SplitPath("", &element));

}

bool CreateTree(NameSpace* ns) {
    int ret = ns->CreateFile("/file1", 0, 0);
    ret |= ns->CreateFile("/file2", 0, 0);
    ret |= ns->CreateFile("/dir1/subdir1/file3", 0, 0);
    ret |= ns->CreateFile("/dir1/subdir1/file4", 0, 0);
    ret |= ns->CreateFile("/dir1/subdir2/file5", 0, 0);
    return ret == 0;
}

TEST_F(NameSpaceTest, NameSpace) {
    std::string version;
    {
        FLAGS_namedb_path = "./db";
        system("rm -rf ./db");
        NameSpace init_ns;
        leveldb::Iterator* it = init_ns._db->NewIterator(leveldb::ReadOptions());
        it->SeekToFirst();
        ASSERT_TRUE(it->Valid());
        ASSERT_EQ(it->key().ToString(), std::string(8, 0) + "version");
        version = it->value().ToString();
        delete it;
    }

    // Load  namespace with old version
    NameSpace ns;
    CreateTree(&ns);
    leveldb::Iterator* it = ns._db->NewIterator(leveldb::ReadOptions());
    it->SeekToFirst();
    ASSERT_EQ(version, it->value().ToString());

    // Iterate namespace
    it->Seek(std::string(7, 0) + '\1');
    ASSERT_TRUE(it->Valid());
    std::string entry1(it->key().data(), it->key().size());
    ASSERT_EQ(entry1.substr(8), std::string("dir1"));

    // next entry
    it->Next();
    // key
    std::string entry2(it->key().data(), it->key().size());
    ASSERT_EQ(entry2.substr(8), std::string("file1"));
    ASSERT_EQ(entry2[0], 0);
    ASSERT_EQ(*reinterpret_cast<int*>(&entry2[1]), 0);
    ASSERT_EQ(entry2[7], 1);
    // value
    FileInfo info;
    ASSERT_TRUE(info.ParseFromArray(it->value().data(), it->value().size()));
    ASSERT_EQ(2, info.entry_id());

    delete it;
}

TEST_F(NameSpaceTest, CreateFile) {
    FLAGS_namedb_path = "./db";
    system("rm -rf ./db");
    NameSpace ns;
    ASSERT_EQ(0, ns.CreateFile("/file1", 0, 0));
    ASSERT_NE(0, ns.CreateFile("/file1", 0, 0));
    ASSERT_EQ(0, ns.CreateFile("/dir1/subdir1/file1", 0, 0));
    ASSERT_EQ(0, ns.CreateFile("/dir1/subdir1/file1", O_TRUNC, 0));
    ASSERT_EQ(0, ns.CreateFile("/dir1/subdir2/file1", 0, 0));
}

TEST_F(NameSpaceTest, List) {
    FLAGS_namedb_path = "./db";
    system("rm -rf ./db");
    NameSpace ns;
    ASSERT_TRUE(CreateTree(&ns));
    google::protobuf::RepeatedPtrField<FileInfo> outputs;
    ASSERT_EQ(0, ns.ListDirectory("/dir1", &outputs));
    ASSERT_EQ(2U, outputs.size());
    ASSERT_EQ(std::string("subdir1"), outputs.Get(0).name());
    ASSERT_EQ(std::string("subdir2"), outputs.Get(1).name());
}

TEST_F(NameSpaceTest, Rename) {
    NameSpace ns;
    bool need_unlink;
    FileInfo remove_file;
    /// dir -> none
    ASSERT_EQ(0, ns.Rename("/dir1/subdir1", "/dir1/subdir3", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    /// dir -> existing dir
    ASSERT_NE(0, ns.Rename("/dir1/subdir2", "/dir1/subdir3", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    /// file -> not exist parent
    ASSERT_NE(0, ns.Rename("/file1", "/dir1/subdir4/file1", &need_unlink, &remove_file));
    /// file -> existing dir
    ASSERT_NE(0, ns.Rename("/file1", "/dir1/subdir3", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    /// file -> none
    ASSERT_EQ(0, ns.Rename("/file1", "/dir1/subdir3/file1", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    /// noe -> none
    ASSERT_NE(0, ns.Rename("/file1", "/dir1/subdir3/file2", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    /// file -> existing file
    ASSERT_EQ(0, ns.Rename("/dir1/subdir2/file5", "/dir1/subdir3/file1", &need_unlink, &remove_file));
    ASSERT_TRUE(need_unlink);
    ASSERT_EQ(remove_file.entry_id(), 2);

    /// Root dir to root dir
    ASSERT_NE(0, ns.Rename("/", "/dir2", &need_unlink, &remove_file));
    ASSERT_EQ(0, ns.Rename("/dir1", "/dir2", &need_unlink, &remove_file));

    /// Deep rename
    ASSERT_EQ(0, ns.CreateFile("/tera/meta/0/00000001.dbtmp", 0, 0));
    ASSERT_EQ(0, ns.Rename("/tera/meta/0/00000001.dbtmp", "/tera/meta/0/CURRENT", &need_unlink, &remove_file));
    ASSERT_FALSE(need_unlink);
    ASSERT_TRUE(ns.LookUp("/tera/meta/0/CURRENT", &remove_file));
}

TEST_F(NameSpaceTest, RemoveFile) {
    FLAGS_namedb_path = "./db";
    system("rm -rf ./db");
    NameSpace ns;
    ASSERT_TRUE(CreateTree(&ns));
    FileInfo file_removed;
    ASSERT_EQ(0, ns.RemoveFile("/file2",&file_removed));
    ASSERT_EQ(3, file_removed.entry_id());
    ASSERT_NE(0, ns.RemoveFile("/",&file_removed));
    ASSERT_EQ(0, ns.RemoveFile("/file1",&file_removed));
    ASSERT_EQ(2, file_removed.entry_id());
    ASSERT_NE(0, ns.RemoveFile("/file2",&file_removed));
    ASSERT_NE(0, ns.RemoveFile("/file3",&file_removed));
}

TEST_F(NameSpaceTest, DeleteDirectory) {
    FLAGS_namedb_path = "./db";
    system("rm -rf ./db");
    NameSpace ns;
    ASSERT_TRUE(CreateTree(&ns));
    std::vector<FileInfo> files_removed;

    // Delete not empty
    ASSERT_NE(0, ns.DeleteDirectory("/dir1", false, &files_removed));
    ASSERT_NE(0, ns.DeleteDirectory("/dir1/subdir2", false, &files_removed));
    // Delete root dir
    ASSERT_NE(0, ns.DeleteDirectory("/", false, &files_removed));
    // Delete subdir
    ASSERT_EQ(0, ns.DeleteDirectory("/dir1/subdir2", true, &files_removed));
    ASSERT_EQ(files_removed.size(), 1U);
    ASSERT_EQ(files_removed[0].entry_id(), 9);
    // List after delete
    google::protobuf::RepeatedPtrField<FileInfo> outputs;
    ASSERT_EQ(0, ns.ListDirectory("/dir1", &outputs));
    ASSERT_EQ(1U, outputs.size());
    ASSERT_EQ(std::string("subdir1"), outputs.Get(0).name());

    // Delete another subdir
    printf("Delete another subdir\n");
    ASSERT_NE(0, ns.DeleteDirectory("/dir1/subdir1", false, &files_removed));
    ASSERT_EQ(0, ns.ListDirectory("/dir1/subdir1", &outputs));
    ASSERT_EQ(2U, outputs.size());

    ASSERT_EQ(0, ns.DeleteDirectory("/dir1", true, &files_removed));
    ASSERT_EQ(files_removed.size(), 2U);
    ASSERT_EQ(files_removed[0].entry_id(), 6);
    ASSERT_EQ(files_removed[1].entry_id(), 7);
    ASSERT_NE(0, ns.ListDirectory("/dir1/subdir1", &outputs));
    ASSERT_NE(0, ns.ListDirectory("/dir1", &outputs));

    // Use rmr to delete a file
    ASSERT_EQ(886, ns.DeleteDirectory("/file1", true, &files_removed));
}
}
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
