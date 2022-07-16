# v8pp target

include(GNUInstallDirs)

if(DEFINED VCPKG_TARGET_TRIPLET)
	find_package(V8 CONFIG REQUIRED)
else()
	list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
	find_package(V8 REQUIRED)
endif()

configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

if(V8_COMPRESS_POINTERS)
	set(V8PP_DEFINES V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
endif()

set(V8PP_HEADERS
	${CMAKE_CURRENT_BINARY_DIR}/config.hpp
	call_from_v8.hpp
	call_v8.hpp
	class.hpp
	context.hpp
	convert.hpp
	function.hpp
	json.hpp
	module.hpp
	object.hpp
	property.hpp
	ptr_traits.hpp
	throw_ex.hpp
	utility.hpp
	version.hpp
)

if(V8PP_HEADER_ONLY)
	list(APPEND V8PP_HEADERS
		class.ipp
		json.ipp
		throw_ex.ipp
		version.ipp
	)
	set(V8PP_SOURCES)
else()
	set(V8PP_SOURCES
		class.cpp
		context.cpp
		convert.cpp
		json.cpp
		throw_ex.cpp
		version.cpp
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
	set(V8PP_COMPILE_OPTIONS -fno-rtti -fexceptions -Wall -Wextra -Wpedantic)
endif()

if(V8PP_HEADER_ONLY)
	add_library(v8pp INTERFACE)
	target_compile_definitions(v8pp INTERFACE ${V8PP_DEFINES})
	target_compile_options(v8pp INTERFACE ${V8PP_COMPILE_OPTIONS})
	target_include_directories(v8pp INTERFACE
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	target_link_libraries(v8pp INTERFACE V8::V8)
else()
	add_library(v8pp ${V8PP_HEADERS} ${V8PP_SOURCES})
	target_compile_definitions(v8pp PUBLIC ${V8PP_DEFINES})
	target_compile_options(v8pp PUBLIC ${V8PP_COMPILE_OPTIONS})
	target_include_directories(v8pp PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	target_link_libraries(v8pp PUBLIC V8::V8)
	if(BUILD_SHARED_LIBS)
		target_link_libraries(v8pp PUBLIC ${CMAKE_DL_LIBS})
	endif()
endif()

#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${V8PP_HEADERS} ${V8PP_SOURCES})

# Install
include(CMakePackageConfigHelpers)

set(targets_export_name v8pp_Targets)

write_basic_package_version_file("${PROJECT_BINARY_DIR}/ConfigVersion.cmake" COMPATIBILITY SameMajorVersion)
configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in" "${PROJECT_BINARY_DIR}/Config.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/v8pp/cmake)

install(TARGETS v8pp EXPORT ${targets_export_name})
install(EXPORT ${targets_export_name}
	NAMESPACE v8pp::
	FILE ${targets_export_name}.cmake
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/v8pp")

install(FILES "${PROJECT_BINARY_DIR}/ConfigVersion.cmake" "${PROJECT_BINARY_DIR}/Config.cmake"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/v8pp")

install(FILES ${V8PP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/v8pp)
