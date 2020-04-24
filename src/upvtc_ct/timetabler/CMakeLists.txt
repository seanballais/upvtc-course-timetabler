################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)

################################################################################
# Set up compilation of the timetabler module.                                 #
################################################################################
add_library(upvtc_ct-lib-timetabler STATIC "")
target_sources(upvtc_ct-lib-timetabler
    PRIVATE
        timetabler.cpp
)

target_link_libraries(upvtc_ct-lib-timetabler
    xlnt
)
