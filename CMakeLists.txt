cmake_minimum_required(VERSION 3.10)
project(platform-sample)

if(MSVC)
  # Not create ZERO_CHECK project
  set(CMAKE_SUPPRESS_REGENERATION true)
  # Use the FOLDER target property
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if(ANDROID)
  find_library(log-lib log)
endif()

#add_subdirectory(func)
add_subdirectory(thread)
add_subdirectory(socket)

add_subdirectory(tests)
