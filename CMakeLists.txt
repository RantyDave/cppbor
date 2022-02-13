cmake_minimum_required (VERSION 3.16)
project (cppbor)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

add_library(cppbor src/cppbor.cpp)
target_include_directories(cppbor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
