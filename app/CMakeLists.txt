cmake_minimum_required(VERSION 3.13)

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
