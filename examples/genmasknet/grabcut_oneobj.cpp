// This program runs grabcut directly on leveldb/lmdb
#define EXT_FRAC 0.2

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
#include <sstream>

#include "caffe/proto/caffe.pb.h"
#include "caffe/util/io.hpp"
#include "caffe/util/rng.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <time.h>
#include <sys/time.h>
#include <stdlib.h> 

using namespace caffe;  // NOLINT(build/namespaces)
using std::pair;
using std::string;

void print_time_elapsed_since_mark(timespec& tmark0, int commit_period){
  timespec tmp_time;
  clock_gettime(CLOCK_REALTIME, &tmp_time);
  double period_time = difftime(tmp_time.tv_sec, tmark0.tv_sec) + 1.0e-9*(tmp_time.tv_nsec - tmark0.tv_nsec);
  printf("%d gc masks took %f seconds, average %f per image\n", commit_period, period_time, period_time/commit_period);
  clock_gettime(CLOCK_REALTIME, &tmark0);
}

void print_time_elapsed_since(timespec& tmark0){
  timespec tmp_time;
  clock_gettime(CLOCK_REALTIME, &tmp_time);
  double period_time = difftime(tmp_time.tv_sec, tmark0.tv_sec) + 1.0e-9*(tmp_time.tv_nsec - tmark0.tv_nsec);
  printf("--- %f seconds\n", period_time);
  clock_gettime(CLOCK_REALTIME, &tmark0);
}

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);

  if(argc < 3){
    printf("Incorrect number of input arguments\n");
    printf("./grabcut_oneobj <img_dbname> <gc_mask_dbname> <begin_idx> <end_idx>\n");
    return 1;
  }

  string img_dbname(argv[1]);
  string gc_mask_dbname(argv[2]);

  int begin_idx = -1;
  int end_idx = -1;
  if(3 < argc && argc == 5){
    begin_idx = atoi(argv[3]);
    end_idx = atoi(argv[4]);
    if(begin_idx >= end_idx){
      printf("begin_idx >= end_idx. Please correct this.\n");
      return 1;
    }
  }

  printf("begin_idx: %d, end_idx: %d\n", begin_idx, end_idx);
  std::ostringstream oss;
  oss << "/" << begin_idx << "_" << end_idx;
  gc_mask_dbname += oss.str();
  printf("%s\n", gc_mask_dbname.c_str());


  // Open image db
  // lmdb
  MDB_env *mdb_env;
  MDB_dbi mdb_dbi;
  MDB_val mdb_key, mdb_data;
  MDB_txn *mdb_txn;
  MDB_cursor *mdb_cursor;

  LOG(INFO) << "Opening lmdb " << img_dbname;
  CHECK_EQ(mdb_env_create(&mdb_env), MDB_SUCCESS)<<"mdb_env_create failed";
  CHECK_EQ(mdb_env_open(mdb_env, img_dbname.c_str(), 0, 0664), MDB_SUCCESS)<<"mdb_env_open failed";
  CHECK_EQ(mdb_txn_begin(mdb_env, NULL, MDB_RDONLY, &mdb_txn), MDB_SUCCESS)
      << "mdb_txn_begin failed";
  CHECK_EQ(mdb_dbi_open(mdb_txn, NULL, 0, &mdb_dbi), MDB_SUCCESS)
      << "mdb_dbi_open failed";
  CHECK_EQ(mdb_cursor_open(mdb_txn, mdb_dbi, &mdb_cursor), MDB_SUCCESS)
      << "mdb_cursor_open failed";
  
  // Open grabcut mask db to write the produced masks
  // lmdb
  MDB_env *gc_env;
  MDB_dbi gc_dbi;
  MDB_val gc_key, gc_data;
  MDB_txn *gc_txn;
  
  LOG(INFO) << "Opening lmdb " << gc_mask_dbname;
  CHECK_EQ(mkdir(gc_mask_dbname.c_str(), 0744), 0)<<"mkdir "<<gc_mask_dbname.c_str()<<" failed";
  CHECK_EQ(mdb_env_create(&gc_env), MDB_SUCCESS)<<"mdb_env_create failed";
  CHECK_EQ(mdb_env_set_mapsize(gc_env, 1099511627776), MDB_SUCCESS) << "mdb_env_set_mapsize failed";
  CHECK_EQ(mdb_env_open(gc_env, gc_mask_dbname.c_str(), 0, 0664), MDB_SUCCESS)<<"mdb_env_open failed";
  CHECK_EQ(mdb_txn_begin(gc_env, NULL, 0, &gc_txn), MDB_SUCCESS)<<"mdb_txn_begin failed";
  CHECK_EQ(mdb_dbi_open(gc_txn, NULL, 0, &gc_dbi), MDB_SUCCESS)<<"mdb_dbi_open failed";

  timespec tmark0;
  clock_gettime(CLOCK_REALTIME, &tmark0);
  int commit_period = 100;

  Datum datum;
  int i=0;
  while (mdb_cursor_get(mdb_cursor, &mdb_key, &mdb_data, MDB_NEXT) == 0){
    if(begin_idx != -1 && end_idx != -1){
      if(i < begin_idx){
        if(i%1000 == 999) printf("skipped past %d entries\n", i);
        i++;
        continue;
      } 
      else if(end_idx <= i) break;
    }

//timespec tmark1; clock_gettime(CLOCK_REALTIME, &tmark1);
    string value;
    value.assign((char*) mdb_data.mv_data, mdb_data.mv_size);
    datum.ParseFromString(value);
    cv::Mat cv_img_c0(datum.height(), datum.width(), CV_8UC1, 
      & datum.mutable_data()->at(0));
    cv::Mat cv_img_c1(datum.height(), datum.width(), CV_8UC1, 
      & datum.mutable_data()->at(datum.height()*datum.width()));
    cv::Mat cv_img_c2(datum.height(), datum.width(), CV_8UC1, 
      & datum.mutable_data()->at(datum.height()*datum.width()*2));
    std::vector<cv::Mat> channels_to_merge;
    channels_to_merge.push_back(cv_img_c0);
    channels_to_merge.push_back(cv_img_c1);
    channels_to_merge.push_back(cv_img_c2);
    cv::Mat cv_img;
    cv::merge(channels_to_merge, cv_img);
    //printf("Image h: %d, w: %d, c: %d l: %d\n", datum.height(), datum.width(), datum.channels(), datum.label());

    //initialize fg rect based on knowledge that cropped image is extended by EXT_FRAC from bbox
    cv::Rect init_rect(datum.height()*EXT_FRAC/(1+2*EXT_FRAC), datum.width()*EXT_FRAC/(1+2*EXT_FRAC),
      datum.height()/(1+2*EXT_FRAC), datum.width()/(1+2*EXT_FRAC));
//print_time_elapsed_since(tmark1);

//timespec tmark2; clock_gettime(CLOCK_REALTIME, &tmark2);
    cv::Mat mask = cv::Mat::zeros(datum.height(), datum.width(), CV_8UC1);
    cv::Mat bgdModel, fgdModel;
    cv::grabCut(cv_img, mask, init_rect, bgdModel, fgdModel, 3, cv::GC_INIT_WITH_RECT);
//print_time_elapsed_since(tmark2);

    cv::Mat binMask(mask.size(), CV_8UC1);
    binMask = mask & 1;

    Datum w_datum;
    w_datum.set_label(datum.label());
    w_datum.set_channels(1);
    w_datum.set_height(binMask.size().height);
    w_datum.set_width(binMask.size().width);
    w_datum.set_data((const char*) binMask.data, binMask.size().height*binMask.size().width);

    //write operation of mask
    gc_key.mv_size = mdb_key.mv_size;
    gc_key.mv_data = mdb_key.mv_data;

    string w_value;
    w_datum.SerializeToString(&w_value);
    gc_data.mv_size = w_value.size();
    gc_data.mv_data = const_cast<char*>(w_value.data());
    CHECK_EQ(mdb_put(gc_txn, gc_dbi, &gc_key, &gc_data, 0), MDB_SUCCESS) << "mdb_put failed";

    if(i%commit_period == commit_period-1){
      printf("===Finished %d examples===\n", i);
      printf("key: %.*s, data: %d\n", 
        (int) mdb_key.mv_size, (char*) mdb_key.mv_data, (int) mdb_data.mv_size);
      printf("gc_key: %.*s, gc_data: %d\n", 
        (int) gc_key.mv_size, (char*) gc_key.mv_data, (int) gc_data.mv_size);

      print_time_elapsed_since_mark(tmark0, commit_period);

      CHECK_EQ(mdb_txn_commit(gc_txn), MDB_SUCCESS)<<"mdb_txn_commit failed";
      CHECK_EQ(mdb_txn_begin(gc_env, NULL, 0, &gc_txn), MDB_SUCCESS)<<"mdb_txn_begin failed";
    }

    i++;
  }
  CHECK_EQ(mdb_txn_commit(gc_txn), MDB_SUCCESS)<<"mdb_txn_commit failed";
  mdb_close(gc_env, gc_dbi);
  mdb_env_close(gc_env);

  mdb_cursor_close(mdb_cursor);
  mdb_txn_abort(mdb_txn);
  mdb_dbi_close(mdb_env, mdb_dbi);
  mdb_env_close(mdb_env);

  return 0;
}
