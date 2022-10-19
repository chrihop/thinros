# Find the kernel release

if (NOT KDIR)

execute_process(
	COMMAND uname -r
	OUTPUT_VARIABLE KERNEL_RELEASE
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find the headers
find_path(KDIR
	include/linux/user.h
	PATHS /lib/modules/${KERNEL_RELEASE}/build
	)

message(STATUS "Kernel release: ${KERNEL_RELEASE}")

endif()

message(STATUS "Kernel headers: ${KDIR}")

if (KDIR)
	set(KINC
		${KDIR}/include
		${KDIR}/arch/${TARGET_ARCH}/include
		CACHE PATH "Kernel headers include dirs"
		)
	set(KDIR_FOUND 1 CACHE STRING "Set to 1 if kernel headers were found")
else ()
	set(KDIR_FOUND 0 CACHE STRING "Set to 1 if kernel headers were found")
endif ()

mark_as_advanced(KDIR_FOUND)
