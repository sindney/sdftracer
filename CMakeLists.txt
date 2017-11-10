cmake_minimum_required(VERSION 3.0)

project(SDFTracer)

set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O2 -NDEBUG")

set(GLM_INCLUDE "/usr/local/include" CACHE PATH "Location of glm headers.")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${GLM_INCLUDE})

add_executable(sdftracer main.cpp)

install(TARGETS sdftracer DESTINATION bin)