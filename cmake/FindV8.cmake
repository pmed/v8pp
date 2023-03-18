# Based on https://raw.githubusercontent.com/nicehash/cpp-ethereum/master/cmake/Findv8.cmake
#
# Find the v8 includes and library
#
# This module defines
#  V8_INCLUDE_DIRS, where to find header, etc.
#  V8_LIBRARIES, the libraries needed to use v8.
#  V8_FOUND, If false, do not try to use v8.

if(WIN32)
	set(ARCH ${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE})
	if(NOT ARCH)
		if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
			set(ARCH x64)
		else()
			set(ARCH x86)
		endif()
	endif()
	message(STATUS "v8-v${MSVC_TOOLSET_VERSION}-${ARCH}")
	file(GLOB V8_NUGET_DEV_DIR LIST_DIRECTORIES TRUE
		${CMAKE_BINARY_DIR}/v8-v${MSVC_TOOLSET_VERSION}-${ARCH}.*)
	string(REPLACE "v8-" "v8.redist-" V8_NUGET_BIN_DIR ${V8_NUGET_DEV_DIR})

	set(V8_NUGET_INCLUDE_DIR ${V8_NUGET_DEV_DIR}/include)
	set(V8_NUGET_LIB_DIR ${V8_NUGET_DEV_DIR}/lib/${CMAKE_BUILD_TYPE})
	set(V8_NUGET_BIN_DIR ${V8_NUGET_BIN_DIR}/lib/${CMAKE_BUILD_TYPE})
	find_file(V8_DLL v8.dll ${V8_NUGET_BIN_DIR})
	find_file(V8_LIBPLATFORMDLL v8_libplatform.dll ${V8_NUGET_BIN_DIR})

	file(GLOB V8_BINARIES ${V8_NUGET_BIN_DIR}/*)
	message(STATUS "Copy ${V8_BINARIES} to the project output dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
	file(COPY ${V8_BINARIES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
endif()

find_path(V8_INCLUDE_DIRS PATHS ${V8_NUGET_INCLUDE_DIR} PATH_SUFFIXES v8 NAMES v8.h)
find_library(V8_LIB PATHS ${V8_NUGET_LIB_DIR} NAMES v8 v8.dll.lib)
find_library(V8_LIBPLATFORM PATHS ${V8_NUGET_LIB_DIR} NAMES v8_libplatform v8_libplatform.dll.lib)

# handle the QUIETLY and REQUIRED arguments and set V8_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_INCLUDE_DIRS V8_LIB V8_LIBPLATFORM)
mark_as_advanced(V8_INCLUDE_DIRS V8_LIB)

if(V8_FOUND)
	add_library(V8::v8 SHARED IMPORTED)
	add_library(V8::libplatform SHARED IMPORTED)
    if(WIN32)
		set_target_properties(V8::v8 PROPERTIES
			IMPORTED_IMPLIB ${V8_LIB} IMPORTED_LOCATION ${V8_DLL})
		set_target_properties(V8::libplatform PROPERTIES
			IMPORTED_IMPLIB ${V8_LIBPLATFORM} IMPORTED_LOCATION ${V8_LIBPLATFORMDLL})
    else()
		set_target_properties(V8::v8 PROPERTIES IMPORTED_LOCATION ${V8_LIB})
		set_target_properties(V8::libplatform PROPERTIES IMPORTED_LOCATION ${V8_LIBPLATFORM})
	endif()
	target_include_directories(V8::v8 INTERFACE ${V8_INCLUDE_DIRS})
	target_include_directories(V8::libplatform INTERFACE ${V8_INCLUDE_DIRS})
endif()
