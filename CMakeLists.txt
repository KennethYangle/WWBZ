cmake_minimum_required( VERSION 2.8 )
project( WWBZ )

set( CMAKE_BUILD_TYPE Release )
set( CMAKE_CXX_FLAGS "-std=c++11 -pthread -O3" )         # 添加 C++ 11 标准支持

find_package( OpenCV )                          # 寻找 OpenCV 库
include_directories( ${OpenCV_INCLUDE_DIRS} )   # 添加头文件
include_directories( ./include )

add_library( CShareMemory SHARED ./src/share_memory.cpp ) # 编译为共享库

add_executable( circle ./src/circle_in_real.cpp )
target_link_libraries( circle ${OpenCV_LIBS} CShareMemory )   # 添加动态链接库
add_executable( main_2 ./src/main_2.cpp )
target_link_libraries( main_2 CShareMemory )