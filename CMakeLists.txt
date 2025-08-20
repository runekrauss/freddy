cmake_minimum_required(VERSION 3.20) # due to C++23
project(FrEDDY LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23) # elifdef
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(Ccache ccache)
if(Ccache)
  if(MSVC)
    file(COPY_FILE ${Ccache} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)

    set(CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "TrackFileAccess=false"
      "UseMultiToolTask=true"
      "DebugInformationFormat=OldStyle"
    )
  endif()
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

add_library(freddy INTERFACE)
target_include_directories(freddy INTERFACE include)

find_package(Boost QUIET NO_MODULE)
if(NOT Boost_FOUND)
  include(FetchContent) # FetchContent_Declare

  message(STATUS "Fetching/Preparing Boost modules")

  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_INCLUDE_LIBRARIES smart_ptr unordered variant2) # dummies to not configure all Boost modules

  FetchContent_Declare(Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.tar.xz # performance reasons
    URL_HASH SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
    DOWNLOAD_EXTRACT_TIMESTAMP ON
  )
  FetchContent_MakeAvailable(Boost)

  message(STATUS "Fetching/Preparing Boost modules - done")

  target_include_directories(freddy SYSTEM INTERFACE
    ${Boost_SOURCE_DIR}/libs/smart_ptr/include
    ${Boost_SOURCE_DIR}/libs/unordered/include
    ${Boost_SOURCE_DIR}/libs/variant2/include
    # their dependencies
    ${Boost_SOURCE_DIR}/libs/assert/include
    ${Boost_SOURCE_DIR}/libs/config/include
    ${Boost_SOURCE_DIR}/libs/container_hash/include
    ${Boost_SOURCE_DIR}/libs/core/include
    ${Boost_SOURCE_DIR}/libs/describe/include
    ${Boost_SOURCE_DIR}/libs/mp11/include
    ${Boost_SOURCE_DIR}/libs/predef/include
    ${Boost_SOURCE_DIR}/libs/static_assert/include
    ${Boost_SOURCE_DIR}/libs/throw_exception/include
  )
endif()

option(FREDDY_STATS "Calculate statistics")
if(FREDDY_STATS)
  target_compile_definitions(freddy INTERFACE BOOST_UNORDERED_ENABLE_STATS)
endif()

option(CLANG_TIDY "Check code")
if(CLANG_TIDY)
  find_program(CLANGTIDY clang-tidy REQUIRED)
  set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY} -extra-arg=-Wno-unknown-warning-option ${CMAKE_CXX_CLANG_TIDY}) # -fix-errors
endif()

option(FREDDY_TEST "Build tests")
if(FREDDY_TEST)
  find_package(Catch2 QUIET)
  if(NOT Catch2_FOUND)
    include(FetchContent) # FetchContent_MakeAvailable

    message(STATUS "Fetching/Preparing Catch2")

    FetchContent_Declare(Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.9.1 # for the sake of robustness
      GIT_SHALLOW 1
      SYSTEM
    )
    FetchContent_MakeAvailable(Catch2)

    set_target_properties(Catch2 PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(Catch2WithMain PROPERTIES CXX_CLANG_TIDY "")

    message(STATUS "Fetching/Preparing Catch2 - done")
  endif()

  if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
  endif()

  include(CTest) # enable_testing() and for generating coverage information
  include(Catch) # catch_discover_tests

  file(GLOB FILENAMES test/**/*.cpp)
  foreach(FILE IN LISTS FILENAMES)
    get_filename_component(DIR_PATH ${FILE} PATH)
    get_filename_component(DIR_NAME ${DIR_PATH} NAME)
    get_filename_component(FILE_NAME ${FILE} NAME_WE)
    set(TEST_NAME ${DIR_NAME}-${FILE_NAME})
    add_executable(${TEST_NAME} ${FILE})
    target_link_libraries(${TEST_NAME} PRIVATE Catch2::Catch2WithMain freddy)
    catch_discover_tests(${TEST_NAME}) # instead of add_test for a more detailed overview
  endforeach()
endif()
