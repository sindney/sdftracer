cmake_minimum_required(VERSION 3.0)

project(SDFTracer)

set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O2 -NDEBUG")

set(GLM_INCLUDE "/usr/local/include" CACHE PATH "Location of glm headers.")
set(STB_INCLUDE "/usr/local/include" CACHE PATH "Location of stb_image headers.")

include_directories(${STB_INCLUDE})
include_directories(${GLM_INCLUDE})

add_executable(sdftracer main.cpp)

install(TARGETS sdftracer DESTINATION bin)
