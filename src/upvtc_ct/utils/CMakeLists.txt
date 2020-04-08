################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)

################################################################################
# Set up compilation of the utils module.                                      #
################################################################################
add_library(upvtc_ct-lib-utils STATIC "")
target_sources(upvtc_ct-lib-utils
    PRIVATE
        data_manager.cpp
        errors.cpp
        utils.cpp
)
