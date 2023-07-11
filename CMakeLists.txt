cmake_minimum_required(VERSION 3.13)

project(thinros)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_custom_target(
    extra-clean
)

include(CMakeDependentOption)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set (THINROS_C_DEFINITIONS "")

# -- options --
set (BUILD_FOR "linux-user" CACHE STRING "ThinROS library build target")
set_property(CACHE BUILD_FOR PROPERTY STRINGS
    linux-user
    linux-kernel
    certikos-user
    certikos-kernel)
message (STATUS "ThinROS build for: ${BUILD_FOR}")

option(THINROS_BUILD_LINUX_DRIVER "ThinROS build linux driver" OFF)
message (STATUS "ThinROS build linux driver: ${BUILD_LINUX_DRIVER}")

option(THINROS_UNIT_TEST "ThinROS unit test" OFF)
message (STATUS "ThinROS unit test: ${THINROS_UNIT_TEST}")

function(find_available_jsons DIR PREFIX OUTPUT)
    file(GLOB JSON_FILES "${DIR}/${PREFIX}-*.json")
    if (NOT JSON_FILES)
        message(FATAL_ERROR "No config files found for ${PREFIX}!")
    endif()
    set (JSON_NAMES)
    foreach (JSON ${JSON_FILES})
        get_filename_component(JSON_NAME ${JSON} NAME_WE)
        string(REGEX REPLACE "${PREFIX}-" "" JSON_NAME ${JSON_NAME})
        list(APPEND JSON_NAMES ${JSON_NAME})
    endforeach()
    set(${OUTPUT} ${JSON_NAMES} PARENT_SCOPE)
endfunction()

function(select_json DIR PREFIX VAR SUPPORTED JSON)
    file(GLOB ${JSON} "${DIR}/${PREFIX}-${VAR}.json")
    if (NOT ${JSON})
        string(REPLACE ";" ", " SUPPORTED_MSGS "${${SUPPORTED}}")
        message(FATAL_ERROR "Unsupported ${PREFIX}: ${VAR}.
        Supported ${PREFIX}: ${SUPPORTED_MSGS}")
    else()
        message(STATUS "ThinROS selected ${PREFIX}: ${VAR} (${${JSON}})")
        mark_as_advanced(FORCE ${VAR})
        set (${JSON} "${${JSON}}" PARENT_SCOPE)
    endif()
endfunction()

function(to_c_def VAR OUTPUT)
    string(TOUPPER ${VAR} VAR_UPPER)
    string(REGEX REPLACE "-" "_" VAR_UPPER ${VAR_UPPER})
    set (${OUTPUT} ${VAR_UPPER} PARENT_SCOPE)
endfunction()

find_available_jsons("config" "platform" SUPPORTED_PLATFORMS)
set(THINROS_PLATFORM "rpi4b" CACHE STRING "ThinROS execution platform")
set_property(CACHE THINROS_PLATFORM PROPERTY STRINGS ${SUPPORTED_PLATFORMS})
select_json("config" "platform" ${THINROS_PLATFORM} SUPPORTED_PLATFORMS THINROS_PLATFORM_JSON)

find_available_jsons("config" "network" SUPPORTED_NETWORKS)
set(THINROS_NETWORK "dos-demo" CACHE STRING "ThinROS network configuration")
set_property(CACHE THINROS_NETWORK PROPERTY STRINGS ${SUPPORTED_NETWORKS})
select_json("config" "network" ${THINROS_NETWORK} SUPPORTED_NETWORKS THINROS_NETWORK_JSON)
# -- #

set (THINROS_C_DEFINITIONS "" NOCACHE)

# -- build for --
if (BUILD_FOR STREQUAL "linux-user")
    list (APPEND THINROS_C_DEFINITIONS
        _STD_LIBC_
    )
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -ggdb -O0)
        add_compile_options(-fsanitize=address,undefined -fno-sanitize=alignment)
        add_link_options(-fsanitize=address,undefined -fno-sanitize=alignment)
    else ()
        add_compile_options(-O2)
    endif()
elseif (BUILD_FOR STREQUAL "certikos-user")
    list (APPEND THINROS_C_DEFINITIONS
        _CERTIKOS_USER_
    )
elseif (BUILD_FOR STREQUAL "certikos-kernel")
    list (APPEND THINROS_C_DEFINITIONS
        _CERTIKOS_KERNEL_
    )
else()
    message(FATAL_ERROR "Unsupported build target: ${BUILD_FOR}")
endif ()
# -- #

# -- platform & network --
set (THINROS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (THINROS_OBJ_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set (THINROS_CFG_DIR "${THINROS_SRC_DIR}/config")
set (THINROS_TOOLS_DIR "${THINROS_SRC_DIR}/tools")

set (THINROS_GEN_DIR "${THINROS_OBJ_DIR}/gen")
if (NOT EXISTS ${THINROS_GEN_DIR})
    file(MAKE_DIRECTORY ${THINROS_GEN_DIR})
endif()

cmake_dependent_option(THINROS_PLATFORM_MODIFIED
    "Advanced option: force regeneration of ThinROS configuration files" ON
    "NOT THINROS_PLATFORM STREQUAL THINROS_PLATFORM_PREV" OFF)
set (THINROS_PLATFORM_PREV "${THINROS_PLATFORM}" CACHE INTERNAL
    "previous built platform value of THINROS_PLATFORM")

cmake_dependent_option(THINROS_NETWORK_MODIFIED
    "Advanced option: force regeneration of ThinROS configuration files" ON
    "NOT THINROS_NETWORK STREQUAL THINROS_NETWORK_PREV" OFF)
set (THINROS_NETWORK_PREV "${THINROS_NETWORK}" CACHE INTERNAL
    "previous built network value of THINROS_NETWORK")

if (NOT EXISTS ${THINROS_OBJ_DIR}/thinros.gen.tstamp
    OR THINROS_PLATFORM_MODIFIED
    OR THINROS_NETWORK_MODIFIED)
    message(STATUS "Generating ThinROS configuration files")
    execute_process(
        COMMAND python3 ${THINROS_TOOLS_DIR}/topic_gen.py
        --platform "${THINROS_PLATFORM_JSON}"
        --network "${THINROS_NETWORK_JSON}"
        --out_dir "${THINROS_GEN_DIR}"
        WORKING_DIRECTORY ${THINROS_SRC_DIR}
    )
    file(TOUCH ${THINROS_OBJ_DIR}/thinros.gen.tstamp)
endif()

set (THINROS_GEN_SRC
    "${THINROS_GEN_DIR}/thinros_cfg.c")

set (THINROS_GEN_INC
    "${THINROS_GEN_DIR}")

to_c_def(${THINROS_PLATFORM} THINROS_PLATFORM_C_DEF)
list(APPEND THINROS_C_DEFINITIONS
    _THINROS_PLATFORM_${THINROS_PLATFORM_C_DEF}_)

to_c_def(${THINROS_NETWORK} THINROS_NETWORK_C_DEF)
list(APPEND THINROS_C_DEFINITIONS
    _THINROS_NETWORK_${THINROS_NETWORK_C_DEF}_)
# -- #

# -- draw the network --
add_custom_command(
    OUTPUT ${THINROS_OBJ_DIR}/thinros.network.draw.tstamp
    COMMAND python3 ${THINROS_TOOLS_DIR}/network-generator.py
        --platform "${THINROS_PLATFORM_JSON}"
        --network "${THINROS_NETWORK_JSON}"
        --out_dir "${THINROS_OBJ_DIR}"
        --draw
    WORKING_DIRECTORY ${THINROS_SRC_DIR}
    DEPENDS ${THINROS_PLATFORM_JSON} ${THINROS_NETWORK_JSON}
    COMMENT "Drawing ThinROS network"
)

add_custom_target(
    draw
    DEPENDS ${THINROS_OBJ_DIR}/thinros.network.draw.tstamp
)
# -- #

# -- unit tests options --
if (THINROS_UNIT_TEST)
    list (APPEND THINROS_C_DEFINITIONS
        _THINROS_UNIT_TESTS_
    )
endif()
# -- #


add_compile_definitions(${THINROS_C_DEFINITIONS})

# -- build library --
add_subdirectory(lib)
# -- #

# -- build tests --
if (BUILD_FOR STREQUAL "linux-user")
    add_subdirectory(tests)
endif()
# --

# -- build driver --
if (THINROS_BUILD_LINUX_DRIVER)
    add_subdirectory(drv)
endif()

# -- build ROS proxy --
if (THINROS_BUILD_ROS_PROXY)
    add_subdirectory(app)
endif()

# -- link with certikos --
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set (THINROS_INCLUDE_DIR
        ${THINROS_SRC_DIR}/lib
        ${THINROS_GEN_INC}
        PARENT_SCOPE)

    set (THINROS_C_DEFINITIONS
        ${THINROS_C_DEFINITIONS}
        PARENT_SCOPE)
endif()
# -- #
