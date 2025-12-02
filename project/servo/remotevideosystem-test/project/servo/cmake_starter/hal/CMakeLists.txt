cmake_minimum_required(VERSION 3.10)
project(hal_module C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#Include paths for headers (optional)
include_directories(
  ${CMAKE_SOURCE_DIR}/app/include
  ${CMAKE_SOURCE_DIR}/hal/include
) 