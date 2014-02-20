CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-service)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)
SET(VERSION_MAJOR 1)
SET(VERSION "${VERSION_MAJOR}.1.0")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")


##########################################################
# Define BASE LIB
##########################################################
ADD_SUBDIRECTORY(email-common-use)

#########################################################
# Define IPC
##########################################################
ADD_SUBDIRECTORY(email-ipc)

##########################################################
# Define STORAGE, NETWORK, CORE lib
##########################################################

ADD_SUBDIRECTORY(email-core)

##########################################################
# Define Email API   
##########################################################

ADD_SUBDIRECTORY(email-api)

##########################################################
# Define Execute File
##########################################################

ADD_SUBDIRECTORY(email-daemon)

##########################################################
# Define Test App
##########################################################

ADD_SUBDIRECTORY(utilities)

##########################################################
# Define Install Files
##########################################################

CONFIGURE_FILE(email-service.pc.in email-service.pc @ONLY)

INSTALL(FILES ${CMAKE_BINARY_DIR}/email-service.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

# LICENSE file
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE.BSD DESTINATION /usr/share/license RENAME email-service)

INSTALL(FILES ${CMAKE_SOURCE_DIR}/email-service.rule DESTINATION /opt/etc/smack/accesses.d/)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/res/ DESTINATION /opt/usr/data/email/res/image FILES_MATCHING PATTERN "*.png")
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/res/ DESTINATION /opt/usr/data/email/res FILES_MATCHING PATTERN "*.sql")
