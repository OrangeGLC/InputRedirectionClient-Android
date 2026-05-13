cmake_minimum_required(VERSION 3.15)
project(transmitter_unit_tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Find CppUTest (Homebrew installation) ----
find_path(CPPUTEST_INCLUDE_DIR
    NAMES CppUTest/TestHarness.h
    PATHS /usr/local/opt/cpputest /opt/homebrew/opt/cpputest
          /usr/local /opt/homebrew
    PATH_SUFFIXES include
)
find_library(CPPUTEST_LIBRARY
    NAMES CppUTest
    PATHS /usr/local/opt/cpputest /opt/homebrew/opt/cpputest
          /usr/local /opt/homebrew
    PATH_SUFFIXES lib
)

if(NOT CPPUTEST_INCLUDE_DIR OR NOT CPPUTEST_LIBRARY)
    message(FATAL_ERROR "CppUTest not found. Install with: brew install cpputest")
endif()

# ---- Mock headers come FIRST (shadow Android NDK/JNI) ----
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# ---- Production source headers ----
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# ---- Define UNIT_TESTING for friend class ----
add_definitions(-DUNIT_TESTING)

# ---- Test executable ----
add_executable(transmitter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../Transmitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../cJSON.c
    main.cpp
    TestTransmitterSetters.cpp
    TestTransmitterConfig.cpp
    TestTransmitterKeyMapping.cpp
    TestTransmitterFrame.cpp
    TestTransmitterIO.cpp
)

target_include_directories(transmitter_tests PRIVATE ${CPPUTEST_INCLUDE_DIR})
target_link_libraries(transmitter_tests ${CPPUTEST_LIBRARY} m)
