CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-common-use)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VISIBILITY} -fvisibility=hidden")

SET(COMMON-USE-LIB "email-common-use")
SET(COMMON-USE-SRCS 
	${CMAKE_SOURCE_DIR}/email-common-use/email-convert.c
	${CMAKE_SOURCE_DIR}/email-common-use/email-utilities.c
	${CMAKE_SOURCE_DIR}/email-common-use/tpl.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/email-daemon/include
	${CMAKE_SOURCE_DIR}/email-common-use/include/ 
	${CMAKE_SOURCE_DIR}/email-core/include/
	${CMAKE_SOURCE_DIR}/email-core/email-storage/include/
)

INCLUDE(FindPkgConfig)
pkg_check_modules(common_pkgs REQUIRED glib-2.0 dlog dbus-1 vconf uw-imap-toolkit security-server libtzplatform-config)

#pkg_check_modules(common_pkgs REQUIRED glib-2.0 dlog dbus-1 vconf contacts-service2 uw-imap-toolkit security-server libtzplatform-config)

FOREACH(flag ${common_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
#message(">>>[${COMMON-USE-LIB}] extra_cflags ${EXTRA_CFLAGS}")
#message(">>>[${COMMON-USE-LIB}] common_pkgs_cflags ${common_pkgs_CFLAGS}")
#message(">>>[${COMMON-USE-LIB}] result ${CMAKE_C_FLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${COMMON-USE-LIB} SHARED ${COMMON-USE-SRCS})
TARGET_LINK_LIBRARIES(${COMMON-USE-LIB} ${common_pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(${COMMON-USE-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${COMMON-USE-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${COMMON-USE-LIB} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)

