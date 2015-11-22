cmake_minimum_required(VERSION 2.6)
project(sf_common CXX)
# to install pkgconfig setup file.
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

#set(CMAKE_INSTALL_PREFIX "$ENV{DATAFS}")
set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED dlog libxml-2.0)
add_definitions(${rpkgs_CFLAGS})

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")
add_definitions(-DLOCALEDIR="$ENV{DATAFS}/share/locale")
add_definitions(-DFACTORYFS="$ENV{FACTORYFS}")
add_definitions(-DDATAFS="$ENV{DATAFS}")
add_definitions(-Wall -fPIC -std=gnu++0x)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-DX1_PROF)
#add_definitions(-D_GETTEXT)
#add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-D_DEFAULT_FONT_NAME="Vera")
#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
#add_definitions(-Wl,--as-needed -Wl,-O1)
#add_definitions(-finstrument-functions)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

add_library(sensord-server SHARED
	crw_lock.cpp
	worker_thread.cpp
	cconfig.cpp
	csensor_config.cpp
	cvirtual_sensor_config.cpp
	csensor_event_queue.cpp
	csensor_event_dispatcher.cpp
	csensor_usage.cpp
	cclient_info_manager.cpp
	cclient_sensor_record.cpp
	cinterval_info_list.cpp
	sensor_plugin_loader.cpp
	sensor_hal.cpp
	sensor_base.cpp
	physical_sensor.cpp
	virtual_sensor.cpp
	sensor_fusion.cpp
)

add_library(sensord-share SHARED
	cpacket.cpp
	csocket.cpp
	cbase_lock.cpp
	cmutex.cpp
	common.cpp
	sensor_info.cpp
)

target_link_libraries(sensord-server ${rpkgs_LDFLAGS} "-lrt -ldl -pthread" "sensord-share")
target_link_libraries(sensord-share ${rpkgs_LDFLAGS} "-lrt -ldl -pthread")
configure_file(sensord-server.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/sensord-server.pc @ONLY)
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)

install(TARGETS sensord-server DESTINATION lib)
install(TARGETS sensord-share DESTINATION lib)
install(FILES sensord-server.pc DESTINATION lib/pkgconfig)
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(FILES
	sensor_fusion.h
	crw_lock.h
	worker_thread.h
	cconfig.h
	csensor_config.h
	cvirtual_sensor_config.h
	csensor_event_queue.h
	cinterval_info_list.h
	sensor_plugin_loader.h
	sensor_hal.h
	sensor_base.h
	physical_sensor.h
	virtual_sensor.h
	sensor_fusion.h
	sf_common.h
	cpacket.h
	csocket.h
	cbase_lock.h
	cmutex.h
	common.h
	sensor_info.h
	DESTINATION include/${PROJECT_NAME}
)

install(FILES
	sensor_common.h
	DESTINATION include/sensor
)

