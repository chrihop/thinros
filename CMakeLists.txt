cmake_minimum_required(VERSION 3.13)

project(thinros)
set(CMAKE_C_STANDARD  11)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
find_package(KernelHeaders REQUIRED)

set(DRIVER  thinros.ko)
if (CROSS_COMPILE)
	set(KBUILD_CMD $(MAKE) -C ${KDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR} modules)
else()
	set(KBUILD_CMD $(MAKE) -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR} modules)
endif()

# create KBuild script
file(
	WRITE
	${CMAKE_CURRENT_SOURCE_DIR}/KBuild
	"obj-m += thinros.o\n"
	"ccflags-y := \n"
	"ldflags-y := \n"
)

# create Makefile script (newer linux)
file(
	WRITE
	${CMAKE_CURRENT_SOURCE_DIR}/Makefile
	"obj-m += thinros.o\n"
	"ccflags-y := \n"
	"ldflags-y := \n"
)

add_custom_command(OUTPUT ${DRIVER}
	COMMAND ${KBUILD_CMD}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS thinros.c
	VERBATIM
)

# only for clion to search the symbols
add_library(thinros_objs
	thinros.c
)

# only for clion to search the symbols
target_include_directories(thinros_objs PUBLIC
	${KINC}
)

add_custom_target(driver DEPENDS ${DRIVER})

add_custom_command(OUTPUT "/proc/thinros"
	COMMAND sudo insmod ${DRIVER}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${DRIVER}
	VERBATIM
	)

add_custom_target(install_driver DEPENDS "/proc/thinros")

add_custom_command(OUTPUT "thinros.ko.uninstall.stamp"
	COMMAND sudo modprobe -r thinros && touch "thinros.ko.uninstall.stamp"
	)

add_custom_target(uninstall_driver DEPENDS "thinros.ko.uninstall.stamp")

set(LIB_THINROS
	lib/thinros_core.c
	lib/thinros_cfg.c
	lib/thinros_linux.c)

add_library(thinros STATIC
	${LIB_THINROS})

target_compile_options(
	thinros PUBLIC
	"-DSTD_LIBC"
)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
	target_compile_options(
		thinros PUBLIC
		-O0 -g -gdb
	)
endif()

add_custom_command(TARGET thinros
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/libthinros.a ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a)

foreach(src ${LIB_THINROS})
	list(APPEND LIB_THINROS_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()

add_custom_command(OUTPUT thinros.so
	COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -DSTD_LIBC -fPIC -shared -o thinros.so ${LIB_THINROS_SRC}
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/thinros.so ${CMAKE_SOURCE_DIR}/ros/src/proxy/thinros.so
	)

add_custom_target(pythinros DEPENDS thinros.so)

add_executable(thinros_test
	lib/thinros_test.c
	)

target_link_libraries(thinros_test thinros)

target_link_options(thinros_test
	PUBLIC -rdynamic)

add_executable(thinros_app
	thinros_app.c
	)

target_link_libraries(thinros_app thinros)

target_link_options(thinros_app
	PUBLIC -rdynamic)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
	target_compile_options(
		thinros_app PUBLIC
		-O0 -g -gdb
	)
endif()

add_executable(thinros_cpp_app
	thinros_app_cpp.cpp
	)

target_link_libraries(thinros_cpp_app thinros)

target_link_options(thinros_cpp_app
	PUBLIC -rdynamic)


add_custom_target(lib DEPENDS thinros)
add_custom_target(test DEPENDS thinros_test)
add_custom_target(app DEPENDS thinros_app)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a
	DEPENDS thinros
	COMMENT "link libthinros.a with ros"
	VERBATIM
)

add_custom_target(link-ros
	DEPENDS ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a
)

find_file(ROS_SETUP setup.bash
	HINTS /opt/ros/noetic
	HINTS /opt/ros/melodic)

if (ROS_SETUP)

set (SOURCE_ROS "source ${ROS_SETUP}")

else ()

set (SOURCE_ROS "true")
message(STATUS "ros is not found. proxy might be not available.")

endif()

set (ROS_CATKIN_WS  ${CMAKE_SOURCE_DIR}/ros)

add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/proxy.build.stamp
	DEPENDS link-ros
	COMMAND bash -c "${SOURCE_ROS} && catkin_make"
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/proxy.build.stamp
	BYPRODUCTS ${ROS_CATKIN_WS}/build
			   ${ROS_CATKIN_WS}/devel
	WORKING_DIRECTORY ${ROS_CATKIN_WS}
	COMMENT "build proxy node ..."
	VERBATIM
)

add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/proxy.run.stamp
	COMMAND bash -c "source ${ROS_CATKIN_WS}/devel/setup.bash && rosrun proxy proxy_node"
	WORKING_DIRECTORY ${ROS_CATKIN_WS}
	DEPENDS ${CMAKE_BINARY_DIR}/proxy.build.stamp
	VERBATIM
)

add_custom_target(proxy-build
	DEPENDS ${CMAKE_BINARY_DIR}/proxy.build.stamp
)

add_custom_target(proxy
	DEPENDS ${CMAKE_BINARY_DIR}/proxy.run.stamp)
