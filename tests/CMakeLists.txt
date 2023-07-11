cmake_minimum_required(VERSION 3.13)

message(STATUS "Building tests for linux user processes")

include_directories(
    ${THINROS_SRC_DIR}/lib
    ${THINROS_GEN_INC})

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

add_executable(thinros_dos_demo_adv_controller
    thinros_dos_demo_adv_controller.c
    )

target_link_options(
    thinros_dos_demo_adv_controller
    PRIVATE -rdynamic)

target_link_libraries(
    thinros_dos_demo_adv_controller
    PRIVATE thinros)

if (THINROS_UNIT_TEST)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
    enable_testing()

    include_directories(
        ${googletest_SOURCE_DIR}/googletest/include
        ${googletest_SOURCE_DIR}/googlemock/include)

    link_libraries(
        gtest_main
        thinros)

    add_link_options(
        -rdynamic)

    add_executable(unit_thinros_object
        unit_thinros_object.cc)

#    gtest_discover_tests(unit_thinros_object)
endif ()
