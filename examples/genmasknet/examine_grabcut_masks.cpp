// Examine grabcut masks

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

#include <getopt.h>

#include <iostream>

#include "caffe/proto/caffe.pb.h"
#include "caffe/util/io.hpp"
#include "caffe/util/rng.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace caffe;  // NOLINT(build/namespaces)
using std::pair;
using std::string;

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);

  if(argc < 2){
    printf("Incorrect number of input arguments\n");
    return 1;
  }

  string gc_mask_dbname(argv[1]);
  
  // Open grabcut mask db to write the produced masks
  // lmdb
  MDB_env *gc_env;
  MDB_dbi gc_dbi;
  MDB_val gc_key, gc_data;
  MDB_txn *gc_txn;
  MDB_cursor * gc_cursor;
  
  LOG(INFO) << "Opening lmdb " << gc_mask_dbname;
  CHECK_EQ(mdb_env_create(&gc_env), MDB_SUCCESS)<<"mdb_env_create failed";
  CHECK_EQ(mdb_env_set_mapsize(gc_env, 1099511627776), MDB_SUCCESS) << "mdb_env_set_mapsize failed";
  CHECK_EQ(mdb_env_open(gc_env, gc_mask_dbname.c_str(), 0, 0664), MDB_SUCCESS)<<"mdb_env_open failed";
  CHECK_EQ(mdb_txn_begin(gc_env, NULL, MDB_RDONLY, &gc_txn), MDB_SUCCESS)<<"mdb_txn_begin failed";
  CHECK_EQ(mdb_dbi_open(gc_txn, NULL, 0, &gc_dbi), MDB_SUCCESS)<<"mdb_dbi_open failed";
  CHECK_EQ(mdb_cursor_open(gc_txn, gc_dbi, &gc_cursor), MDB_SUCCESS)<<"mdb_cursor_open failed";

  Datum datum;
  int i=0;
  while (mdb_cursor_get(gc_cursor, &gc_key, &gc_data, MDB_NEXT) == 0){
    
if(i%1000 == 0){
    printf("key: %p %.*s, data: %p %d\n", 
      gc_key.mv_data, (int) gc_key.mv_size, (char*)gc_key.mv_data, 
      gc_data.mv_data, (int) gc_data.mv_size);
}
    //cv::Mat binMask = cv::Mat(256, 256, CV_8UC1, gc_data.mv_data).clone();
    //binMask *= 255;

    //cv::namedWindow("Grabcut Mask", cv::WINDOW_AUTOSIZE);
    //cv::imshow("Grabcut Mask", binMask);
    
    //cv::waitKey();

    i++;
  }
  mdb_cursor_close(gc_cursor);
  mdb_txn_abort(gc_txn);
  mdb_close(gc_env, gc_dbi);
  mdb_env_close(gc_env);

  return 0;
}
