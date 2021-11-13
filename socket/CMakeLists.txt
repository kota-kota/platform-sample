cmake_minimum_required(VERSION 3.8)
project(socket LANGUAGES CXX)

set(common_SOURCES
  MySocket.hpp
  Logger.hpp
  Logger.cpp
)

if(WIN32)
  set(SOURCES
    select/MySocket.cpp
  )
else()
  set(SOURCES
    epoll/MySocket.cpp
  )
endif()

if(WIN32)
  #find_library(ws2_32_LIBRARY ws2_32 REQUIRED)
  set(ws2_32_LIBRARY ws2_32.lib)
else()
  find_package(Threads REQUIRED)
endif()

add_library(socket STATIC ${SOURCES} ${common_SOURCES})
target_compile_features(socket PRIVATE cxx_std_11)
target_compile_options(socket
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum -Wno-writable-strings -Wno-format-nonliteral>
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)

target_link_libraries(socket PRIVATE ${ws2_32_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(socket PUBLIC ./)
