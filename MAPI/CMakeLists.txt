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
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Account.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Message.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Network.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Rule.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Mailbox.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Types.c
	${CMAKE_SOURCE_DIR}/MAPI/Emf_Mapi_Init.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/ipc/include
	${CMAKE_SOURCE_DIR}/MAPI/include
	${CMAKE_SOURCE_DIR}/email-core/em-storage/include
	${CMAKE_SOURCE_DIR}/ipc/socket/include
	${CMAKE_SOURCE_DIR}/Common/include
	${CMAKE_SOURCE_DIR}/email-core/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(api_pkgs REQUIRED glib-2.0 dlog dbus-1 vconf heynoti db-util contacts-service uw-imap-toolkit) 

FOREACH(flag ${api_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${API-LIB} SHARED ${API-SRCS})

TARGET_LINK_LIBRARIES(${API-LIB} ${api_pkgs_LDFLAGS} email-ipc email-storage email-core email-base email-network)
#${NETWORK-LIB} ${CORE-LIB} ${STORAGE-LIB} ${IPC-LIB} ${BASE-LIB})

SET_TARGET_PROPERTIES(${API-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${API-LIB} PROPERTIES VERSION ${VERSION})

#INSTALL(TARGETS ${API-LIB} LIBRARY DESTINATION lib)
INSTALL(TARGETS ${API-LIB} DESTINATION lib COMPONENT RuntimeLibraries)

SET(MAPI-HEADERS
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Account.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Message.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Mailbox.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Rule.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Init.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Types.h
	${CMAKE_SOURCE_DIR}/MAPI/include/Emf_Mapi_Network.h
	${CMAKE_SOURCE_DIR}/include/emf-types.h 
)

INSTALL(FILES ${MAPI-HEADERS} DESTINATION include/email-service)

