#########################################################################
# File Name: build2malloc.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2019年03月29日 星期五 14时24分43秒
#########################################################################
#!/bin/bash

export CUR_PATH="$(cd `dirname $0`; pwd)"

export CC=/usr/bin/gcc
export INCLUDE_DIR=${CUR_PATH}/../build/build/include/
export LIB_DIR=${CUR_PATH}/../build/build/lib/

${CC} -g -DGRANDSTREAM_NETWORKS -I${INCLUDE_DIR} -L${LIB_DIR} -Wl,-rpath=${LIB_DIR} malloc.c -lpthread -Wl,--dynamic-linker=${LIB_DIR}/ld-linux-x86-64.so.2

