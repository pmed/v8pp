cmake_minimum_required(VERSION 3.14)
# using file(CREATE_LINK) that was added in 3.14

# Get version from config.hpp
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/v8pp/config.hpp config_hpp)
if(NOT config_hpp MATCHES "V8PP_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"")
    message(FATAL_ERROR "Cannot get V8PP_VERSION from config.hpp.")
endif()
unset(config_hpp)
# Use math to skip leading zeros if any.
math(EXPR V8PP_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR V8PP_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR V8PP_VERSION_PATCH ${CMAKE_MATCH_3})
set(V8PP_VERSION ${V8PP_VERSION_MAJOR}.${V8PP_VERSION_MINOR}.${V8PP_VERSION_PATCH})

project(v8pp VERSION ${V8PP_VERSION} LANGUAGES CXX)

# Determine if v8pp is built as a subproject (using add_subdirectory)
# or if it is the master project.
if(NOT DEFINED V8PP_MASTER_PROJECT)
    set(V8PP_MASTER_PROJECT OFF)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(V8PP_MASTER_PROJECT ON)
    endif()
endif()

# Options that control generation of various targets.
option(V8PP_WARNING "Enable extra warnings and expensive tests." ${V8PP_MASTER_PROJECT})
option(V8PP_WERROR  "Halt the compilation with an error on compiler warnings." OFF)
option(V8PP_TEST    "Generate the test target." ${V8PP_MASTER_PROJECT})
option(V8PP_INSTALL "Generate install target." ${V8PP_MASTER_PROJECT})

# Warnings setup
set(V8PP_WARNING_OPTIONS)

if(V8PP_WARNING)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND V8PP_WARNING_OPTIONS "-Wall")
        list(APPEND V8PP_WARNING_OPTIONS "-Wextra")
        list(APPEND V8PP_WARNING_OPTIONS "-Wno-unused-function")
    elseif(
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    )
        list(APPEND V8PP_WARNING_OPTIONS "-Wall")
        list(APPEND V8PP_WARNING_OPTIONS "-Wextra")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_msvc_warning_flags(WARNING_OPTIONS)
        list(APPEND V8PP_WARNING_OPTIONS "/W3")
    else()
        message(SEND_ERROR "unknown compiler \"${CMAKE_CXX_COMPILER_ID}\"")
    endif()
endif()

if(V8PP_WERROR)
    if(
        CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    )
        list(APPEND V8PP_WARNING_OPTIONS "-Werror")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_msvc_warning_flags(WARNING_OPTIONS)
        list(APPEND V8PP_WARNING_OPTIONS "/WX")
    else()
        message(SEND_ERROR "unknown compiler \"${CMAKE_CXX_COMPILER_ID}\"")
    endif()
endif()

# Find V8 package
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

find_package(V8 REQUIRED)

add_subdirectory(v8pp)

if(V8PP_TEST)
    add_subdirectory(test)
endif()

if(V8PP_INSTALL)
    # cmake code for install is based on https://github.com/onqtam/doctest/blob/master/CMakeLists.txt
    
    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    
    if(CMAKE_SYSTEM_NAME STREQUAL Linux)
        include(GNUInstallDirs)
        set(include_install_dir ${CMAKE_INSTALL_INCLUDEDIR})
        set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
    else()
        set(include_install_dir "include")
        set(config_install_dir "lib/cmake/${PROJECT_NAME}")
    endif()
    
    set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
    set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
    set(targets_export_name "${PROJECT_NAME}Targets")
    set(namespace "${PROJECT_NAME}::")
    
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${version_config}"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    configure_file("Config.cmake.in" "${project_config}" @ONLY)
    
    install(
        TARGETS v8pp v8pp-header-only
        EXPORT "${targets_export_name}"
        INCLUDES DESTINATION "${include_install_dir}"
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/v8pp/include/"
        DESTINATION "${include_install_dir}/"
    )

    install(
        FILES "${project_config}" "${version_config}"
        DESTINATION "${config_install_dir}"
    )

    install(
        EXPORT "${targets_export_name}"
        NAMESPACE "${namespace}"
        DESTINATION "${config_install_dir}"
    )
endif()
