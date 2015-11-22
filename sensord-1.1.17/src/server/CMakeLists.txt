cmake_minimum_required(VERSION 2.6)
project(sensord CXX)

#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
#add_definitions(-Wall -g -pg)
add_definitions(-Wall -std=gnu++0x)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt -ldl -pthread -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

SET(SERVER_SRCS
	dbus_util.cpp
	server.cpp
	command_worker.cpp
	permission_checker.cpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${SERVER_SRCS})

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} "sensord-server")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
