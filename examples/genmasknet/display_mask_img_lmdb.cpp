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

  if(argc < 3){
    printf("Incorrect number of input arguments\n");
    printf("./display_mask_img_lmdb <mask_dbname> <img_dbname>\n");
    return 1;
  }

  string mask_dbname(argv[1]);
  string img_dbname(argv[2]);

  // Open mask db to extract the produced masks
  // lmdb
  MDB_env *mask_env;
  MDB_dbi mask_dbi;
  MDB_val mask_key, mask_data;
  MDB_txn *mask_txn;
  MDB_cursor * mask_cursor;
  
  LOG(INFO) << "Opening lmdb " << mask_dbname;
  CHECK_EQ(mdb_env_create(&mask_env), MDB_SUCCESS)<<"mdb_env_create failed";
  CHECK_EQ(mdb_env_set_mapsize(mask_env, 1099511627776), MDB_SUCCESS) << "mdb_env_set_mapsize failed";
  CHECK_EQ(mdb_env_open(mask_env, mask_dbname.c_str(), 0, 0664), MDB_SUCCESS)<<"mdb_env_open failed";
  CHECK_EQ(mdb_txn_begin(mask_env, NULL, MDB_RDONLY, &mask_txn), MDB_SUCCESS)<<"mdb_txn_begin failed";
  CHECK_EQ(mdb_dbi_open(mask_txn, NULL, 0, &mask_dbi), MDB_SUCCESS)<<"mdb_dbi_open failed";
  CHECK_EQ(mdb_cursor_open(mask_txn, mask_dbi, &mask_cursor), MDB_SUCCESS)<<"mdb_cursor_open failed";

  MDB_env *img_env;
  MDB_dbi img_dbi;
  MDB_val img_key, img_data;
  MDB_txn *img_txn;
  MDB_cursor * img_cursor;
  
  LOG(INFO) << "Opening lmdb " << img_dbname;
  CHECK_EQ(mdb_env_create(&img_env), MDB_SUCCESS)<<"mdb_env_create failed";
  CHECK_EQ(mdb_env_set_mapsize(img_env, 1099511627776), MDB_SUCCESS) << "mdb_env_set_mapsize failed";
  CHECK_EQ(mdb_env_open(img_env, img_dbname.c_str(), 0, 0664), MDB_SUCCESS)<<"mdb_env_open failed";
  CHECK_EQ(mdb_txn_begin(img_env, NULL, MDB_RDONLY, &img_txn), MDB_SUCCESS)<<"mdb_txn_begin failed";
  CHECK_EQ(mdb_dbi_open(img_txn, NULL, 0, &img_dbi), MDB_SUCCESS)<<"mdb_dbi_open failed";
  CHECK_EQ(mdb_cursor_open(img_txn, img_dbi, &img_cursor), MDB_SUCCESS)<<"mdb_cursor_open failed";

  Datum datum;
  string value;
  int h = datum.height();
  int w = datum.width();
  int i=0;
  while (mdb_cursor_get(mask_cursor, &mask_key, &mask_data, MDB_NEXT) == 0){
    printf("key: %p %.*s, data: %p %d\n", 
      mask_key.mv_data, (int) mask_key.mv_size, (char*)mask_key.mv_data, 
      mask_data.mv_data, (int) mask_data.mv_size);
    value.assign((char*) mask_data.mv_data, mask_data.mv_size);
    datum.ParseFromString(value);
    h = datum.height();
    w = datum.width();
    cv::Mat binMask = cv::Mat(h, w, CV_8UC1, & datum.mutable_data()->at(0)).clone();
    //binMask *= 255;
    cv::namedWindow("Mask", cv::WINDOW_AUTOSIZE);
    cv::imshow("Mask", binMask);

    CHECK_EQ(mdb_cursor_get(img_cursor, &img_key, &img_data, MDB_NEXT), 0) 
      << "mdb_cursor_get failed for img lmdb";
    printf("key: %p %.*s, data: %p %d\n", 
      img_key.mv_data, (int) img_key.mv_size, (char*)img_key.mv_data, 
      img_data.mv_data, (int) img_data.mv_size);
    value.assign((char*) img_data.mv_data, img_data.mv_size);
    datum.ParseFromString(value);
    h = datum.height();
    w = datum.width();
    cv::Mat img_c0 = cv::Mat(h, w, CV_8UC1, & datum.mutable_data()->at(0));
    cv::Mat img_c1 = cv::Mat(h, w, CV_8UC1, & datum.mutable_data()->at(h*w));
    cv::Mat img_c2 = cv::Mat(h, w, CV_8UC1, & datum.mutable_data()->at(h*w*2));
    std::vector<cv::Mat> channels_to_merge;
    channels_to_merge.push_back(img_c0);
    channels_to_merge.push_back(img_c1);
    channels_to_merge.push_back(img_c2);
    cv::Mat img;
    cv::merge(channels_to_merge, img);
    cv::namedWindow("Img", cv::WINDOW_AUTOSIZE);
    cv::imshow("Img", img);
    
    cv::Mat overlay;
    cv::cvtColor(binMask > 10, overlay, CV_GRAY2BGR);
    overlay = overlay & img;
    cv::namedWindow("Overlay0", cv::WINDOW_AUTOSIZE);
    cv::imshow("Overlay0", overlay);

    cv::waitKey();

    i++;
  }
  mdb_cursor_close(mask_cursor);
  mdb_txn_abort(mask_txn);
  mdb_close(mask_env, mask_dbi);
  mdb_env_close(mask_env);

  mdb_cursor_close(img_cursor);
  mdb_txn_abort(img_txn);
  mdb_close(img_env, img_dbi);
  mdb_env_close(img_env);

  return 0;
}
