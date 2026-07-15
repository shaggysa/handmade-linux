cmake_minimum_required(VERSION 4.2)
project(handmade_linux C)

set(CMAKE_C_STANDARD 99)

add_executable(handmade_linux main.c)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

target_link_libraries(handmade_linux PRIVATE
        ${X11_LIBRARIES}
        ${X11_Xrender_LIB}
)
