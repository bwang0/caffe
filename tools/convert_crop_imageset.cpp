// This program converts a set of images to a leveldb by storing them as Datum
// proto buffers.
// Usage:
//   convert_crop_imageset [-g] ROOTFOLDER/ LISTFILE DB_NAME RANDOM_SHUFFLE[0 or 1]
//                     [resize_height] [resize_width]
// where ROOTFOLDER is the root folder that holds all the images, and LISTFILE
// should be a list of files as well as their labels, in the format as
//   subfolder1/file1.JPEG 7
//   ....
// if RANDOM_SHUFFLE is 1, a random shuffle will be carried out before we
// process the file lines.
// Optional flag -g indicates the images should be read as
// single-channel grayscale. If omitted, grayscale images will be
// converted to color.

#include <glog/logging.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <lmdb.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>

#include <iostream>

#include "caffe/proto/caffe.pb.h"
#include "caffe/util/io.hpp"
#include "caffe/util/rng.hpp"

using namespace caffe;  // NOLINT(build/namespaces)
using std::pair;
using std::string;

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  if (argc < 4 || argc > 9) {
    printf("Convert a set of cropped images to the leveldb format used\n"
        "as input for Caffe.\n"
        "Usage:\n"
        "    convert_crop_imageset [-g] ROOTFOLDER/ LISTFILE DB_NAME"
        " RANDOM_SHUFFLE_DATA[0 or 1] DB_BACKEND[leveldb or lmdb]"
        " [resize_height] [resize_width]\n");
    return 1;
  }
  typedef int int64;
  // Test whether argv[1] == "-g"
  bool is_color= !(string("-g") == string(argv[1]));
  int  arg_offset = (is_color ? 0 : 1);
  std::ifstream infile(argv[arg_offset+2]);
  std::vector<std::pair<string, std::vector<int> > > lines;
  string filename;
  int label;
  int row_min, col_min, row_max, col_max; // crop box
  int ann_id;

  std::vector<int> metainfo;
  while (infile >> filename >> label >> row_min >> col_min >> row_max >> col_max >> ann_id) {
    metainfo.push_back(label);
    metainfo.push_back(row_min);
    metainfo.push_back(col_min);
    metainfo.push_back(row_max);
    metainfo.push_back(col_max);
    metainfo.push_back(ann_id);
    lines.push_back(std::make_pair(filename, metainfo));
    metainfo.clear();
  }
  if (argc >= (arg_offset+5) && argv[arg_offset+4][0] == '1') {
    // randomly shuffle data
    LOG(INFO) << "Shuffling data";
    shuffle(lines.begin(), lines.end());
  }
  LOG(INFO) << "A total of " << lines.size() << " images.";

  string db_backend = "leveldb";
  if (argc >= (arg_offset+6)) {
    db_backend = string(argv[arg_offset+5]);
    if (!(db_backend == "leveldb") && !(db_backend == "lmdb")) {
      LOG(FATAL) << "Unknown db backend " << db_backend;
    }
  }

  int resize_height = 0;
  int resize_width = 0;
  if (argc >= (arg_offset+7)) {
    resize_height = atoi(argv[arg_offset+6]);
  }
  if (argc >= (arg_offset+8)) {
    resize_width = atoi(argv[arg_offset+7]);
  }

  // Open new db
  // lmdb
  MDB_env *mdb_env;
  MDB_dbi mdb_dbi;
  MDB_val mdb_key, mdb_data;
  MDB_txn *mdb_txn;
  // leveldb
  leveldb::DB* db;
  leveldb::Options options;
  options.error_if_exists = true;
  options.create_if_missing = true;
  options.write_buffer_size = 268435456;
  leveldb::WriteBatch* batch = NULL;

  // Open db
  if (db_backend == "leveldb") {  // leveldb
    LOG(INFO) << "Opening leveldb " << argv[arg_offset+3];
    leveldb::Status status = leveldb::DB::Open(
        options, argv[arg_offset+3], &db);
    CHECK(status.ok()) << "Failed to open leveldb " << argv[arg_offset+3];
    batch = new leveldb::WriteBatch();
  } else if (db_backend == "lmdb") {  // lmdb
    LOG(INFO) << "Opening lmdb " << argv[arg_offset+3];
    CHECK_EQ(mkdir(argv[arg_offset+3], 0744), 0)
        << "mkdir " << argv[arg_offset+3] << "failed";
    CHECK_EQ(mdb_env_create(&mdb_env), MDB_SUCCESS) << "mdb_env_create failed";
    CHECK_EQ(mdb_env_set_mapsize(mdb_env, 1099511627776), MDB_SUCCESS)  // 1TB
        << "mdb_env_set_mapsize failed";
    CHECK_EQ(mdb_env_open(mdb_env, argv[3], 0, 0664), MDB_SUCCESS)
        << "mdb_env_open failed";
    CHECK_EQ(mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn), MDB_SUCCESS)
        << "mdb_txn_begin failed";
    CHECK_EQ(mdb_open(mdb_txn, NULL, 0, &mdb_dbi), MDB_SUCCESS)
        << "mdb_open failed";
  } else {
    LOG(FATAL) << "Unknown db backend " << db_backend;
  }

  // Storing to db
  string root_folder(argv[arg_offset+1]);
  Datum datum;
  int count = 0;
  const int kMaxKeyLength = 256;
  char key_cstr[kMaxKeyLength];
  int data_size;
  bool data_size_initialized = false;

  for (int line_id = 0; line_id < lines.size(); ++line_id) {
    if (!ReadCroppedImageToDatum(root_folder + lines[line_id].first,
        lines[line_id].second[0], resize_height, resize_width, is_color, &datum,
        lines[line_id].second[1], //row_min of bbox
        lines[line_id].second[2], //col_min of bbox
        lines[line_id].second[3], //row_max of bbox
        lines[line_id].second[4], //col_max of bbox
        true) //beyond border flag
      ) {
      continue;
    }
    if (!data_size_initialized) {
      data_size = datum.channels() * datum.height() * datum.width();
      data_size_initialized = true;
    } else {
      const string& data = datum.data();
      CHECK_EQ(data.size(), data_size) << "Incorrect data field size "
          << data.size();
    }
    // sequential
    snprintf(key_cstr, kMaxKeyLength, "%08d_%s(%05d,%05d,%05d,%05d)%08d", line_id,
        lines[line_id].first.c_str(),
        lines[line_id].second[1], lines[line_id].second[2],
        lines[line_id].second[3], lines[line_id].second[4],
        lines[line_id].second[5]);

    string value;
    datum.SerializeToString(&value);
    string keystr(key_cstr);

    //
    if(line_id%1000 == 0){
      std::cout<<keystr<<std::endl;
    }

    // Put in db
    if (db_backend == "leveldb") {  // leveldb
      batch->Put(keystr, value);
    } else if (db_backend == "lmdb") {  // lmdb
      mdb_data.mv_size = value.size();
      mdb_data.mv_data = reinterpret_cast<void*>(&value[0]);
      mdb_key.mv_size = keystr.size();
      mdb_key.mv_data = reinterpret_cast<void*>(&keystr[0]);
      CHECK_EQ(mdb_put(mdb_txn, mdb_dbi, &mdb_key, &mdb_data, 0), MDB_SUCCESS)
          << "mdb_put failed";
    } else {
      LOG(FATAL) << "Unknown db backend " << db_backend;
    }

    if (++count % 1000 == 0) {
      // Commit txn
      if (db_backend == "leveldb") {  // leveldb
        db->Write(leveldb::WriteOptions(), batch);
        delete batch;
        batch = new leveldb::WriteBatch();
      } else if (db_backend == "lmdb") {  // lmdb
        CHECK_EQ(mdb_txn_commit(mdb_txn), MDB_SUCCESS)
            << "mdb_txn_commit failed";
        CHECK_EQ(mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn), MDB_SUCCESS)
            << "mdb_txn_begin failed";
      } else {
        LOG(FATAL) << "Unknown db backend " << db_backend;
      }
      LOG(ERROR) << "Processed " << count << " files.";
    }
  }
  // write the last batch
  if (count % 1000 != 0) {
    if (db_backend == "leveldb") {  // leveldb
      db->Write(leveldb::WriteOptions(), batch);
      delete batch;
      delete db;
    } else if (db_backend == "lmdb") {  // lmdb
      CHECK_EQ(mdb_txn_commit(mdb_txn), MDB_SUCCESS) << "mdb_txn_commit failed";
      mdb_close(mdb_env, mdb_dbi);
      mdb_env_close(mdb_env);
    } else {
      LOG(FATAL) << "Unknown db backend " << db_backend;
    }
    LOG(ERROR) << "Processed " << count << " files.";
  }
  return 0;
}
