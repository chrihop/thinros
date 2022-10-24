cmake_minimum_required(VERSION 3.13)

if (NOT TARGET_ARCH)
execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE TARGET_ARCH
)
string(STRIP ${TARGET_ARCH} TARGET_ARCH)

message(STATUS "`TARGET_ARCH` not set, defaulting to host architecture ${TARGET_ARCH}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(KernelHeaders REQUIRED)

set(DRIVER thinros)

if (CROSS_COMPILE)
    set(KBUILD_CMD $(MAKE) -C ${KDIR} ARCH=${TARGET_ARCH} CROSS_COMPILE=${CROSS_COMPILE} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})
else ()
    set(KBUILD_CMD $(MAKE) -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})
endif ()

set(DRIVER_SRC
    )

foreach(SRC ${DRIVER_SRC})
    get_filename_component(SRC_NAME ${SRC} NAME_WE)
    list(APPEND DRIVER_OBJ ${SRC_NAME}.o)
endforeach()

string (REPLACE ";" " " DRIVER_OBJ "${DRIVER_OBJ}")

file (
    WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Makefile
    "obj-m := ${DRIVER}.o" "\n"
    "${DRIVER}-objs += ${DRIVER_OBJ}" "\n"
    "ccflags-y += -D_LINUX_KERNEL_ -I${CMAKE_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}" "\n"
    "ldflags-y += " "\n"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko
    COMMAND ${KBUILD_CMD} modules
    DEPENDS ${DRIVER_SRC}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#    COMMENTS "Building ${DRIVER}.ko"
    VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko.clean
    COMMAND ${KBUILD_CMD} clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)

add_custom_target(
    ${DRIVER}-driver
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko
)

add_custom_target(
    ${DRIVER}-driver-clean
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko.clean
)

add_dependencies(
    extra-clean
    ${DRIVER}-driver-clean
)

# only for symbol search
add_library(
    ${DRIVER}.objs
    STATIC EXCLUDE_FROM_ALL
    ${DRIVER}.c
    ${DRIVER_SRC}
)

target_include_directories(
    ${DRIVER}.objs
    PRIVATE
    ${KINC}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(
    ${DRIVER}.objs
    PRIVATE
    _LINUX_KERNEL_
)
## --

# install / remove driver
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/install.tstamp
    COMMAND sudo insmod ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER}.ko
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Installing ${DRIVER}.ko"
    VERBATIM
)

add_custom_target(
    install-${DRIVER}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/install.tstamp
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/remove.tstamp
    COMMAND sudo rmmod ${DRIVER}
    DEPENDS "/proc/thinros"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing ${DRIVER}.ko"
    VERBATIM
)

add_custom_target(
    remove-${DRIVER}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/remove.tstamp
)
