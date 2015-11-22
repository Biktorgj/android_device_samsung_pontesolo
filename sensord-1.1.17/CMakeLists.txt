cmake_minimum_required(VERSION 2.6)
project(sensord_main CXX)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED glib-2.0 gio-2.0 vconf dlog libxml-2.0 libsystemd-daemon)
add_definitions(${rpkgs_CFLAGS})

# to install pkgconfig setup file.
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions(-Wall -O3 -omit-frame-pointer)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-Wall -g -D_DEBUG)

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DTARGET")
        MESSAGE("add -DTARGET")
ELSE("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DSIMULATOR")
        MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" MATCHES "^arm.*")

CONFIGURE_FILE(sensor_plugins.xml.in sensor_plugins.xml @ONLY)
CONFIGURE_FILE(sensors.xml.in sensors.xml @ONLY)
CONFIGURE_FILE(virtual_sensors.xml.in virtual_sensors.xml @ONLY)
install(FILES
	sensor_plugins.xml
	sensors.xml
	virtual_sensors.xml
	DESTINATION etc)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME sensord)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME libsensord)

INSTALL(FILES sensord.efl DESTINATION /etc/smack/accesses.d)

add_subdirectory(src)
