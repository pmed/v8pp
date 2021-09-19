cmake_minimum_required(VERSION 3.12)

project(v8pp
	VERSION 1.7.0
	DESCRIPTION "Bind C++ functions and classes into V8 JavaScript engine"
	HOMEPAGE_URL https://github.com/pmed/v8pp
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_TESTING "Build and run tests" OFF)

# Custom project options
set(V8PP_HEADER_ONLY 0 CACHE BOOL "Header-only library")
set(V8PP_ISOLATE_DATA_SLOT 0 CACHE STRING "v8::Isolate data slot number, used in v8pp for shared data")
set(V8PP_PLUGIN_INIT_PROC_NAME "v8pp_module_init" CACHE STRING "v8pp plugin initialization procedure name")
set(V8PP_PLUGIN_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX} CACHE STRING "v8pp plugin filename suffix")
option(V8_COMPRESS_POINTERS "Use new V8 ABI with V8_COMPRESS_POINTERS and V8_31BIT_SMIS_ON_64BIT_ARCH" ON)

add_subdirectory(v8pp)

if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(plugins)
	add_subdirectory(test)
endif()
