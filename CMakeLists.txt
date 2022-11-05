cmake_minimum_required(VERSION 3.13)

project(thinros)
set(CMAKE_C_STANDARD  11)
set(CMAKE_CXX_STANDARD 20)

add_custom_target(
	extra-clean
)

option(BUILD_FOR_SECURE_GATEWAY "Build for secure gateway" OFF)

if (BUILD_FOR_SECURE_GATEWAY)
	message(STATUS "Build for secure gateway")
	add_definitions(-DTROS_SCENARIO_SECURE_GATEWAY)
endif()

if (BUILD_FOR_SAFETY_CONTROLLER)
	message(STATUS "Build for safety controller")
	add_definitions(-DTROS_SCENARIO_SAFETY_CONTROLLER)
endif()


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_options(-g -ggdb -O0)
	if (BAREMETAL)
	else()
		add_compile_options(-fsanitize=address,undefined -fno-sanitize=alignment)
		add_link_options(-fsanitize=address,undefined -fno-sanitize=alignment)
	endif()
else()
	add_compile_options(-O2)
endif()

add_subdirectory(drv)

if (BAREMETAL)

	list(APPEND THINROS_EXTRA_SRC
		)


else()

	list(APPEND THINROS_EXTRA_SRC
		lib/thinros_linux.c)


	add_compile_definitions(
		_STD_LIBC_
	)

endif()

include_directories(${CMAKE_SOURCE_DIR})

set(LIB_THINROS_SRC
	lib/thinros_core.c
	lib/thinros_cfg.c
	${THINROS_EXTRA_SRC})

add_library(thinros STATIC
	${LIB_THINROS_SRC}
	)

if (NOT BAREMETAL)

add_library(thinros_shared SHARED
	${LIB_THINROS_SRC}
	)

target_link_options(thinros_shared
	PRIVATE
	-fPIC -shared -ldl
)

add_custom_command(
	TARGET thinros_shared
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:thinros_shared> ${CMAKE_SOURCE_DIR}/pythinros/libthinros.so
)

target_compile_options(
	thinros_shared
	PRIVATE
	-fno-sanitize=all
)

target_link_options(
	thinros_shared
	PRIVATE
	-fno-sanitize=all
)

endif()

add_subdirectory(test)

#add_custom_command(TARGET thinros
#	POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/libthinros.a ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a)
#
#foreach(src ${LIB_THINROS})
#	list(APPEND LIB_THINROS_SRC
#		${CMAKE_CURRENT_SOURCE_DIR}/${src})
#endforeach()
#
#add_custom_command(OUTPUT thinros.so
#	COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -DSTD_LIBC -fPIC -shared -o thinros.so ${LIB_THINROS_SRC}
#	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/thinros.so ${CMAKE_SOURCE_DIR}/ros/src/proxy/thinros.so
#	)
#
#add_custom_target(pythinros DEPENDS thinros.so)
#
#add_executable(thinros_test
#	lib/thinros_test.c
#	)
#
#target_link_libraries(thinros_test thinros)
#
#target_link_options(thinros_test
#	PUBLIC -rdynamic)
#
#add_executable(thinros_app
#	thinros_app.c
#	)
#
#target_link_libraries(thinros_app thinros)
#
#target_link_options(thinros_app
#	PUBLIC -rdynamic)
#
#if (CMAKE_BUILD_TYPE EQUAL "Debug")
#	target_compile_options(
#		thinros_app PUBLIC
#		-O0 -g -gdb
#	)
#endif()
#
#add_executable(thinros_cpp_app
#	thinros_app_cpp.cpp
#	)
#
#target_link_libraries(thinros_cpp_app thinros)
#
#target_link_options(thinros_cpp_app
#	PUBLIC -rdynamic)
#
#
#add_custom_target(lib DEPENDS thinros)
#add_custom_target(test DEPENDS thinros_test)
#add_custom_target(app DEPENDS thinros_app)
#
#add_custom_command(
#	OUTPUT ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a
#	DEPENDS thinros
#	COMMENT "link libthinros.a with ros"
#	VERBATIM
#)
#
#add_custom_target(link-ros
#	DEPENDS ${CMAKE_SOURCE_DIR}/ros/src/proxy/libthinros.a
#)
#
#find_file(ROS_SETUP setup.bash
#	HINTS /opt/ros/noetic
#	HINTS /opt/ros/melodic)
#
#if (ROS_SETUP)
#
#set (SOURCE_ROS "source ${ROS_SETUP}")
#
#else ()
#
#set (SOURCE_ROS "true")
#message(STATUS "ros is not found. proxy might be not available.")
#
#endif()
#
#set (ROS_CATKIN_WS  ${CMAKE_SOURCE_DIR}/ros)
#
#add_custom_command(
#	OUTPUT ${CMAKE_BINARY_DIR}/proxy.build.stamp
#	DEPENDS link-ros
#	COMMAND bash -c "${SOURCE_ROS} && catkin_make"
#	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/proxy.build.stamp
#	BYPRODUCTS ${ROS_CATKIN_WS}/build
#			   ${ROS_CATKIN_WS}/devel
#	WORKING_DIRECTORY ${ROS_CATKIN_WS}
#	COMMENT "build proxy node ..."
#	VERBATIM
#)
#
#add_custom_command(
#	OUTPUT ${CMAKE_BINARY_DIR}/proxy.run.stamp
#	COMMAND bash -c "source ${ROS_CATKIN_WS}/devel/setup.bash && rosrun proxy proxy_node"
#	WORKING_DIRECTORY ${ROS_CATKIN_WS}
#	DEPENDS ${CMAKE_BINARY_DIR}/proxy.build.stamp
#	VERBATIM
#)
#
#add_custom_target(proxy-build
#	DEPENDS ${CMAKE_BINARY_DIR}/proxy.build.stamp
#)
#
#add_custom_target(proxy
#	DEPENDS ${CMAKE_BINARY_DIR}/proxy.run.stamp)
