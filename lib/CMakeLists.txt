cmake_minimum_required(VERSION 3.13)

set (THINROS_LIB_DIR "${THINROS_SRC_DIR}/lib")

if (BUILD_FOR STREQUAL "linux-user")
    set (EXTRA_SRC
        "${THINROS_LIB_DIR}/thinros_linux.c")
else()
    set (EXTRA_SRC "")
endif()

add_custom_command(
    OUTPUT ${THINROS_GEN_DIR}/thinros_cfg.c
           ${THINROS_GEN_DIR}/thinros_cfg.h
           ${THINROS_OBJ_DIR}/thinros.gen.tstamp
    COMMAND python3 ${THINROS_TOOLS_DIR}/network-generator.py
        --platform "${THINROS_PLATFORM_JSON}"
        --network "${THINROS_NETWORK_JSON}"
        --out_dir "${THINROS_GEN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch ${THINROS_OBJ_DIR}/thinros.gen.tstamp
    DEPENDS ${THINROS_NETWORK_JSON}
            ${THINROS_PLATFORM_JSON}
            ${THINROS_TOOLS_DIR}/network-generator.py
            ${THINROS_TOOLS_DIR}/templates/thinros_cfg.jinja2.c
            ${THINROS_TOOLS_DIR}/templates/thinros_cfg.jinja2.h
    WORKING_DIRECTORY ${THINROS_OBJ_DIR}
)

add_custom_target(
    thinros_gen
    DEPENDS ${THINROS_OBJ_DIR}/thinros.gen.tstamp)

set (THINROS_LIB_SRC
    "${THINROS_LIB_DIR}/thinros_core.c"
    "${THINROS_LIB_DIR}/thinros_object.c"
    ${THINROS_GEN_SRC}
    ${EXTRA_SRC})

include_directories(
    "${THINROS_LIB_DIR}"
    "${THINROS_GEN_INC}")

add_library(thinros STATIC ${THINROS_LIB_SRC})
add_dependencies(thinros thinros_gen)

if (BUILD_FOR STREQUAL "linux-user")
    add_library(thinros_shared SHARED ${THINROS_LIB_SRC})
    target_link_options(
        thinros_shared
        PRIVATE
        -fPIC -shared -ldl)
    target_compile_options(
        thinros_shared
        PRIVATE
        -fno-sanitize=all)
    target_link_options(
        thinros
        PRIVATE
        -fno-sanitize=all)
    add_custom_command(
        TARGET thinros_shared
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:thinros_shared>
        ${THINROS_SRC_DIR}/pythinros/libthinros.so)
endif()
