CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(email-base)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

MESSAGE("")
MESSAGE(">>> current directory: ${CMAKE_CURRENT_SOURCE_DIR}")
MESSAGE(">>> Build type: ${CMAKE_BUILD_TYPE}")

SET(VISIBILITY "-DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\"")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VISIBILITY} -fvisibility=hidden")

SET(BASE-LIB "email-base")
SET(BASE-SRCS 
	${CMAKE_SOURCE_DIR}/Common/Msg_Convert.c
)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/Common/include/ 
	${CMAKE_SOURCE_DIR}/email-core/include/
	${CMAKE_SOURCE_DIR}/email-core/em-storage/include/
)

INCLUDE(FindPkgConfig)
pkg_check_modules(base_pkgs REQUIRED glib-2.0 dlog contacts-service) 

FOREACH(flag ${base_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
#message(">>>[${BASE-LIB}] extra_cflags ${EXTRA_CFLAGS}")
#message(">>>[${BASE-LIB}] base_pkgs_cflags ${base_pkgs_CFLAGS}")
#message(">>>[${BASE-LIB}] result ${CMAKE_C_FLAGS}")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

ADD_LIBRARY(${BASE-LIB} SHARED ${BASE-SRCS})
TARGET_LINK_LIBRARIES(${BASE-LIB} ${base_pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(${BASE-LIB} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${BASE-LIB} PROPERTIES VERSION ${VERSION})

#INSTALL(TARGETS ${BASE-LIB} LIBRARY DESTINATION lib)
INSTALL(TARGETS ${BASE-LIB} DESTINATION lib COMPONENT RuntimeLibraries)

