cmake_minimum_required(VERSION 2.6)
project(bio_hrm CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

SET(SENSOR_PHY_NAME bio_hrm_phy_sensor)
SET(SENSOR_VIRT_NAME bio_hrm_virt_sensor)
SET(SENSOR_HAL_NAME bio_hrm_sensor_hal)

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

add_library(${SENSOR_PHY_NAME} SHARED
	${SENSOR_PHY_NAME}.cpp
)

add_library(${SENSOR_VIRT_NAME} SHARED
	hrm_alg.cpp
	hrm_alg_maxim.cpp
	hrm_alg_adi.cpp
	${SENSOR_VIRT_NAME}.cpp
)

add_library(${SENSOR_HAL_NAME} SHARED
	${SENSOR_HAL_NAME}.cpp
)

target_link_libraries(${SENSOR_PHY_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_VIRT_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS})

install(TARGETS ${SENSOR_PHY_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_VIRT_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
