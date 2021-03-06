cmake_minimum_required(VERSION 2.6)
project(sensord_src CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

include_directories(${CMAKE_SOURCE_DIR}/src/shared)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED glib-2.0 gio-2.0 vconf)
add_definitions(${rpkgs_CFLAGS} -DUSE_ONLY_ONE_MODULE -DUSE_LCD_TYPE_CHECK)
add_definitions(-Wall -std=gnu++0x)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "0")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(SENSOR_FUSION_ENABLE "0")
set(ORIENTATION_ENABLE "0")
set(GRAVITY_ENABLE "0")
set(LINEAR_ACCELERATION_ENABLE "0")

IF("${ACCEL}" STREQUAL "ON")
add_subdirectory(accel)
ENDIF()
IF("${GYRO}" STREQUAL "ON")
add_subdirectory(gyro)
ENDIF()
IF("${UNCAL_GYRO}" STREQUAL "ON")
add_subdirectory(uncal_gyro)
ENDIF()
IF("${PROXI}" STREQUAL "ON")
add_subdirectory(proxi)
ENDIF()
IF("${LIGHT}" STREQUAL "ON")
add_subdirectory(light)
ENDIF()
IF("${GEO}" STREQUAL "ON")
add_subdirectory(geo)
ENDIF()
IF("${UNCAL_GEO}" STREQUAL "ON")
add_subdirectory(uncal_geo)
ENDIF()
IF("${PEDO}" STREQUAL "ON")
add_subdirectory(pedo)
ENDIF()
IF("${CONTEXT}" STREQUAL "ON")
add_subdirectory(context)
ENDIF()
IF("${FLAT}" STREQUAL "ON")
add_subdirectory(flat)
ENDIF()
IF("${BIO}" STREQUAL "ON")
add_subdirectory(bio)
ENDIF()
IF("${BIO_HRM}" STREQUAL "ON")
add_subdirectory(bio_hrm)
ENDIF()
IF("${AUTO_ROTATION}" STREQUAL "ON")
add_subdirectory(auto_rotation)
ENDIF()
IF("${MOTION}" STREQUAL "ON")
add_subdirectory(motion)
ENDIF()
IF("${PRESSURE}" STREQUAL "ON")
add_subdirectory(pressure)
ENDIF()
IF("${PIR}" STREQUAL "ON")
add_subdirectory(pir)
ENDIF()
IF("${PIR_LONG}" STREQUAL "ON")
add_subdirectory(pir_long)
ENDIF()
IF("${TEMPERATURE}" STREQUAL "ON")
add_subdirectory(temperature)
ENDIF()
IF("${HUMIDITY}" STREQUAL "ON")
add_subdirectory(humidity)
ENDIF()
IF("${ULTRAVIOLET}" STREQUAL "ON")
add_subdirectory(ultraviolet)
ENDIF()
IF("${DUST}" STREQUAL "ON")
add_subdirectory(dust)
ENDIF()
IF("${ORIENTATION}" STREQUAL "ON")
set(SENSOR_FUSION_ENABLE "1")
set(ORIENTATION_ENABLE "1")
ENDIF()
IF("${GRAVITY}" STREQUAL "ON")
set(SENSOR_FUSION_ENABLE "1")
set(ORIENTATION_ENABLE "1")
set(GRAVITY_ENABLE "1")
ENDIF()
IF("${LINEAR_ACCEL}" STREQUAL "ON")
set(SENSOR_FUSION_ENABLE "1")
set(ORIENTATION_ENABLE "1")
set(GRAVITY_ENABLE "1")
set(LINEAR_ACCELERATION_ENABLE "1")
ENDIF()
IF("${SENSOR_FUSION_ENABLE}" STREQUAL "1")
add_subdirectory(sensor_fusion)
ENDIF()
IF("${ORIENTATION_ENABLE}" STREQUAL "1")
add_subdirectory(orientation)
ENDIF()
IF("${GRAVITY_ENABLE}" STREQUAL "1")
add_subdirectory(gravity)
ENDIF()
IF("${LINEAR_ACCELERATION_ENABLE}" STREQUAL "1")
add_subdirectory(linear_accel)
ENDIF()
add_subdirectory(rotation_vector)

add_subdirectory(server)
add_subdirectory(libsensord)
add_subdirectory(shared)
