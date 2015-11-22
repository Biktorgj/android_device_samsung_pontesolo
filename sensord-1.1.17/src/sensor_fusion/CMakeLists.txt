cmake_minimum_required(VERSION 2.6)
project(sensor_fusion CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(VERSION 1.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED vconf)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "0")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
	ADD_DEFINITIONS("-DTARGET -DHWREV_CHECK")
	MESSAGE("add -DTARGET -DHWREV_CHECK")
ELSE("${ARCH}" MATCHES "^arm.*")
	ADD_DEFINITIONS("-DSIMULATOR")
	MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" MATCHES "^arm.*")

add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-DUSE_DLOG_LOG)
add_definitions(-Iinclude)

SET(SENSOR_FUSION_NAME sensor_fusion)

add_library(${SENSOR_FUSION_NAME} SHARED
		euler_angles.cpp
		matrix.cpp
		orientation_filter.cpp
		quaternion.cpp
		rotation_matrix.cpp
		sensor_data.cpp
		vector.cpp
		)

target_link_libraries(${SENSOR_FUSION_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_FUSION_NAME} DESTINATION lib/sensord)
