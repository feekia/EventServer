cmake_minimum_required(VERSION 3.6)

project(Event)

SET(CMAKE_CXX_COMPILER "clang++")         #设置编译器（可选的有gcc，g++）

set( CMAKE_BUILD_TYPE debug )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fexceptions")


#设定可执行二进制文件的目录
SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) 

#设定存放编译出来的库文件的目录
SET( LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib) 

#并且把该目录设为连接目录
LINK_DIRECTORIES( ${PROJECT_SOURCE_DIR}/lib)

#设定头文件目录
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/handler)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/network)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/utils)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/timer)
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(handler)
add_subdirectory(network)
add_subdirectory(timer)
add_subdirectory(examples)
