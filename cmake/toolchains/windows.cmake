set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(WINDOWS_SDK_VERSION 10.0.26100.0)
set(WINDOWS_SDK_ROOT
    "/Users/shediao/Downloads/eb715acd3d/winsdk/10"
    CACHE PATH "Path to a Windows SDK and CRT installation")

set(WINDOWS_VC_ROOT "/Users/shediao/Downloads/eb715acd3d/VC")

set(CMAKE_FIND_ROOT_PATH ${WINDOWS_SDK_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES
    ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/ucrt
    ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/um
    ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/shared
    ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/winrt
    ${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/include/)

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)

set(CMAKE_SYSROOT "/Users/shediao/Downloads/eb715acd3d/Windows\ Kits/10")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -target x86_64-pc-windows-msvc  /imsvc ${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/atlmfc/include  /imsvc ${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/include  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/ucrt  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/um  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/shared  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/winrt -Wno-unknown-argument /std:c++20 -Wno-c++98-compat -Wno-c++17-compat /EHsc -Wno-unused-command-line-argument /utf-8"
)
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -target x86_64-pc-windows-msvc  /imsvc ${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/atlmfc/include  /imsvc ${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/include  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/ucrt  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/um  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/shared  /imsvc ${WINDOWS_SDK_ROOT}/Include/${WINDOWS_SDK_VERSION}/winrt -Wno-unknown-argument -Wno-unused-command-line-argument  /utf-8"
)
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

add_link_options(
  /manifest:no
  -libpath:${WINDOWS_VC_ROOT}/Tools/MSVC/14.41.34120/lib/x64
  -libpath:${WINDOWS_SDK_ROOT}/Lib/${WINDOWS_SDK_VERSION}/x64
  -libpath:${WINDOWS_SDK_ROOT}/Lib/${WINDOWS_SDK_VERSION}/ucrt/x64
  -libpath:${WINDOWS_SDK_ROOT}/Lib/${WINDOWS_SDK_VERSION}/um/x64)
