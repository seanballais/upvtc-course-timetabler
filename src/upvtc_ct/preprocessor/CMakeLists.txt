################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)

################################################################################
# Set up compilation of the preprocessor module.                               #
################################################################################
add_library(upvtc_ct-lib-preprocessor STATIC "")
target_sources(upvtc_ct-lib-preprocessor
    PRIVATE
        data_manager.cpp
)
target_link_libraries(upvtc_ct-lib-preprocessor
    upvtc_ct-lib-ds
)