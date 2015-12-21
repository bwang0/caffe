##############################
## ~/privatemodules/default
##############################
# module load gcc/4.8.0
# module load git
# module load python/2.7.5
# 
# module load cuda/6.5
# module load mkl
# module load boost/1.57.0-gcc-4.8.0
# module load opencv
# 
# module load protobuf
# module load hdf5/1.8.14/gcc/4.8.0
# 
# module load matlab/2014a
##############################

## Refer to http://caffe.berkeleyvision.org/installation.html
# Contributions simplifying and improving our build system are welcome!

# cuDNN acceleration switch (uncomment to build with cuDNN).
# USE_CUDNN := 1

# CPU-only switch (uncomment to build without GPU support).
# CPU_ONLY := 1

# To customize your choice of compiler, uncomment and set the following.
# N.B. the default for Linux is g++ and the default for OSX is clang++
# CUSTOM_CXX := g++

# CUDA directory contains bin/ and lib/ directories that we need.
CUDA_DIR := /usr/cac/rhel6/cuda/6.5
# On Ubuntu 14.04, if cuda tools are installed via
# "sudo apt-get install nvidia-cuda-toolkit" then use this instead:
# CUDA_DIR := /usr

# CUDA architecture setting: going with all of them.
# For CUDA < 6.0, comment the *_50 lines for compatibility.
CUDA_ARCH := -gencode arch=compute_20,code=sm_20 \
		-gencode arch=compute_20,code=sm_21 \
		-gencode arch=compute_30,code=sm_30 \
		-gencode arch=compute_35,code=sm_35 \
		-gencode arch=compute_50,code=sm_50 \
		-gencode arch=compute_50,code=compute_50

# BLAS choice:
# atlas for ATLAS (default)
# mkl for MKL
# open for OpenBlas
BLAS := mkl
# Custom (MKL/ATLAS/OpenBLAS) include and lib directories.
# Leave commented to accept the defaults for your choice of BLAS
# (which should work)!
BLAS_INCLUDE := /home/software/rhel6/intel-14.0/mkl/include
BLAS_LIB := /home/software/rhel6/intel-14.0/mkl/lib

# This is required only if you will compile the matlab interface.
# MATLAB directory should contain the mex binary in /bin.
# MATLAB_DIR := /usr/local
# MATLAB_DIR := /Applications/MATLAB_R2012b.app

# NOTE: this is required only if you will compile the python interface.
# We need to be able to find Python.h and numpy/arrayobject.h.
PYTHON_INCLUDE := /home/software/rhel6/python/2.7.5/include/python2.7 \
		/home/software/rhel6/python/2.7.5/lib/python2.7/site-packages/numpy/core/include
# Anaconda Python distribution is quite popular. Include path:
# Verify anaconda location, sometimes it's in root.
# ANACONDA_HOME := $(HOME)/anaconda
# PYTHON_INCLUDE := $(ANACONDA_HOME)/include \
		# $(ANACONDA_HOME)/include/python2.7 \
		# $(ANACONDA_HOME)/lib/python2.7/site-packages/numpy/core/include \

# We need to be able to find libpythonX.X.so or .dylib.
PYTHON_LIB := /home/software/rhel6/python/2.7.5/lib
# PYTHON_LIB := $(ANACONDA_HOME)/lib

# Uncomment to support layers written in Python (will link against Python libs)
# WITH_PYTHON_LAYER := 1

# Whatever else you find you need goes here.
INCLUDE_DIRS := $(PYTHON_INCLUDE) \
		/home/software/rhel6/protobuf/2.5.0/include \
		/home/software/rhel6/glog/0.33/include \
		/home/software/rhel6/gflags/2.1.1/include \
		/home/software/rhel6/leveldb/1.15.0/include \
		/home/software/rhel6/snappy/1.1.1/include \
		/home/software/rhel6/lmdb/0.9.10/liblmdb \
		/home/software/rhel6/hdf5-1.8.14/gcc-4.8.0/include \
		/home/software/rhel6/opencv/2.4.9/include \
		/home/software/rhel6/boost/1.57.0-gcc-4.8.0/include \
		/home/software/rhel6/intel-14.0/mkl/include

LIBRARY_DIRS := $(PYTHON_LIB) \
		/home/software/rhel6/protobuf/2.5.0/lib \
		/home/software/rhel6/glog/0.33/lib \
		/home/software/rhel6/gflags/2.1.1/lib \
		/home/software/rhel6/leveldb/1.15.0 \
		/home/software/rhel6/snappy/1.1.1/lib \
		/home/software/rhel6/lmdb/0.9.10/liblmdb \
		/home/software/rhel6/hdf5-1.8.14/gcc-4.8.0/lib \
		/home/software/rhel6/opencv/2.4.9/lib \
		/home/software/rhel6/boost/1.57.0-gcc-4.8.0/lib \
		/home/software/rhel6/intel-14.0/mkl/lib/intel64 \
		/usr/lib

# Uncomment to use `pkg-config` to specify OpenCV library paths.
# (Usually not necessary -- OpenCV libraries are normally installed in one of the above $LIBRARY_DIRS.)
# USE_PKG_CONFIG := 1

BUILD_DIR := build
DISTRIBUTE_DIR := distribute

# Uncomment for debugging. Does not work on OSX due to https://github.com/BVLC/caffe/issues/171
# DEBUG := 1

# The ID of the GPU that 'make runtest' will use to run unit tests.
TEST_GPUID := 0

# enable pretty build (comment to see full commands)
Q ?= @
