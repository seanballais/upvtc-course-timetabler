################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)

################################################################################
# Set up compilation of the upvtc_ct module.                                   #
################################################################################
add_subdirectory(ds)
add_subdirectory(preprocessor)

add_library(upvtc_ct-lib INTERFACE)
target_link_libraries(upvtc_ct-lib
    INTERFACE
        upvtc_ct-lib-ds
        upvtc_ct-lib-preprocessor
)
