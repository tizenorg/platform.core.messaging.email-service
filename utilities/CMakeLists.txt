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
# Define Test App
##########################################################


SET(TEST-APP-SRCS
	${CMAKE_SOURCE_DIR}/utilities/test-application/main.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-utility.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-mail.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-mailbox.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-account.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-rule.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-thread.c
	${CMAKE_SOURCE_DIR}/utilities/test-application/testapp-others.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/utilities/test-application/include
	${CMAKE_SOURCE_DIR}/email-api/include
	${CMAKE_SOURCE_DIR}/email-common-use/include
	${CMAKE_SOURCE_DIR}/email-core/include
	${CMAKE_SOURCE_DIR}/email-core/email-storage/include
	${CMAKE_SOURCE_DIR}/email-ipc/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(test_app_pkgs REQUIRED glib-2.0 gthread-2.0 vconf db-util uw-imap-toolkit contacts-service drm-client)

FOREACH(flag ${test_app_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_EXECUTABLE(email-test-app ${TEST-APP-SRCS})
TARGET_LINK_LIBRARIES(email-test-app ${test_app_pkgs_LDFLAGS} email-api)

INSTALL(TARGETS email-test-app DESTINATION bin)

