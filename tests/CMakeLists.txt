cmake_minimum_required(VERSION 3.8)
project(tests LANGUAGES CXX)

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

add_executable(MyThreadTest MyThreadTest.cpp)
add_executable(MySocketTest MySocketTest.cpp)

target_compile_features(MyThreadTest PRIVATE cxx_std_11)
target_compile_features(MySocketTest PRIVATE cxx_std_11)
target_compile_options(MyThreadTest
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum -Wno-reserved-id-macro -Wno-unused-macros -Wno-unused-function>
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)
target_compile_options(MySocketTest
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum -Wno-reserved-id-macro -Wno-unused-macros -Wno-unused-function -Wno-writable-strings -Wno-format-nonliteral>
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
)
target_link_libraries(MyThreadTest PRIVATE thread ${log-lib})
target_link_libraries(MySocketTest PRIVATE socket ${log-lib})

if(MSVC)
  set_target_properties(MyThreadTest PROPERTIES FOLDER "tests")
  set_target_properties(MySocketTest PROPERTIES FOLDER "tests")
endif()
