CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-service)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VISIBILITY} -fvisibility=hidden")

##########################################################
# Define Execute File
##########################################################

SET(MAIN-SRCS 
	${CMAKE_SOURCE_DIR}/email-daemon/main.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-init.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-account.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-mail.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-mailbox.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-etc.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-emn.c
	${CMAKE_SOURCE_DIR}/email-daemon/email-daemon-auto-poll.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/email-common-use/include
	${CMAKE_SOURCE_DIR}/email-ipc/include
	${CMAKE_SOURCE_DIR}/email-api/include
	${CMAKE_SOURCE_DIR}/email-core/include
	${CMAKE_SOURCE_DIR}/email-core/email-storage/include
	${CMAKE_SOURCE_DIR}/email-daemon/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(main_pkgs REQUIRED glib-2.0 dlog dbus-1 gthread-2.0  db-util openssl uw-imap-toolkit vconf contacts-service alarm-service mm-player devman_haptic heynoti)
# _TIZEN_PUBLIC_


FOREACH(flag ${main_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_EXECUTABLE(${PROJECT_NAME} ${MAIN-SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${main_pkgs_LDFLAGS} dl ssl email-core email-storage email-network email-ipc email-common-use)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

