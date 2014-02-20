CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-api)

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
# Define Email API   
##########################################################

SET(API-LIB "email-api")
SET(API-SRCS 
	${CMAKE_SOURCE_DIR}/email-api/email-api-account.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-mail.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-network.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-rule.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-mailbox.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-init.c
	${CMAKE_SOURCE_DIR}/email-api/email-api-etc.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/email-daemon/include
	${CMAKE_SOURCE_DIR}/email-api/include
	${CMAKE_SOURCE_DIR}/email-api/socket/include
	${CMAKE_SOURCE_DIR}/email-ipc/include
	${CMAKE_SOURCE_DIR}/email-core/include
	${CMAKE_SOURCE_DIR}/email-core/email-storage/include
	${CMAKE_SOURCE_DIR}/email-common-use/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(api_pkgs REQUIRED glib-2.0 dlog dbus-1 vconf db-util contacts-service2 uw-imap-toolkit) 

FOREACH(flag ${api_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${API-LIB} SHARED ${API-SRCS})

TARGET_LINK_LIBRARIES(${API-LIB} ${api_pkgs_LDFLAGS} email-ipc email-storage email-core email-common-use email-network)
#${NETWORK-LIB} ${CORE-LIB} ${STORAGE-LIB} ${IPC-LIB} ${COMMON-LIB})

SET_TARGET_PROPERTIES(${API-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${API-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${API-LIB} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)

SET(EMAIL-API-HEADERS
	${CMAKE_SOURCE_DIR}/email-api/include/email-api.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-account.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-init.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-mail.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-mailbox.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-network.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-rule.h
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-etc.h
	${CMAKE_SOURCE_DIR}/email-common-use/include/email-types.h
	${CMAKE_SOURCE_DIR}/email-common-use/include/email-errors.h
)

INSTALL(FILES ${EMAIL-API-HEADERS} DESTINATION include/email-service)

##########################################################
# Define Email SMIME API   
##########################################################

SET(SMIME-API-LIB "email-smime-api")
SET(SMIME-API-SRCS 
	${CMAKE_SOURCE_DIR}/email-api/email-api-smime.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/email-api/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(smime_api_pkgs REQUIRED glib-2.0 dlog) 

FOREACH(flag ${smime_api_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${SMIME-API-LIB} SHARED ${SMIME-API-SRCS})

TARGET_LINK_LIBRARIES(${SMIME-API-LIB} ${api_pkgs_LDFLAGS} email-ipc email-storage email-smime)

SET_TARGET_PROPERTIES(${SMIME-API-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${SMIME-API-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${SMIME-API-LIB} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)

SET(SMIME-EMAIL-API-HEADERS
	${CMAKE_SOURCE_DIR}/email-api/include/email-api-smime.h
)

INSTALL(FILES ${SMIME-EMAIL-API-HEADERS} DESTINATION include/email-service)

