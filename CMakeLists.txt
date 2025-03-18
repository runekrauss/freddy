cmake_minimum_required(VERSION 3.25)
project(freddy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(freddy INTERFACE)
target_include_directories(freddy INTERFACE include)

find_package(Boost QUIET NO_MODULE)
if(NOT Boost_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        boost_unordered  # so that Boost does not have to be fully downloaded and to speed up the compile time
        GIT_REPOSITORY https://github.com/MikePopoloski/boost_unordered
        GIT_TAG v1.87.0
        GIT_SHALLOW 1
        SYSTEM
    )
    FetchContent_MakeAvailable(boost_unordered)

    target_link_libraries(freddy INTERFACE boost_unordered)
endif()

find_package(lorina QUIET NO_MODULE)
if(NOT lorina_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            lorina  # so that Boost does not have to be fully downloaded and to speed up the compile time
            GIT_REPOSITORY https://github.com/hriener/lorina
            GIT_TAG v0.2
            GIT_SHALLOW 1
            SYSTEM
    )
    FetchContent_MakeAvailable(lorina)

    target_link_libraries(freddy INTERFACE cli11 lorina)
endif()

option(CLANGTIDY "Check code")
if(CLANGTIDY)
    find_program(CLANG_TIDY clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -extra-arg=-Wno-unknown-warning-option -fix-errors ${CMAKE_CXX_CLANG_TIDY})
endif()

option(FREDDY_TEST "Build tests")
if(FREDDY_TEST)
    find_package(Catch2 QUIET)
    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
            GIT_SHALLOW 1
            SYSTEM
        )
        FetchContent_MakeAvailable(Catch2)

        set_target_properties(Catch2 PROPERTIES CXX_CLANG_TIDY "")
        set_target_properties(Catch2WithMain PROPERTIES CXX_CLANG_TIDY "")
    endif()

    if(NOT WIN32)
        set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    endif()

    include(CTest)
    include(Catch)
    message(STATUS "Building tests")
    file(GLOB FILENAMES test/**/*.cpp)
    foreach(FILE IN LISTS FILENAMES)
        get_filename_component(DIR_PATH ${FILE} PATH)
        get_filename_component(DIR_NAME ${DIR_PATH} NAME)
        get_filename_component(FILE_NAME ${FILE} NAME_WE)
        set(TEST_NAME ${DIR_NAME}_${FILE_NAME})
        add_executable(${TEST_NAME} ${FILE})
        target_link_libraries(${TEST_NAME} PRIVATE Catch2::Catch2WithMain freddy)
        target_link_libraries(${TEST_NAME} PUBLIC cli11 lorina)
        #add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        catch_discover_tests(${TEST_NAME})
    endforeach()
endif()
