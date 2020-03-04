################################################################################
# Set up CMake.                                                                #
################################################################################
cmake_minimum_required(VERSION 3.13)

################################################################################
# Set up compilation of the app.                                               #
################################################################################
add_subdirectory(upvtc_ct)

add_executable(upvtc_ct "")
target_sources(upvtc_ct
    PRIVATE
        main.cpp
)
target_link_libraries(upvtc_ct
    upvtc_ct-lib
    ${CONAN_LIBS}
)
