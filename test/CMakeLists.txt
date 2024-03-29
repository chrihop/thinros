cmake_minimum_required(VERSION 3.13)

if (BAREMETAL)
    return()
endif()

message(STATUS "Building with the Linux")

#include(FetchContent)
#FetchContent_Declare(
#    googletest
#    GIT_REPOSITORY https://github.com/google/googletest.git
#    GIT_TAG release-1.12.1
#)
## For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)
#
#enable_testing()

add_executable(thinros_test
    thinros_test.c
    )

target_link_libraries(thinros_test
    PRIVATE
    thinros)

target_link_options(thinros_test
    PRIVATE -rdynamic)

add_executable(thinros_app
    thinros_app.c
    )

target_link_libraries(thinros_app
    PRIVATE
    thinros)

target_link_options(thinros_app
    PRIVATE -rdynamic)

add_executable(thinros_app_udp
    thinros_app_udp.c
    )

target_link_libraries(thinros_app_udp
    PRIVATE
    thinros)

target_link_options(thinros_app_udp
    PRIVATE -rdynamic)


add_executable(thinros_udp_bridge_helper
    thinros_udp_bridge_helper.c
    )

target_link_libraries(thinros_udp_bridge_helper
    PRIVATE
    thinros)

target_link_options(thinros_udp_bridge_helper
    PRIVATE -rdynamic)

add_executable(thinros_cpp_app
    thinros_app_cpp.cc
    )

target_link_libraries(thinros_cpp_app
    PRIVATE thinros)

target_link_options(thinros_cpp_app
    PRIVATE -rdynamic)

add_executable(thinros_app_helper
    thinros_app_helper.cc
    )

target_link_libraries(thinros_app_helper
    PRIVATE thinros)

target_link_options(thinros_app_helper
    PRIVATE -rdynamic)
