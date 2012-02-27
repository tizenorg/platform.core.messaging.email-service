CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-ipc)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(VISIBILITY "-DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VISIBILITY} -fvisibility=hidden")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VISIBILITY} -fvisibility=hidden")

SET(IPC-LIB "email-ipc")
SET(IPC-SRCS 
	${CMAKE_SOURCE_DIR}/ipc/ipc-library-api.cpp 
	${CMAKE_SOURCE_DIR}/ipc/ipc-library-build.cpp
	${CMAKE_SOURCE_DIR}/ipc/ipc-library-proxy.cpp
	${CMAKE_SOURCE_DIR}/ipc/ipc-library-stub.cpp
	${CMAKE_SOURCE_DIR}/ipc/api/ipc-api-info.cpp
	${CMAKE_SOURCE_DIR}/ipc/api/ipc-param.cpp
	${CMAKE_SOURCE_DIR}/ipc/api/ipc-param-list.cpp
	${CMAKE_SOURCE_DIR}/ipc/common/cm-list.cpp
	${CMAKE_SOURCE_DIR}/ipc/common/cm-sys-msg-queue.cpp
	${CMAKE_SOURCE_DIR}/ipc/proxy/ipc-callback-info.cpp
	${CMAKE_SOURCE_DIR}/ipc/proxy/ipc-proxy-main.cpp
	${CMAKE_SOURCE_DIR}/ipc/proxy/ipc-proxy-socket.cpp
	${CMAKE_SOURCE_DIR}/ipc/socket/ipc-socket.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-queue.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-response-info.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-stub-main.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-stub-socket.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-task.cpp
	${CMAKE_SOURCE_DIR}/ipc/stub/ipc-task-manager.cpp
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/ipc/include
	${CMAKE_SOURCE_DIR}/ipc/proxy/include
	${CMAKE_SOURCE_DIR}/ipc/stub/include
	${CMAKE_SOURCE_DIR}/ipc/common/include
	${CMAKE_SOURCE_DIR}/ipc/socket/include
	${CMAKE_SOURCE_DIR}/ipc/api/include
	${CMAKE_SOURCE_DIR}/Common/include
	${CMAKE_SOURCE_DIR}/MAPI/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(ipc_pkgs REQUIRED dlog glib-2.0)

FOREACH(flag ${ipc_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
#message("***cmake_c_flags ${CMAKE_C_FLAGS}")
#message("***extra_cflags ${EXTRA_CFLAGS}")
#message("***base_pkgs_cflags ${base_pkgs_CFLAGS}")
#message("***result ${CMAKE_CXX_FLAGS}")


ADD_DEFINITIONS("-DBRSDK_VER=\"2\"")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${IPC-LIB} SHARED ${IPC-SRCS})
TARGET_LINK_LIBRARIES(${IPC-LIB} ${ipc_pkgs_LDFLAGS} email-base pthread)
SET_TARGET_PROPERTIES(${IPC-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${IPC-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${IPC-LIB} DESTINATION lib COMPONENT RuntimeLibraries)

