#########################################################################
# File Name: build2host.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2019年03月25日 星期一 09时21分37秒
#########################################################################
#!/bin/bash

export CFLAGS=" \
    -DGRANDSTREAM_NETWORKS"

./configure \
    --prefix=/data/mem-management \
    --enable-log

