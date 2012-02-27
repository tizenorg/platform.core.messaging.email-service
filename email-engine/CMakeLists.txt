CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-service)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(VISIBILITY "-DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VISIBILITY} -fvisibility=hidden")

##########################################################
# Define Execute File
##########################################################

SET(MAIN-SRCS 
	${CMAKE_SOURCE_DIR}/email-engine/main.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-init.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-account.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-global.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-mail.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-mailbox.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-etc.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-emn-noti.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-emn.c
	${CMAKE_SOURCE_DIR}/email-engine/emf-auto-poll.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/Common/include
	${CMAKE_SOURCE_DIR}/ipc/include
	${CMAKE_SOURCE_DIR}/MAPI/include
	${CMAKE_SOURCE_DIR}/email-core/drivers
	${CMAKE_SOURCE_DIR}/email-core/em-storage/include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/email-core/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(main_pkgs REQUIRED glib-2.0 drm-service dlog dbus-1 gthread-2.0  db-util openssl uw-imap-toolkit vconf contacts-service alarm-service mm-player devman_haptic)


FOREACH(flag ${main_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_EXECUTABLE(${PROJECT_NAME} ${MAIN-SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${main_pkgs_LDFLAGS} dl ssl email-core email-storage email-network email-ipc email-base)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

