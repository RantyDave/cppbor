cmake_minimum_required (VERSION 3.11.4)
project (cppbor)

set(CMAKE_CXX_COMPILER /Library/Developer/CommandLineTools/usr/bin/clang++)
set(CMAKE_CXX_STANDARD 17)
include_directories(SYSTEM /Library/Developer/CommandLineTools/usr/include/c++/v1)
link_directories(/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib)
set(CMAKE_EXE_LINKER_FLAGS -lc++)

add_executable(cppbor cppbor/main cppbor/cppbor)
