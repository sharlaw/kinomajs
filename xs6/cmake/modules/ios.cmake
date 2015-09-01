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
include (CMakeForceCompiler)

MACRO(FIND_TOOL VAR TOOL)
		FIND_PROGRAM(XCRUN xcrun)
		IF(XCRUN)
			EXECUTE_PROCESS(COMMAND ${XCRUN} -f ${TOOL} OUTPUT_VARIABLE TOOL_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
			IF(TOOL_PATH)
				SET(${VAR} ${TOOL_PATH} CACHE PATH "Path to a program")
			ELSE()
				FIND_PROGRAM(${VAR} ${TOOL})
			ENDIF()
		ENDIF()
ENDMACRO()

SET(CMAKE_SYSTEM_NAME Darwin)
SET(CMAKE_SYSTEM_VERSION 1)
FIND_TOOL(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
IF(CMAKE_UNAME)
	EXEC_PROGRAM(uname ARGS -r OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_VERSION)
	STRING(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\1" DARWIN_MAJOR_VERSION "${CMAKE_HOST_SYSTEM_VERSION}")
ENDIF()
SET(UNIX TRUE)
SET(APPLE TRUE)
SET(IOS TRUE)

set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")

FIND_TOOL(CMAKE_INSTALL_NAME_TOOL install_name_tool)
FIND_TOOL(CMAKE_AR libtool)
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -static -o <TARGET> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -static -o <TARGET> <OBJECTS>")
FIND_TOOL(CMAKE_C_COMPILER clang)
FIND_TOOL(CMAKE_CXX_COMPILER clang++)
SET(CMAKE_CXX_COMPILER_WORKS TRUE)
SET(CMAKE_C_COMPILER_WORKS TRUE)

SET(SDKROOT /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk)

SET(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "Force unset of the deployment target for iOS" FORCE)
SET(CMAKE_OSX_SYSROOT ${SDKROOT} CACHE PATH "Sysroot used for iOS support")
SET(CMAKE_FIND_ROOT_PATH ${SDKROOT} ${CMAKE_PREFIX_PATH} CACHE string  "iOS find search path root")
SET(CMAKE_FIND_FRAMEWORK FIRST)

SET(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
# SET(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} -c -x assembler-with-cpp -arch armv7s -arch armv7 -MMD -DSUPPORT_NEON_IOS=1 -o <OBJECT> <SOURCE>")
SET(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} -c -x assembler-with-cpp -arch armv7 -MMD -DSUPPORT_NEON_IOS=1 -o <OBJECT> <SOURCE>")
SET(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
SET(CMAKE_ASM_COMPILER_ID AppleClang)
# SET(CMAKE_OSX_ARCHITECTURES armv7 armv7s arm64 CACHE STRING "Build architecture of iOS")
SET(CMAKE_OSX_ARCHITECTURES armv7 arm64 CACHE STRING "Build architecture of iOS")
SET(CODESIGN_ALLOCATE /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/codesign_allocate)
SET(CMAKE_FRAMEWORK_PATH ${SDKROOT}/System/Library/Frameworks)
SET(VERSION_MIN -miphoneos-version-min=6.0)
