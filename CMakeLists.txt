cmake_minimum_required(VERSION 2.8)
project(lux-server)
set(CMAKE_BUILD_TYPE Debug)

set(LUX_SERVER_VERSION_MAJOR 0)
set(LUX_SERVER_VERSION_MINOR 0)
set(LUX_SERVER_VERSION_PATCH 0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -Og -pipe \
    -fasynchronous-unwind-tables -fstack-protector-strong -D_GLIBCXX_ASSERTIONS\
    -ftrapv")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -flto -DNDEBUG")
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "enabling link-time optimizations")

    set(CMAKE_AR "gcc-ar")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH true)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

configure_file("${PROJECT_SOURCE_DIR}/config.hpp.in"
               "${PROJECT_BINARY_DIR}/include/config.hpp")

include_directories(SYSTEM "include")
include_directories("deps/lux-shared/include"
                    "src" "${PROJECT_BINARY_DIR}/include"
                    "${PROJECT_BINARY_DIR}/deps/lux-shared/include")
add_subdirectory("deps/lux-shared")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(lux-server ${SOURCES})

find_package(Threads REQUIRED)
find_library(ENET_LIB enet)

target_link_libraries(lux-server lux)
target_link_libraries(lux-server Threads::Threads)
target_link_libraries(lux-server "${ENET_LIB}")

