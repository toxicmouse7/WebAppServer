cmake_minimum_required(VERSION 3.21)
project(WebAppServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(WebAppServer main.cpp ClientRepr.hpp Server.cpp Server.hpp)
