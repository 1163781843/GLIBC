#########################################################################
# File Name: build2host.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2019年03月25日 星期一 09时21分37秒
#########################################################################
#!/bin/bash

../glibc-2.29/configure \
    --prefix=${PWD}/build \
    --disable-silent-rules \
     --disable-dependency-tracking \
     --enable-kernel=2.6.32 \
     --without-cvs \
     --disable-profile \
     --without-gd \
     --enable-clocale=gnu \
     --enable-add-ons \
     --without-selinux \
     --enable-obsolete-rpc \
     --enable-nscd

