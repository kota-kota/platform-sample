cmake_minimum_required(VERSION 3.8)
project(func LANGUAGES CXX)

set(SOURCES
  MyFunc.hpp
  MyFunc.cpp
)

add_library(func STATIC ${SOURCES})
target_compile_features(socket PRIVATE cxx_std_11)
target_compile_options(socket
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum>
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)

target_include_directories(func PUBLIC ./)
