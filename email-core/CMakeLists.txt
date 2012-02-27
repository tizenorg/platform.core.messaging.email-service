CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-storage)

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
# Define STORAGE lib
##########################################################

SET(STORAGE-LIB "email-storage")
SET(STORAGE-SRCS 
	./em-storage/em-storage-noti.c
	./em-storage/em-storage.c
	${CMAKE_SOURCE_DIR}/emf-emn-storage/emf-emn-storage.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_ROOT_DIR}/include/common/include
	${CMAKE_ROOT_DIR}/include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/em-storage/include
)


INCLUDE(FindPkgConfig)
pkg_check_modules(storage_pkgs REQUIRED drm-service glib-2.0 dlog db-util openssl vconf secure-storage dbus-1 contacts-service)

set(EXTRA_CFLAGS "")
FOREACH(flag ${storage_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

#message(">>> extra_cflags ${EXTRA_CFLAGS}")
#message(">>> result ${CMAKE_C_FLAGS}")


SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${STORAGE-LIB} SHARED ${STORAGE-SRCS})
TARGET_LINK_LIBRARIES(${STORAGE-LIB} ${storage_LDFLAGS} ${BASE-LIB} ss-client)
SET_TARGET_PROPERTIES(${STORAGE-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${STORAGE-LIB} PROPERTIES VERSION ${VERSION})

#INSTALL(TARGETS ${STORAGE-LIB} LIBRARY DESTINATION lib)
INSTALL(TARGETS ${STORAGE-LIB} DESTINATION lib COMPONENT RuntimeLibraries)


##########################################################
# Define network lib                                                  
##########################################################

SET(NETWORK-LIB "email-network")
SET(NETWORK-SRCS 
	./em-network/em-network.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include/common/include
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/email-core/include
	${CMAKE_SOURCE_DIR}/email-core/em-storage/include
)

INCLUDE(FindPkgConfig)
pkg_check_modules(network_pkgs REQUIRED glib-2.0 uw-imap-toolkit dbus-1 vconf dlog openssl)

FOREACH(flag ${network_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

#message(">> net extra raw: ${core_proxy_pkgs_CFLAGS}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${NETWORK-LIB} SHARED ${NETWORK-SRCS})
TARGET_LINK_LIBRARIES(${NETWORK-LIB} ${network_pkgs_LDFLAGS} ${BASE-LIB} ${STORAGE-LIB} dl)
SET_TARGET_PROPERTIES(${NETWORK-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${NETWORK-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${NETWORK-LIB} DESTINATION lib COMPONENT RuntimeLibraries)

##########################################################
# Define Core lib
##########################################################

SET(CORE-LIB "email-core")
SET(CORE-SRCS 
	${CMAKE_SOURCE_DIR}/email-core/em-core-event.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-imap-mailbox.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-mailbox.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-mailbox-sync.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-mesg.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-mm-callbacks.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-utils.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-api.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-smtp.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-timer.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-imap-idle.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-global.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-account.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-mime.c
	${CMAKE_SOURCE_DIR}/email-core/em-core-sound.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include/common/include
	${CMAKE_SOURCE_DIR}/include/
	${CMAKE_SOURCE_DIR}/email-core/em-storage/include
	${CMAKE_SOURCE_DIR}/email-core/include
	${CMAKE_SOURCE_DIR}/Common/include
	${CMAKE_SOURCE_DIR}/email-core/em-network/include
)


INCLUDE(FindPkgConfig)
pkg_check_modules(core_pkgs REQUIRED glib-2.0 drm-service dlog dbus-1 gthread-2.0 uw-imap-toolkit vconf contacts-service secure-storage openssl accounts-svc mm-player mm-session devman_haptic alarm-service notification libcurl libxml-2.0)

set(EXTRA_CFLAGS "")
FOREACH(flag ${core_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

#message(">> core extra raw: ${core_pkgs_CFLAGS}")
#message(">> core extra : ${EXTRA_CFLAGS}")
#message(">> core result: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DCREATEPROTO=unixproto")
ADD_DEFINITIONS("-DEMPTYPROTO=unixproto")
ADD_DEFINITIONS("-DMAILSPOOL=\"/var/spool/mail\"")
ADD_DEFINITIONS("-DANONYMOUSHOME=\"/var/spool/mail/anonymous\"")
ADD_DEFINITIONS("-DACTIVEFILE=\"/var/lib/news/active\"")
ADD_DEFINITIONS("-DNEWSSPOOL=\"/var/spool/news\"")
ADD_DEFINITIONS("-DRSHPATH=\"/usr/bin/rsh\"")
ADD_DEFINITIONS("-DMD5ENABLE=\"/etc/cram-md5.pwd\"")
ADD_DEFINITIONS("-DSSL_CERT_DIRECTORY=\"/opt/etc/ssl/certs\"")
ADD_DEFINITIONS("-DSSL_KEY_DIRECTORY=\"/opt/etc/ssl/certs\"")
ADD_DEFINITIONS("-DCHUNKSIZE=65536")
ADD_DEFINITIONS("-DLOCKPGM=\"\"")
ADD_DEFINITIONS("-DLOCKPGM1=\"/usr/libexec/mlock\"")
ADD_DEFINITIONS("-DLOCKPGM2=\"/usr/sbin/mlock\"")
ADD_DEFINITIONS("-DLOCKPGM3=\"/usr/etc/mlock\"")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${CORE-LIB} SHARED ${CORE-SRCS})

TARGET_LINK_LIBRARIES(${CORE-LIB} ${core_pkgs_LDFLAGS} ${STORAGE-LIB} ${NETWORK-LIB})

SET_TARGET_PROPERTIES(${CORE-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${CORE-LIB} PROPERTIES VERSION ${VERSION})

INSTALL(TARGETS ${CORE-LIB} DESTINATION lib COMPONENT RuntimeLibraries)


