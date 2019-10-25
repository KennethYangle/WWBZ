cmake_minimum_required( VERSION 2.8 )
project( WWBZ )

if(MINGW)
    add_definitions("-D_USE_MATH_DEFINES") # For M_PI
endif()
if(MSVC)
    add_definitions("-std=c++11 -WX -W2")
    add_definitions("-D_USE_MATH_DEFINES") # For M_PI
else()
    add_definitions("-std=c++11 -Wall -Wextra -Werror")
endif()

set( CMAKE_BUILD_TYPE Release )
set( CMAKE_CXX_FLAGS "-std=c++11 -pthread -O3" )         # 添加 C++ 11 标准支持

find_package( OpenCV )                          # 寻找 OpenCV 库
include_directories( ${OpenCV_INCLUDE_DIRS} )   # 添加头文件
include_directories( ./include )

add_library( CShareMemory SHARED ./src/share_memory.cpp ) # 编译为共享库

add_executable( circle ./src/circle_in_real.cpp )
target_link_libraries( circle ${OpenCV_LIBS} CShareMemory )   # 添加动态链接库
# add_executable( main_2 ./src/main_2.cpp )
# target_link_libraries( main_2 CShareMemory )

find_package(MAVSDK REQUIRED)
add_executable(offboard
    ./src/offboard_velocity.cpp
)
target_link_libraries(offboard
    MAVSDK::mavsdk_action
    MAVSDK::mavsdk_offboard
    MAVSDK::mavsdk_telemetry
    MAVSDK::mavsdk
    CShareMemory
)