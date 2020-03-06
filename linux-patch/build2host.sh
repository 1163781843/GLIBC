#########################################################################
# File Name: build2host.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2020年03月04日 星期三 11时02分35秒
#########################################################################
#!/bin/bash

export CC="/usr/bin/gcc"
export CXX="/usr/bin/g++"

export CFLAGS="-g"
export CXXFLAGS="${CFLAGS} -std=c++11"
export CPPFLAGS="${CFLAGS} -std=c++11"
export buildDIR="buildDIR"

rm -rf ${buildDIR}; mkdir -p ${buildDIR}; cd ${buildDIR}

cmake ..



