# gflags
wget https://github.com/schuhschuh/gflags/archive/master.zip
unzip master.zip
cd gflags-master
mkdir build && cd build
export CXXFLAGS="-fPIC" && cmake -D CMAKE_INSTALL_PREFIX=/mnt/neocortex/scratch/bhwang/subsystem .. && make VERBOSE=1
make && make install

# lmdb
git clone git://gitorious.org/mdb/mdb.git
cd mdb/libraries/liblmdb
# edit Makefile to install it to subsystem
make && make install