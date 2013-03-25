CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-ipc)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(IPC-LIB "email-ipc")
SET(IPC-SRCS
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-api.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-build.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-proxy.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-stub.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-api/email-ipc-api-info.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-api/email-ipc-param.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-api/email-ipc-param-list.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-proxy/email-proxy-callback-info.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-proxy/email-proxy-main.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-proxy/email-proxy-socket.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-socket/email-ipc-socket.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-stub/email-stub-main.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-stub/email-stub-socket.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-stub/email-stub-task.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-stub/email-stub-task-manager.c
	${CMAKE_SOURCE_DIR}/email-ipc/email-activation/email-dbus-activation.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/email-daemon/include
	${CMAKE_SOURCE_DIR}/email-ipc/include
	${CMAKE_SOURCE_DIR}/email-ipc/email-proxy/include
	${CMAKE_SOURCE_DIR}/email-ipc/email-stub/include
	${CMAKE_SOURCE_DIR}/email-ipc/email-socket/include
	${CMAKE_SOURCE_DIR}/email-ipc/email-ipc-api/include
	${CMAKE_SOURCE_DIR}/email-ipc/email-activation/include
	${CMAKE_SOURCE_DIR}/email-common-use/include
	${CMAKE_SOURCE_DIR}/email-api/include
	${CMAKE_SOURCE_DIR}/email-core/email-storage/include/
	${CMAKE_SOURCE_DIR}/email-core/include/
)

INCLUDE(FindPkgConfig)
pkg_check_modules(ipc_pkgs REQUIRED dlog dbus-1 glib-2.0 libsystemd-daemon)

FOREACH(flag ${ipc_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_DEFINITIONS("-DBRSDK_VER=\"2\"")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

MESSAGE("Before DBUS binding")

FIND_PROGRAM(DBUS_BINDING_TOOL NAMES dbus-binding-tool)
EXEC_PROGRAM("${DBUS_BINDING_TOOL}" ARGS "--prefix=email_service ${CMAKE_CURRENT_SOURCE_DIR}/email-activation/email-service.xml --mode=glib-client --output=${CMAKE_CURRENT_SOURCE_DIR}/email-activation/include/email-service-glue.h")
EXEC_PROGRAM("${DBUS_BINDING_TOOL}" ARGS "--prefix=email_service ${CMAKE_CURRENT_SOURCE_DIR}/email-activation/email-service.xml --mode=glib-server --output=${CMAKE_CURRENT_SOURCE_DIR}/email-activation/include/email-service-binding.h")

MESSAGE("After DBUS binding")

ADD_LIBRARY(${IPC-LIB} SHARED ${IPC-SRCS})
TARGET_LINK_LIBRARIES(${IPC-LIB} ${ipc_pkgs_LDFLAGS} email-common-use pthread)
SET_TARGET_PROPERTIES(${IPC-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${IPC-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/email-activation/email-service.service DESTINATION share/dbus-1/services)
INSTALL(TARGETS ${IPC-LIB} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)


