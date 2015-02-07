# vim: sw=3 ts=3 expandtab smartindent
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake-modules)

#option(GCC_LINK_TIME_OPTIMIZATION "Enable GCC Link Time Optimization (LTO)" ON)
#if (GCC_LINK_TIME_OPTIMIZATION)
    #add_compile_options(-flto=jobserver)
    #set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=jobserver")
    #set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto=jobserver")
    #set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto=jobserver")
    #set(CMAKE_CXX_ARCHIVE_CREATE "/usr/bin/gcc-ar cr <TARGET> <OBJECTS>")
    #set(CMAKE_CXX_ARCHIVE_APPEND "/usr/bin/gcc-ar r <TARGET> <OBJECTS>")
    #set(CMAKE_CXX_ARCHIVE_FINISH "/usr/bin/gcc-ranlib <TARGET>")
    #set(CMAKE_AR "/usr/bin/gcc-ar")
    #set(CMAKE_RANLIB "/usr/bin/gcc-ranlib")
#endif()

project(epoll-executor)

include(CodeCoverage)
include(EnableExtraCompilerWarnings)
globally_enable_extra_compiler_warnings()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_COVERAGE})
   set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_COVERAGE})
endif()
add_compile_options(-std=c++1y)
add_compile_options(-Werror)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
   add_definitions(-DEPOLLING_IS_LINUX=0 -DEPOLLING_IS_WINDOWS=1 -DEPOLLING_IS_FREEBSD=0)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
   add_definitions(-DEPOLLING_IS_LINUX=1 -DEPOLLING_IS_WINDOWS=0 -DEPOLLING_IS_FREEBSD=0)
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
   add_definitions(-DEPOLLING_IS_LINUX=0 -DEPOLLING_IS_WINDOWS=0 -DEPOLLING_IS_FREEBSD=1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src /home/andrew/third-party/executors/include)

add_subdirectory(src)
add_subdirectory(application/unit_tests)
