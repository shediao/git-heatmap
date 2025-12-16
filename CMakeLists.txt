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
if(MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Qunused-arguments) # libgit2
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET
    "13.3"
    CACHE STRING "Deployment target" FORCE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(
  libgit2
  GIT_REPOSITORY "https://github.com/libgit2/libgit2.git"
  GIT_TAG v1.9.1)

set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_CLI
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_FUZZERS
    OFF
    CACHE BOOL "" FORCE)
set(USE_SSH
    OFF
    CACHE BOOL "" FORCE)
set(USE_HTTPS
    OFF
    CACHE BOOL "" FORCE)
set(USE_NTLMCLIENT
    OFF
    CACHE BOOL "" FORCE)
set(REGEX_BACKEND
    "builtin"
    CACHE STRING "" FORCE)
set(USE_BUNDLED_ZLIB
    ON
    CACHE BOOL "" FORCE)
set(USE_ICONV
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(libgit2)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/shediao/argparse.hpp.git
  GIT_TAG v0.0.13)
FetchContent_MakeAvailable(argparse)

add_executable(${PROJECT_NAME} src/main.cpp src/args.cpp src/glob.cpp
                               src/utils.cpp src/heatmap.cpp src/terminal.cpp)

target_include_directories(${PROJECT_NAME}
                           PRIVATE "${libgit2_SOURCE_DIR}/include")
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /EHsc /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic
                                                 -Wshadow -Werror)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
  # ASAN
  target_compile_options(
    ${PROJECT_NAME} PRIVATE -O0 -fsanitize=address -fsanitize=undefined
                            -fno-omit-frame-pointer)
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address
                      -fsanitize=undefined -fno-omit-frame-pointer)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE libgit2package argparse::argparse)

if(MINGW)
  target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-mconsole")
endif()

if(LINUX)
  target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
