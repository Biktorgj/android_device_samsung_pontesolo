cmake_minimum_required(VERSION 2.6)
project(rv_raw CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

SET(SENSOR_NAME rv_raw_sensor)
SET(SENSOR_HAL_NAME rv_raw_sensor_hal)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

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
		rv_raw_sensor.cpp
)

add_library(${SENSOR_HAL_NAME} SHARED
		rv_raw_sensor_hal.cpp
)

target_link_libraries(${SENSOR_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS})

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
