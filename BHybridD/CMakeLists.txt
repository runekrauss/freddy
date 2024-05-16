cmake_minimum_required(VERSION 3.22)
project(BHybridD)

set(CMAKE_CXX_STANDARD 20)

add_executable(BHybridD hybridTest.cpp)

add_subdirectory(../../eddy WORKING_DIRECTORY)
target_link_libraries(BHybridD eddy)
