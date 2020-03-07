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

include(CheckTypeSize)
set(CMAKEDEFINE_INT32_T "int")
set(CMAKEDEFINE_UINT32_T "unsigned int")
set(CMAKEDEFINE_LONG_T "long")
set(CMAKEDEFINE_ULONG_T "unsigned long")
set(CMAKEDEFINE_INT8_T "char")
set(CMAKEDEFINE_UINT8_T "unsigned char")
set(CMAKEDEFINE_INT16_T "short")
set(CMAKEDEFINE_UINT16_T "unsigned short")
set(CMAKEDEFINE_VOID_T "void")

set(CMAKEDEFINE_UINT32_MASK "0xFFFFFFFF")
set(CMAKEDEFINE_UINT8_MASK "0xFF")
set(CMAKEDEFINE_UINT16_MASK "0xFFFF")

CHECK_TYPE_SIZE(long CMAKE_SIZEOF_LONG)
if(CMAKE_SIZEOF_LONG STREQUAL 4)
    set(CMAKEDEFINE_LD_T "\"%d\"")
    set(CMAKEDEFINE_LU_T "\"%u\"")
    set(CMAKEDEFINE_LX_T "\"%x\"")
    set(CMAKEDEFINE_ULONG_MASK "0xFFFFFFFF")
    set(CMAKEDEFINE_BYTE "4")
else(CMAKE_SIZEOF_LONG STREQUAL 4)
    set(CMAKEDEFINE_LD_T "\"%ld\"")
    set(CMAKEDEFINE_LU_T "\"%lu\"")
    set(CMAKEDEFINE_LX_T "\"%lx\"")
    set(CMAKEDEFINE_ULONG_MASK "0xFFFFFFFFFFFFFFFF")
    set(CMAKEDEFINE_BYTE "8")
endif(CMAKE_SIZEOF_LONG STREQUAL 4)

include(ExternalProject)
set(BINUTILS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/libs/binutils-2.34 CACHE FILEPATH "Where to find sources for binutils-2.34.")
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

include_directories(${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/usr/include)

include(CheckIncludeFile)
include(CheckIncludeFiles)

check_include_file("stdio.h" HAVE_STDIO_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("stdarg.h" HAVE_STDARG_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("time.h" HAVE_TIME_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("sys/ptrace.h" HAVE_SYS_PTRACE_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/syscall.h" HAVE_SYS_SYSCALL_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/user.h" HAVE_SYS_USER_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)

# include(CheckIncludeFileCXX)
# check_include_file_cxx("iostream" HAVE_IOSTREAM_H)
# check_include_file_cxx("list" HAVE_LIST_H)
# check_include_file_cxx("memory" HAVE_MEMORY_H)
# check_include_file_cxx("fstream" HAVE_FSTREAM_H)

configure_file (
    "${PROJECT_SOURCE_DIR}/include/config.h.in"
    "${PROJECT_SOURCE_DIR}/include/config.h"
    @ONLY
)

find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
    set(THREAD_LIBRARY ${CMAKE_THREAD_LIBS_INIT})
else(CMAKE_THREAD_LIBS_INIT)
    message(FATAL_ERROR "pthread lib could not be found")
endif(CMAKE_THREAD_LIBS_INIT)

add_executable(patchtool
    ${PROJECT_SOURCE_DIR}/src/logger.c
    ${PROJECT_SOURCE_DIR}/src/sysmem.cpp
    ${PROJECT_SOURCE_DIR}/src/binelf.cpp
    ${PROJECT_SOURCE_DIR}/src/patch.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)

add_dependencies(patchtool
    bfd
    opcodes
    iberty
)

add_definitions(
    -g
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

add_library(extdynlib
    SHARED
    ${PROJECT_SOURCE_DIR}/src/logger.c
    ${PROJECT_SOURCE_DIR}/example/extdynlib.c)

set_target_properties(extdynlib
    PROPERTIES
    VERSION 1.0.1
    SOVERSION 1)

add_library(dylib
    SHARED
    ${PROJECT_SOURCE_DIR}/src/logger.c
    ${PROJECT_SOURCE_DIR}/example/linkdylib.c)

set_target_properties(dylib
    PROPERTIES
    VERSION 1.0.1
    SOVERSION 1)

add_executable(recipient
    ${PROJECT_SOURCE_DIR}/src/logger.c
    ${PROJECT_SOURCE_DIR}/example/recipient.c
)

add_dependencies(recipient dylib)

target_link_libraries(recipient
    ${CMAKE_BINARY_DIR}/libdylib.so
    -ldl)



