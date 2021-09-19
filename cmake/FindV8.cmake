# Based on https://raw.githubusercontent.com/nicehash/cpp-ethereum/master/cmake/Findv8.cmake
#
# Find the v8 includes and library
#
# This module defines
#  V8_INCLUDE_DIRS, where to find header, etc.
#  V8_LIBRARIES, the libraries needed to use v8.
#  V8_FOUND, If false, do not try to use v8.

find_path(V8_INCLUDE_DIRS PATH_SUFFIXES v8 NAMES v8.h)

find_library(V8_LIB NAMES v8)
find_library(V8_LIBPLATFORM NAMES v8_libplatform)
set(V8_LIBRARIES ${V8_LIB} ${V8_LIBPLATFORM})

# handle the QUIETLY and REQUIRED arguments and set V8_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_INCLUDE_DIRS V8_LIB V8_LIBPLATFORM)
mark_as_advanced(V8_INCLUDE_DIRS V8_LIBRARIES)

if(V8_FOUND)
    if(NOT TARGET V8::V8)
        add_library(V8::V8 SHARED IMPORTED)
	set_target_properties(V8::V8 PROPERTIES IMPORTED_LOCATION "${V8_LIB}")
	target_include_directories(V8::V8 INTERFACE "${V8_INCLUDE_DIRS}")
	target_link_libraries(V8::V8 INTERFACE "${V8_LIBRARIES}")
    endif()
endif()
