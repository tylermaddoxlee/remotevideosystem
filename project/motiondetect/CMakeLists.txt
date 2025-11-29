cmake_minimum_required(VERSION 3.10)
project(motioncam)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(motioncam opencv_test2.cpp)
target_link_libraries(motioncam ${OpenCV_LIBS})
