set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Assume `/usr/aarch64-linux-gnu/` exists
# (e.g. though `sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu` on Ubuntu)
set(GCC_ARM_TOOLCHAIN "/usr")

# Target triple
set(TARGET_TRIPLE aarch64-linux-gnu)

# C/C++ toolchain
set(GCC_ARM_SYSROOT "${GCC_ARM_TOOLCHAIN}/${TARGET_TRIPLE}")
set(CMAKE_SYSROOT ${GCC_ARM_SYSROOT})
include_directories(${CMAKE_SYSROOT}/include ${CMAKE_SYSROOT}/include/c++/9/aarch64-linux-gnu)
link_directories(${CMAKE_SYSROOT}/lib)

# C compiler
set(CMAKE_C_COMPILER_TARGET ${TARGET_TRIPLE})
set(CMAKE_C_COMPILER "clang" CACHE STRING "clang compiler" FORCE)

# C compiler options
set(CMAKE_C_FLAGS_INIT "-Weverything -Werror -Wno-padded -Wno-covered-switch-default -Wno-switch-enum -B${GCC_ARM_TOOLCHAIN}" CACHE STRING "c flags init" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Og" CACHE STRING "c flags debug" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O3" CACHE STRING "c flags release" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Oz" CACHE STRING "c flags minsizerel" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "c flags relwithdebinfo" FORCE)

# C++ compiler
set(CMAKE_CXX_COMPILER_TARGET ${TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "clang++ compiler" FORCE)

# C++ compiler options
set(CMAKE_CXX_FLAGS_INIT "-Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-covered-switch-default -Wno-switch-enum -B${GCC_ARM_TOOLCHAIN}" CACHE STRING "c++ flags init" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Og" CACHE STRING "c++ flags debug" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE STRING "c++ flags release" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz" CACHE STRING "c++ flags minsizerel" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "c++ flags relwithdebinfo" FORCE)

# Exe linker options
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# Root path
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

# Search for programs in the build host directories
# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
