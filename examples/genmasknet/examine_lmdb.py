#!/usr/bin/env python

from pycocotools.coco import COCO
from caffe.proto.caffe_pb2 import Datum
import caffe.io
import lmdb

import os
import tempfile
import random
import numpy as np
from scipy.ndimage import zoom
import skimage.io

import matplotlib.pyplot as plt
import pylab

import sys

def display_img(key, value, valgt):
  base_dir = '../images/val2014/'
  img_filename = key.partition('_')[2].partition('(')[0]
  img_filename = base_dir + img_filename
  orig_img = skimage.io.imread(img_filename)

  myd = Datum()

  myd.ParseFromString(value)
  img = caffe.io.datum_to_array(myd)
  img = np.swapaxes(img,0,1)
  img = np.swapaxes(img,1,2)
  img_c = np.copy(img)
  img_c[:,:,0] = img[:,:,2]
  img_c[:,:,2] = img[:,:,0]
  img = img_c

  if len(valgt) != 0:
    mydgt = Datum()
    mydgt.ParseFromString(valgt)
    imggt = caffe.io.datum_to_array(mydgt)
    imggt = np.swapaxes(imggt,0,1)
    imggt = np.swapaxes(imggt,1,2)
    print imggt.shape
    
    imggt3 = np.tile(imggt, (1,1,3))
    my_cut = imggt3 * img

  if len(valgt) == 0:
    fig = plt.figure()
    plt.subplot(1,2,1)
    plt.imshow(img)
    plt.subplot(1,2,2)
    plt.imshow(orig_img)
  else:
    fig = plt.figure()
    plt.subplot(2,2,1)
    plt.imshow(img)
    plt.subplot(2,2,2)
    plt.imshow(imggt[:,:,0])
    plt.subplot(2,2,3)
    plt.imshow(my_cut)
    plt.subplot(2,2,4)
    plt.imshow(orig_img)
  #print img

  plt.waitforbuttonpress()
  plt.close(fig)

if __name__ == "__main__":
  #img_db_name = "/mnt/neocortex4/scratch/bhwang/coco/311_val_1obj_ext20_256x256_img"
  #gt_mask_db_name = "/mnt/neocortex4/scratch/bhwang/coco/311_val_1obj_ext20_256x256_gt_mask"

  if len(sys.argv) == 0:
    print "./examine_lmdb.py <img_db_name> [gt_mask_db_name]"
  else:
    img_db_name = sys.argv[1]
    gt_mask_db_name = ''
    if len(sys.argv) == 3:
      gt_mask_db_name = sys.argv[2]

  env = lmdb.open(img_db_name, map_size=1048576*1024*1024) # 1TB
  txn = env.begin(write=False)
  cur = txn.cursor()

  if len(gt_mask_db_name) > 0:
    envgt = lmdb.open(gt_mask_db_name, map_size=1048576*1024*1024) # 1TB
    txngt = envgt.begin(write=False)
    curgt = txngt.cursor()
  
  #import pdb; pdb.set_trace()

  for key,value in cur:
    print key

    if len(gt_mask_db_name) > 0:
      valgt = txngt.get(key)
      display_img(key, value, valgt)
    else:
      display_img(key, value, '')

  sys.exit(0)

#img_key = "00000196_COCO_val2014_000000330369.jpg(00080,00108,00172,00165)00192518"
#gt_key = "00000197_COCO_val2014_000000330369.jpg(00080,00108,00172,00165)00192518"
#value = img_cur.get(img_key)
#valgt = gt_cur.get(gt_key)
#display_img(value, valgt)

############################################################

fig = plt.figure()
#pic1 = fig.add_subplot(2,2,1)
plt.subplot(2,2,1)
plt.imshow(img_np)
#pic2 = fig.add_subplot(2,2,3)
plt.subplot(2,2,3)
plt.xlabel('Prediction: '+getname(labelpre))
plt.imshow(a)
#pic3 = fig.add_subplot(2,2,4)
plt.subplot(2,2,4)
plt.xlabel('Groundtruth: ' + getname(int(datum.label)))
plt.imshow(b)
#pylab.show(block=False)

plt.waitforbuttonpress()
plt.close(fig)



for (idx, ann) in enumerate(anns):
  if ann['iscrowd'] == 1:
    continue
  ext_bbox = self.get_extended_bbox(ann)
  
  #Commented out so that always center the object, even if bbox goes over border
  #ext_bbox[0] = max(ext_bbox[0], 0.0) # floats
  #ext_bbox[1] = max(ext_bbox[1], 0.0)
  #ext_bbox[2] = min(ext_bbox[2], self.coco.images[ann['image_id']]['height'])
  #ext_bbox[3] = min(ext_bbox[3], self.coco.images[ann['image_id']]['width'])

  # fill in segmentation mask in the frame of the extended bbox
  all_seg_adjusted_xy = []
  for seg in ann['segmentation']:
    all_seg_adjusted_xy.append(
        [coord-ext_bbox[1] if idx%2==0 else coord-ext_bbox[0] for idx,coord in enumerate(seg)]
        )
  gt_mask = self.coco.segToMask(all_seg_adjusted_xy, 
                                int(ext_bbox[2]-ext_bbox[0]+1), int(ext_bbox[3]-ext_bbox[1]+1))
  gt_mask = zoom(gt_mask, 
                 (self.target_gt_mask_h/float(gt_mask.shape[0]), 
                  self.target_gt_mask_w/float(gt_mask.shape[1])),
                  order=0)
  gt_mask = np.array(gt_mask, dtype=np.uint8)

  mykey = self.get_key_string(idx, ann)
  myd.data = gt_mask.tobytes(order='C')
  txn.put(mykey.encode(), myd.SerializeToString())

  # every 1000, do a commit
  if idx%1000 == 999:
    txn.commit()
    txn = env.begin(write=True)

txn.commit()
env.close()
  