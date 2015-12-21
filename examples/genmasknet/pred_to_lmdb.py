#!/usr/bin/env python

import caffe,lmdb,os
import numpy as np
import sys

## read image setting
#data_root = '/mnt/neocortex4/scratch/bhwang/coco4'
#lmdb_file = 'coco_val_1obj_ext20_256x256_img'
#mean_file = 'coco_val_1obj_ext20_256x256_img_mean.npy'

## caffe_pred setting
#deploy_file = 'mirror_alexnet_whole_deploy.prototxt'
#model_file = 'mirror_alex_whole_caffemodel_iter_7600.caffemodel'

#dest_lmdb_file = 'new_coco_val_predict_mask_256_new'

if len(sys.argv) < 6:
  print "./pred_to_lmdb.py <deploy_file> <model_file> <img_lmdb> <mean_img.npy> <out_gt_lmdb>"
  sys.exit(1)

deploy_file = sys.argv[1]
model_file = sys.argv[2]
img_lmdb = sys.argv[3]
mean_file = sys.argv[4]
out_gt_lmdb = sys.argv[5]

db_out = lmdb.Environment(out_gt_lmdb,map_size=int(1e12))
txn_out = db_out.begin(write=True,buffers=True)

caffe.set_mode_gpu()
caffe.set_device(2)
net = caffe.Net(deploy_file, model_file, caffe.TEST)

########################################
### begin
env_img = lmdb.open(img_lmdb)
txn_img = env_img.begin()
cursor_img = txn_img.cursor()

for (idx,(key,value)) in enumerate(cursor_img):
  if idx%100==0 and idx>1:
    txn_out.commit()
    print idx
  #if idx>1:
  #    break
  datum = caffe.io.caffe_pb2.Datum()
  datum.ParseFromString(value)
  img_np = caffe.io.datum_to_array(datum)
  
  ### caffe_pred
  net.blobs['data'].reshape(1,3,256,256)
  net.blobs['data'].data[...] = img_np - np.load(mean_file)
  out = net.forward(blobs=['segmask'])
  arr = out['segmask']
  arr = arr[0,:,:,:]
  arr = np.round(arr)
  arr = arr.astype(float)

  ### convert to datum
  datum_out = caffe.io.array_to_datum(arr)
  txn_out.put(key,datum_out.SerializeToString())  
txn_out.commit()
db_out.close()




    

