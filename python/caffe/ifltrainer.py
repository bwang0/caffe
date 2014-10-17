#!/usr/bin/env python
"""
CNN IFL Trainer
"""

import numpy as np
import copy

from google.protobuf import text_format

import caffe

class IflTrainer():
    def __init__(self, solver_fn, pretrained_model_fn):
        self.solver_fn = solver_fn
        self.net_fn = ''
        self.pretrained_model_fn = pretrained_model_fn

        self.solver_param = []
        self.net_param = []

    def training_loop(self, iter_i):
        self.solver_param[iter_i] = caffe_pb2.SolverParameter()
        text_format.Merge(open(self.solver_fn).read(), solver_param[iter_i])
        self.net_fn = solver_param.net
        self.net_param[iter_i] = caffe_pb2.NetParameter()
        text_format.Merge(open(self.net_fn).read(), self.net_param[iter_i])

        # evaluate here and decide on new params
        net_model = caffe.Net(self.net_fn, self.pretrained_model_fn)

        # get new net_param and net_model
        (new_net_param, new_net_model) = \
            self.add_feature_maps('conv1', 30, self.net_param[iter_i], net_model)

        # reload solver + net_model

        # SolverResume

    def evaluate_performance(self):
        pass

    def add_feature_maps(self, layer_name, num_additional_maps, old_net_param, old_net_model):
        # create new net_param
        new_net_param = copy.deepcopy(old_net_param)
        for layer in new_net_param.layers:
            if layer.name == layer_name:
                curr_num_maps = layer.convolution_param.num_output
                layer.convolution_param.num_output = curr_num_maps + num_additional_maps
                break

        # create new weights after adding feature maps
        new_net_model = 

        # write new new_param

        # write new weights
        

    def create_new_conv_column(self):
        pass

