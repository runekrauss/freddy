cmake_minimum_required(VERSION 3.20.5)
project(eddy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(eddy INTERFACE)
target_include_directories(eddy INTERFACE include)

option(CLANGTIDY "Check code")
if(CLANGTIDY)
    find_program(CLANG_TIDY clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -extra-arg=-Wno-unknown-warning-option -fix-errors)
endif()

option(EDDY_TEST "Build tests")
if(EDDY_TEST)
    find_package(Catch2)
    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.0.1
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    include(CTest)
    include(Catch)
    message(STATUS "Building tests")
    file(GLOB FILENAMES test/*.cpp)
    foreach(FILE IN LISTS FILENAMES)
        get_filename_component(NAME ${FILE} NAME_WE)
        set(TEST_NAME test_${NAME})
        add_executable(${TEST_NAME} ${FILE})
        target_link_libraries(${TEST_NAME} PRIVATE Catch2::Catch2WithMain eddy)
        #catch_discover_tests(${TEST_NAME})
        add_test(NAME ${NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()
