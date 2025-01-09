cmake_minimum_required(VERSION 3.15)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  cmake_policy(SET CMP0135 NEW)
endif()
project(git-heatmap VERSION 1.0)

# 设置默认编译类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "Build type (Release, Debug, RelWithDebInfo, MinSizeRel)."
            FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /EHsc /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
  set(CMAKE_SHARED_LINK_FLAGS
      "${CMAKE_SHARED_LINK_FLAGS} -static-libgcc -static-libstdc++")
  set(CMAKE_MODULE_LINK_FLAGS
      "${CMAKE_MODULE_LINK_FLAGS} -static-libgcc -static-libstdc++")
endif()

option(USE_SYSTEM_LIBGIT2 "use system libgit2" OFF)

if(USE_SYSTEM_LIBGIT2)
  find_package(libgit2 QUIET)
  if(NOT libgit2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBGIT2 REQUIRED libgit2)
    set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIRS})
    set(LIBGIT2_LIBRARY_DIRS ${LIBGIT2_LIBRARY_DIRS})
    set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES})
  else()
    set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
    set(LIBGIT2_LIBRARY_DIRS "") # CMake 模式下通常不需要额外的库目录
    set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES})
  endif()
  message(STATUS "Using system libgit2")
  message(STATUS "  Include dirs: ${LIBGIT2_INCLUDE_DIRS}")
  message(STATUS "  Libraries: ${LIBGIT2_LIBRARIES}")
else()
  set(LIBGIT2_DIR ${CMAKE_CURRENT_BINARY_DIR}/libgit2)
  set(LIBGIT2_CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_CLI=OFF
      -DBUILD_EXAMPLES=OFF
      -DBUILD_FUZZERS=OFF
      -DBUILD_SHARED_LIBS=OFF
      -DBUILD_TESTS=OFF
      -DUSE_AUTH_NEGOTIATE=OFF
      -DUSE_AUTH_NTLM=OFF
      -DUSE_BUNDLED_ZLIB=ON
      -DUSE_HTTPS=OFF
      -DUSE_ICONV=OFF
      -DUSE_SSH=OFF
      -DREGEX_BACKEND=builtin
      -DCMAKE_INSTALL_PREFIX=${LIBGIT2_DIR})
  if(DEFINED CMAKE_TOOLCHAIN_FILE)
    list(APPEND LIBGIT2_CMAKE_ARGS --toolchain=${CMAKE_TOOLCHAIN_FILE})
  endif()
  if(DEFINED CMAKE_OSX_ARCHITECTURES)
    string(REPLACE ";" "^_^" OSX_ARCHITECTURES_LISTSEP
                   "${CMAKE_OSX_ARCHITECTURES}")
    list(APPEND LIBGIT2_CMAKE_ARGS
         -DCMAKE_OSX_ARCHITECTURES=${OSX_ARCHITECTURES_LISTSEP})
  endif()

  message(STATUS "${LIBGIT2_CMAKE_ARG_STR}")
  include(ExternalProject)
  ExternalProject_Add(
    libgit2
    GIT_REPOSITORY https://github.com/libgit2/libgit2.git
    GIT_TAG v1.9.0
    LIST_SEPARATOR "^_^"
    CMAKE_ARGS ${LIBGIT2_CMAKE_ARGS}
    BUILD_IN_SOURCE 0
    INSTALL_DIR ${LIBGIT2_DIR})
  include_directories(SYSTEM ${LIBGIT2_DIR}/include)
  list(PREPEND CMAKE_PREFIX_PATH ${LIBGIT2_DIR}/lib/cmake)
  set(ENV{PKG_CONFIG_PATH} "${LIBGIT2_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
  set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_DIR}/include)
  set(LIBGIT2_LIBRARY_DIRS ${LIBGIT2_DIR}/lib)
  set(LIBGIT2_LIBRARIES git2)

endif()

include(FetchContent)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/shediao/argparse.hpp.git
  GIT_TAG main)
FetchContent_MakeAvailable(argparse)

# 添加可执行文件
add_executable(git-heatmap src/main.cpp src/args.cpp src/git_analyzer.cpp
                           src/glob.cpp src/time_utils.cpp src/git_heatmap.cpp)

if(NOT USE_SYSTEM_LIBGIT2)
  add_dependencies(git-heatmap libgit2)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
  # ASAN
  target_compile_options(
    git-heatmap PRIVATE -O0 -fsanitize=address -fsanitize=undefined
                        -fno-omit-frame-pointer)
  target_link_options(git-heatmap PRIVATE -fsanitize=address
                      -fsanitize=undefined -fno-omit-frame-pointer)
endif()
# 包含目录
target_include_directories(git-heatmap PRIVATE ${LIBGIT2_INCLUDE_DIRS})

# 链接目录
target_link_directories(git-heatmap PRIVATE ${LIBGIT2_LIBRARY_DIRS})

# 链接库
target_link_libraries(git-heatmap PRIVATE ${LIBGIT2_LIBRARIES}
                                          argparse::argparse)

if(MSVC AND NOT USE_SYSTEM_LIBGIT2)
  target_link_libraries(git-heatmap PRIVATE ws2_32 secur32)
endif()

install(TARGETS git-heatmap RUNTIME DESTINATION bin)
