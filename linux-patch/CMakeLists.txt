# Copyright (c) 2006, 2017, Oracle and/or its affiliates.
# Copyright (c) 2008, 2018, MariaDB Corporation
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

project(SIPTOOLS)

cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_BUILD_TYPE "Debug")

include(ExternalProject)

set(BINUTILS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/binutils-2.34 CACHE FILEPATH "Where to find sources for binutils-2.34.")
set(binutils_configure_opts --with-system-zlib --enable-host-shared)
list(APPEND binutils_configure_opts "CC=${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1}")

file(GLOB BINUTILS_ALL_FILES ${BINUTILS_SOURCE_DIR}/*)
ExternalProject_Add(libbinutils
    PREFIX
        build_binutils
    DOWNLOAD_COMMAND
        cp -a "${BINUTILS_ALL_FILES}" "<SOURCE_DIR>/"
    CONFIGURE_COMMAND
        "<SOURCE_DIR>/configure" ${binutils_configure_opts}
        "--prefix=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr"
        "--libdir=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib"
    BUILD_COMMAND
        make
    INSTALL_COMMAND
        make install
)

add_library(bfd STATIC IMPORTED)
set_target_properties(bfd PROPERTIES IMPORTED_LOCATION
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libbfd.a")

add_library(opcodes STATIC IMPORTED)
set_target_properties(opcodes PROPERTIES IMPORTED_LOCATION
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libopcodes.a")

add_library(iberty STATIC IMPORTED)
set_target_properties(iberty PROPERTIES IMPORTED_LOCATION
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libiberty.a")

add_dependencies(bfd libbinutils)
add_dependencies(opcodes libbinutils)
add_dependencies(iberty libbinutils)

set(BFD_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libbfd.a)
set(OPCODES_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libopcodes.a)
set(IBERTY_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/lib/libiberty.a)
set(Z_LIBRARY -lz)
set(DL_LIBRARY -ldl)

include_directories(${PROJECT_SOURCE_DIR}/patch
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/include)

find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
    set(THREAD_LIBRARY ${CMAKE_THREAD_LIBS_INIT})
else(CMAKE_THREAD_LIBS_INIT)
    message(FATAL_ERROR "pthread lib could not be found")
endif(CMAKE_THREAD_LIBS_INIT)

find_package(LibXml2)
if(LIBXML2_LIBRARY)
else(LIBXML2_LIBRARY)
    message(FATAL_ERROR "xml2 lib could not be found")
endif(LIBXML2_LIBRARY)

add_executable(patchtool
    ${PROJECT_SOURCE_DIR}/patch/logger.c
    ${PROJECT_SOURCE_DIR}/patch/patch.cpp
)

add_dependencies(patchtool
    bfd
    opcodes
    iberty
)

add_definitions(
    -g
    -std=c++11
    -DPACKAGE
    -DPACKAGE_VERSION
    -D_GNU_SOURCE
)

target_link_libraries(patchtool
    ${THREAD_LIBRARY}
    ${BFD_LIBRARY}
    ${OPCODES_LIBRARY}
    ${IBERTY_LIBRARY}
    ${Z_LIBRARY}
    ${DL_LIBRARY})

install(PROGRAMS
    ${CMAKE_BINARY_DIR}/patchtool
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/sbin)
