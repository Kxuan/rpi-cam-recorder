cmake_minimum_required(VERSION 3.14)
project(v4l2)

set(CMAKE_CXX_STANDARD 17)

add_executable(v4l2 main.cpp V4l2Device.cpp V4l2Device.h)