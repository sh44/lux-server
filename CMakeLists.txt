cmake_minimum_required(VERSION 2.8)
project(lux-server)
set(CMAKE_BUILD_TYPE Debug)

set(LUX_SERVER_VERSION_MAJOR 0)
set(LUX_SERVER_VERSION_MINOR 0)
set(LUX_SERVER_VERSION_PATCH 0)

set(THREADS_PREFER_PTHREAD_FLAG ON)

configure_file("${PROJECT_SOURCE_DIR}/config.h.in"
               "${PROJECT_BINARY_DIR}/include/config.h")

include_directories("include" "deps/lux-shared/include"
                    "src" "${PROJECT_BINARY_DIR}/include")
add_subdirectory("deps/lux-shared")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(lux-server ${SOURCES})

find_package(Threads REQUIRED)
find_library(ENET_LIB enet)
find_library(BULLET_DYNAMICS_LIB  BulletDynamics)
find_library(BULLET_COLLISION_LIB BulletCollision)
find_library(BULLET_LINEAR_LIB    LinearMath)
find_path(BULLET_INC bullet)

target_link_libraries(lux-server lux)
target_link_libraries(lux-server Threads::Threads)
target_link_libraries(lux-server "${ENET_LIB}")
target_link_libraries(lux-server "${BULLET_DYNAMICS_LIB}")
target_link_libraries(lux-server "${BULLET_COLLISION_LIB}")
target_link_libraries(lux-server "${BULLET_LINEAR_LIB}")
target_include_directories(lux-server PUBLIC "${BULLET_INC}/bullet")

