################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)
project(upvtc_ct)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include CMake project modules.
include("BuildType")

# Integrate with Conan.
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup()

# Make sure we're using C++ 17 when compiling the main executaBLE.
# We need to use C++ 17 since we're using the new way of declaring namespaces
# that is only available in C++ 17 (and likely in future versions).
set(CMAKE_CXX_STANDARD 17)

################################################################################
# Set up compilation of the app.                                               #
################################################################################
include_directories("${CMAKE_SOURCE_DIR}/src")

# Note that the CMakeLists.txt file for the main application is here in this
# directory.
add_subdirectory(src)

################################################################################
# Copy data files to the output directory.                                     #
################################################################################
message(STATUS "Copying data files to the binary folder")
configure_file(
    "${CMAKE_SOURCE_DIR}/data/study_plans.json"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/study_plans.json"
    COPYONLY)
