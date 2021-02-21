cmake_minimum_required(VERSION 3.1)

project(v8pp VERSION 1.7.0 LANGUAGES CXX)

find_package(V8 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared library" ON)

# Custom project options
set(V8PP_HEADER_ONLY 0 CACHE BOOL "Header-only library")
set(V8PP_ISOLATE_DATA_SLOT 0 CACHE STRING "v8::Isolate data slot number, used in v8pp for shared data")
set(V8PP_PLUGIN_INIT_PROC_NAME "v8pp_module_init" CACHE STRING "v8pp plugin initialization procedure name")
set(V8PP_PLUGIN_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX} CACHE STRING "v8pp plugin filename suffix")
set(V8PP_WITH_TESTS 1 CACHE BOOL "build with tests")

configure_file(v8pp/config.hpp.in v8pp/config.hpp)

set(V8PP_HEADERS
	${CMAKE_SOURCE_DIR}/v8pp/call_from_v8.hpp
	${CMAKE_SOURCE_DIR}/v8pp/call_v8.hpp
	${CMAKE_SOURCE_DIR}/v8pp/class.hpp
	${CMAKE_BINARY_DIR}/v8pp/config.hpp
	${CMAKE_SOURCE_DIR}/v8pp/context.hpp
	${CMAKE_SOURCE_DIR}/v8pp/convert.hpp
	${CMAKE_SOURCE_DIR}/v8pp/function.hpp
	${CMAKE_SOURCE_DIR}/v8pp/json.hpp
	${CMAKE_SOURCE_DIR}/v8pp/module.hpp
	${CMAKE_SOURCE_DIR}/v8pp/object.hpp
	${CMAKE_SOURCE_DIR}/v8pp/property.hpp
	${CMAKE_SOURCE_DIR}/v8pp/ptr_traits.hpp
	${CMAKE_SOURCE_DIR}/v8pp/throw_ex.hpp
	${CMAKE_SOURCE_DIR}/v8pp/utility.hpp
	${CMAKE_SOURCE_DIR}/v8pp/version.hpp
)

if(V8PP_HEADER_ONLY)
	set(V8PP_SOURCES
		${CMAKE_SOURCE_DIR}/v8pp/class.ipp
		${CMAKE_SOURCE_DIR}/v8pp/json.ipp
		${CMAKE_SOURCE_DIR}/v8pp/throw_ex.ipp
		${CMAKE_SOURCE_DIR}/v8pp/version.ipp
	)
else()
	set(V8PP_SOURCES
		${CMAKE_SOURCE_DIR}/v8pp/class.cpp
		${CMAKE_SOURCE_DIR}/v8pp/context.cpp
		${CMAKE_SOURCE_DIR}/v8pp/convert.cpp
		${CMAKE_SOURCE_DIR}/v8pp/json.cpp
		${CMAKE_SOURCE_DIR}/v8pp/throw_ex.cpp
		${CMAKE_SOURCE_DIR}/v8pp/version.cpp
	)
endif()

#set(CMAKE_CXX_RTTI OFF)
if(MSVC)
	set(V8PP_COMPILE_OPTIONS /GR- /EHsc /permissive- /W4)
	# disable specific warnings
	list(APPEND V8PP_COMPILE_OPTIONS /wd4190)
	# set warning level 3 for system headers
	list(APPEND V8PP_COMPILE_OPTIONS /experimental:external /external:anglebrackets /external:W3)
else()
	set(V8PP_COMPILE_OPTIONS -fno-rtti -Wall -Wextra -Wpedantic)
endif()

if(V8PP_HEADER_ONLY)
	add_library(v8pp INTERFACE)
	target_sources(v8pp INTERFACE ${V8PP_HEADERS} ${V8PP_SOURCES})
	target_compile_options(v8pp INTERFACE ${V8PP_COMPILE_OPTIONS})
	target_include_directories(v8pp INTERFACE ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
	target_include_directories(v8pp SYSTEM INTERFACE ${V8_INCLUDE_DIR})
	target_link_libraries(v8pp INTERFACE V8::V8)
else()
	add_library(v8pp ${V8PP_HEADERS} ${V8PP_SOURCES})
	target_compile_options(v8pp PRIVATE ${V8PP_COMPILE_OPTIONS})
	target_include_directories(v8pp PUBLIC ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
	target_include_directories(v8pp SYSTEM PUBLIC ${V8_INCLUDE_DIR})
	target_link_libraries(v8pp PUBLIC V8::V8)
endif()

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${V8PP_HEADERS} ${V8PP_SOURCES})

if(BUILD_SHARED_LIBS)
	if (NOT V8PP_HEADER_ONLY)
		target_link_libraries(v8pp PUBLIC ${CMAKE_DL_LIBS})
	endif()

	add_library(console MODULE plugins/console.cpp)
	set_target_properties(console PROPERTIES PREFIX "")
	target_link_libraries(console v8pp)

	add_library(file MODULE plugins/file.cpp)
	set_target_properties(file PROPERTIES PREFIX "")
	target_link_libraries(file v8pp)
endif()


if(V8PP_WITH_TESTS)
	enable_testing()
	add_executable(v8pp_test
		test/main.cpp
		test/test.hpp
		test/test_call_from_v8.cpp
		test/test_call_v8.cpp
		test/test_class.cpp
		test/test_context.cpp
		test/test_convert.cpp
		test/test_factory.cpp
		test/test_function.cpp
		test/test_json.cpp
		test/test_module.cpp
		test/test_object.cpp
		test/test_property.cpp
		test/test_ptr_traits.cpp
		test/test_throw_ex.cpp
		test/test_utility.cpp
	)
	if (V8PP_HEADER_ONLY)
		target_sources(v8pp_test PRIVATE v8pp/context.cpp)
	endif()

	target_link_libraries(v8pp_test v8pp)
	if(BUILD_SHARED_LIBS)
		target_link_libraries(v8pp_test ${CMAKE_DL_LIBS})
	endif()

	add_test(v8pp_test v8pp_test --version --run-tests)
	set_tests_properties(v8pp_test PROPERTIES ENVIRONMENT LD_LIBRARY_PATH=${V8_LIB_DIR}:$ENV{LD_LIBRARY_PATH})

	if(BUILD_SHARED_LIBS)
		file(GLOB JS_TESTS test/*.js)
		add_test(v8pp_js_test v8pp_test --lib-path ${CMAKE_BINARY_DIR} ${JS_TESTS})
		set_tests_properties(v8pp_js_test PROPERTIES ENVIRONMENT LD_LIBRARY_PATH=${V8_LIB_DIR}:$ENV{LD_LIBRARY_PATH})
	endif()
endif()

# Install
#include(GNUInstallDirs)
#include(CMakePackageConfigHelpers)

#write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" COMPATIBILITY SameMajorVersion)
#export(TARGETS ${PROJECT_NAME} NAMESPACE ${PROJECT_NAME}:: FILE "${PROJECT_NAME}Config.cmake")
#install(DIRECTORY "include" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#install(TARGETS ${PROJECT_NAME} EXPORT "${PROJECT_NAME}Config" INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
#install(EXPORT "${PROJECT_NAME}Config" NAMESPACE ${PROJECT_NAME}:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
#install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

macro(find_v8_dev)
	if(NOT DEFINED V8_INCLUDE_DIR)
		set(V8_INCLUDE_DIR ${V8_DIR}/include)
	endif()
	message(STATUS "V8 include dir: ${V8_INCLUDE_DIR}")
	if(NOT EXISTS "${V8_INCLUDE_DIR}")
		message(FATAL_ERROR "No V8 include dir found")
	endif()

	if(NOT DEFINED V8_LIB_DIR)
		set(V8_LIB_DIR ${V8_DIR}/lib)
	endif()
	message(STATUS "V8 lib dir: ${V8_LIB_DIR}")
	if(NOT EXISTS "${V8_LIB_DIR}")
		message(FATAL_ERROR "No V8 lib dir found")
	endif()

	if(BUILD_SHARED_LIBS)
		file(GLOB V8_LIBRARIES ${V8_LIB_DIR}/*${CMAKE_SHARED_LIBRARY_SUFFIX})
	else()
		file(GLOB V8_LIBRARIES ${V8_LIB_DIR}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
	endif()
	message(STATUS "V8 libraries: ${V8_LIBRARIES}")
	if(NOT V8_LIBRARIES)
		message(FATAL_ERROR "No V8 libraries found")
	endif()
endmacro()
