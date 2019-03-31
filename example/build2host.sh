#########################################################################
# File Name: build2malloc.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2019年03月29日 星期五 14时24分43秒
#########################################################################
#!/bin/bash

export CUR_PATH="$(cd `dirname $0`; pwd)"

export CC="/usr/bin/gcc"

export CFLAGS="-g \
    -I${CUR_PATH}/../build/build/include \
    -DGRANDSTREAM_NETWORKS"

export LDFLAGS=" \
    -L${CUR_PATH}/../build/build/lib/ \
    -Wl,-rpath=${CUR_PATH}/../build/build/lib/ \
    -Wl,--dynamic-linker=${CUR_PATH}/../build/build/lib/ld-linux-x86-64.so.2"

./bootstrap.sh

./configure
