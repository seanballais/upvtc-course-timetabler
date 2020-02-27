################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.12)
project(upvtc_ct)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include CMake project modules.
include("BuildType")

# Make sure we're using C++ 17 when compiling the main executaBLE.
# We need to use C++ 17 since we're using the new way of declaring namespaces
# that is only available in C++ 17 (and likely in future versions).
set(CMAKE_CXX_STANDARD 17)

################################################################################
# Set up compilation of the app.                                               #
################################################################################
include_directories("${CMAKE_SOURCE_DIR}/src")

set(PROJECT_MAIN "${CMAKE_SOURCE_DIR}/src/main.cpp")

add_executable(upvtc_ct ${PROJECT_MAIN})
