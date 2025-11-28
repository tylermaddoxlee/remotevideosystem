cmake_minimum_required(VERSION 3.10)
project(beatbox C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_definitions(_POSIX_C_SOURCE=200809L _GNU_SOURCE)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/include
)

set(APP_SOURCES
    src/main.c

    ../hal/src/servo.c
    ../hal/src/joystick.c
)

add_executable(project ${APP_SOURCES})

find_package(ALSA REQUIRED)
target_link_libraries(project PRIVATE asound pthread rt gpiod m)

