#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(XSR)

INCLUDE_DIRECTORIES(${XS6}/includes)
INCLUDE_DIRECTORIES(${XS6}/sources)
INCLUDE_DIRECTORIES(${XS6}/sources/tool)

IF(WIN32)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /c /D _CONSOLE /D WIN32 /D _CRT_SECURE_NO_DEPRECATE /D mxParse=1 /D mxRun=1 /nologo /Zp1")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /D _DEBUG /D mxDebug /Od /Z7")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /D NDEBUG /O2")
	SET(LINK_OPTIONS ws2_32.lib comctl32.lib)
ELSEIF(UNIX)
	IF(APPLE)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -mmacosx-version-min=10.7")
		FIND_LIBRARY(CORE_SERVICES CoreServices)
		SET(LINK_OPTIONS ${CORE_SERVICES})
	ELSE()
		SET(LINK_OPTIONS "-rdynamic -lm -ldl")
	ENDIF()
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -DmxParse -DmxRun=1")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DmxDebug=1 -g -O0 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
ENDIF()

SET(CMAKE_XCODE_ADDRITBUTE_SYMROOT ${CMAKE_BINARY_DIR})

LIST(APPEND SOURCES ${XS6}/sources/tool/xs6Host.c)
LIST(APPEND SOURCES ${XS6}/sources/tool/xs6Platform.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6API.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6All.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Array.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Boolean.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Code.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Common.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6DataView.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Date.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Debug.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Error.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Function.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Generator.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Global.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6JSON.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Lexical.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6MapSet.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Marshall.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Math.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Memory.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Module.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Number.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Object.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Profile.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Promise.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Property.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Proxy.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6RegExp.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Run.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Scope.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Script.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6SourceMap.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6String.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Symbol.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Syntaxical.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Tree.c)
LIST(APPEND SOURCES ${XS6}/sources/xs6Type.c)
LIST(APPEND SOURCES ${XS6}/sources/xs_dtoa.c)
LIST(APPEND SOURCES ${XS6}/sources/xs_pcre.c)
LIST(APPEND SOURCES ${XS6}/sources/xsr6.c)

ADD_EXECUTABLE(xsr ${SOURCES})
IF(LINK_OPTIONS)
	TARGET_LINK_LIBRARIES(xsr ${LINK_OPTIONS})
ENDIF()
SET_TARGET_PROPERTIES(xsr PROPERTIES OUTPUT_NAME xsr6)
ADD_CUSTOM_COMMAND(
	TARGET xsr
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xsr> ${XS6_TOOLS_BIN}
	)