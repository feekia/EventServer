cmake_minimum_required(VERSION 3.6)

project(ES)

SET(CMAKE_CXX_COMPILER "g++")         #设置编译器（可选的有gcc，g++,clang++）

set( CMAKE_BUILD_TYPE debug )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIC -pipe -std=c++11 -fexceptions")


#设定可执行二进制文件的目录
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin) 

#设定存放编译出来的库文件的目录
SET( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib) 

#并且把该目录设为连接目录
LINK_DIRECTORIES( ${PROJECT_SOURCE_DIR}/build/lib)

#设定头文件目录
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/handler)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/network)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/utils)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/timer)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/codec)

aux_source_directory(handler ES_SRCS)
aux_source_directory(network ES_SRCS)
aux_source_directory(codec ES_SRCS)
aux_source_directory(utils ES_SRCS)

add_library (es ${ES_SRCS})
add_subdirectory(examples)
