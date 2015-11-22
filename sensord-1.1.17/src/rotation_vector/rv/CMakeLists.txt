cmake_minimum_required(VERSION 2.6)
project(rv CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

SET(SENSOR_NAME rv_sensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)
include_directories(${CMAKE_SOURCE_DIR}/src/sensor_fusion)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED vconf)
add_definitions(${rpkgs_CFLAGS} -DUSE_ONLY_ONE_MODULE -DUSE_LCD_TYPE_CHECK)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "0")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)


FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ELSE("${ARCH}" MATCHES "^arm.*")
	ADD_DEFINITIONS("-DSIMULATOR")
	MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" MATCHES "^arm.*")

add_definitions(-Wall -O3 -omit-frame-pointer)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-Iinclude)

add_library(${SENSOR_NAME} SHARED
		rv_sensor.cpp
)

target_link_libraries(${SENSOR_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
