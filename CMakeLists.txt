cmake_minimum_required(VERSION 3.15)
project(server)

#set(CMAKE_CXX_STANDARD 14)

# 设置编译选项
# 允许c++14标准、O3优化、多线程。match选项可避免一些cpu上的问题
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=native -O3 -pthread" )


#include and src file
file(GLOB_RECURSE project_header *.h)
file(GLOB_RECURSE project_cpp *.cpp)

#exe
#add_executable(${PROJECT_NAME} ${project_cpp})

add_executable(${PROJECT_NAME} main.cpp server.cpp server.h common.h thread.cpp thread.h)