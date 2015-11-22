cmake_minimum_required(VERSION 2.6)
project(motion)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

SET(SENSOR_NAME motion_sensor)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED vconf)
add_definitions(${rpkgs_CFLAGS} -DUSE_ONLY_ONE_MODULE)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

ADD_DEFINITIONS("-DTARGET -DHWREV_CHECK")

add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-Wall -g3 -O2 -rdynamic -fno-omit-frame-pointer)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-D_ANDROID_SHOWLOG)
add_definitions(-DMM_MOTION3D_PROJECT)
add_definitions(-Iinclude)
add_definitions(-Wall -std=gnu++0x)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

add_library(${SENSOR_NAME} SHARED
	motion_sensor.cpp
)

target_link_libraries(${SENSOR_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
