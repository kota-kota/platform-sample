cmake_minimum_required(VERSION 3.8)
project(thread LANGUAGES CXX)

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

if(NOT WIN32)
  find_package(Threads REQUIRED)
endif()

set(SOURCES MyThread.cpp MyThread.hpp)

add_library(thread STATIC ${SOURCES})
target_compile_features(thread PRIVATE cxx_std_11)
target_compile_options(thread
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum>
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)
target_link_libraries(thread PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(thread PUBLIC ./)
