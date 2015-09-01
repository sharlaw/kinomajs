#
#      Copyright (C) 2010-2015 Marvell International Ltd.
#      Copyright (C) 2002-2010 Kinoma, Inc.
# 
#      Licensed under the Apache License, Version 2.0 (the "License");
#      you may not use this file except in compliance with the License.
#      You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#      Unless required by applicable law or agreed to in writing, software
#      distributed under the License is distributed on an "AS IS" BASIS,
#      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#      See the License for the specific language governing permissions and
#      limitations under the License.
#
# Simple Copy tool
MACRO(COPY)
	SET(oneValueArgs SOURCE DESTINATION)
	CMAKE_PARSE_ARGUMENTS(LOCAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	GET_FILENAME_COMPONENT(SOURCE_NAME ${LOCAL_SOURCE} NAME)
	GET_FILENAME_COMPONENT(DESTIANTION_NAME ${LOCAL_DESTINATION} NAME)
	GET_FILENAME_COMPONENT(DIRECTORY ${LOCAL_DESTINATION} DIRECTORY)

	FILE(COPY ${LOCAL_SOURCE} DESTINATION ${DIRECTORY})
	IF(NOT ${SOURCE_NAME} STREQUAL ${DESTIANTION_NAME})
		FILE(RENAME ${DIRECTORY}/${SOURCE_NAME} ${DIRECTORY}/${DESTIANTION_NAME})
	ENDIF()

endmacro()

# Finds a library or framework and appends to the list of variables to link agains
#
# LIBNAME: the name of the library or framework to find
# LIST: the list to append the library too once found
MACRO(LOCAL_FIND_LIBRARY)
	SET(onevalueArgs LIBNAME LIST OPTION)
	SET(options OPTIONAL EXIT)
	CMAKE_PARSE_ARGUMENTS(LOCAL "${options}" "${onevalueArgs}" "${multiValueArgs}" ${ARGN})

	STRING(TOUPPER ${LOCAL_LIBNAME} LIB_NAME)
	FIND_LIBRARY(${LIB_NAME} ${LOCAL_LIBNAME} ${LOCAL_OPTION})
	IF(${${LIB_NAME}} STREQUAL ${LIB_NAME}-NOTFOUND)
	 	IF(NOT ${LOCAL_EXIT})
			LOCAL_FIND_LIBRARY(LIBNAME ${LOCAL_LIBNAME} LIST ${LOCAL_LIST} OPTION NO_CMAKE_FIND_ROOT_PATH EXIT true)
	 	ELSE()
	 		MESSAGE(FATAL_ERROR ": ${LOCAL_LIBNAME} not found")
	 	ENDIF()
	ELSE()
		IF(APPLE)
			GET_FILENAME_COMPONENT(EXTENSION ${${LIB_NAME}} EXT)
			IF(${EXTENSION} STREQUAL ".framework")
				MARK_AS_ADVANCED(${LIB_NAME})
			ENDIF()
		ENDIF()
		LIST(APPEND ${LOCAL_LIST} ${${LIB_NAME}})
	ENDIF()
ENDMACRO()

# Gets the current running platform
#
# VARIABLE: Output variable for platform name
MACRO(GET_PLATFORM VARIABLE)
	IF(UNIX)
		IF(APPLE)
			SET(${VARIABLE} "mac")
		ELSE()
			EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
			SET(${VARIABLE} "linux/${ARCHITECTURE}")
		endif ()
	ELSEIF(WIN32 OR CYGWIN)
		SET(${VARIABLE} "win")
	ENDIF()
ENDMACRO()
