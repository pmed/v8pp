# Copy all headers to ${CMAKE_CURRENT_BINARY_DIR}/v8pp
# It makes this repo a little bit submodule friendly
set(V8PP_INCLUDES_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${V8PP_INCLUDES_DIR}/v8pp")

file(GLOB v8pp_hpp RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.hpp)
file(GLOB v8pp_ipp RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.ipp)

foreach(f ${v8pp_hpp};${v8pp_ipp})
    set(_original "${CMAKE_CURRENT_LIST_DIR}/${f}")
    set(_linkname "${V8PP_INCLUDES_DIR}/v8pp/${f}")
    file(CREATE_LINK "${_original}" "${_linkname}" COPY_ON_ERROR)
endforeach()

# Header only library target
add_library(v8pp-header-only INTERFACE)

add_library(v8pp::v8pp-header-only ALIAS v8pp-header-only)

target_include_directories(v8pp-header-only INTERFACE
    $<BUILD_INTERFACE:${V8PP_INCLUDES_DIR}>
    $<INSTALL_INTERFACE:include/v8pp>)

target_compile_definitions(v8pp-header-only INTERFACE V8PP_HEADER_ONLY=1)

target_compile_features(v8pp-header-only INTERFACE cxx_std_14)

target_link_libraries(v8pp-header-only INTERFACE V8::V8)

# Static library target
file(GLOB v8pp_cpp *.cpp)
list(SORT v8pp_cpp)

add_library(v8pp STATIC ${v8pp_cpp})

add_library(v8pp::v8pp ALIAS v8pp)

target_include_directories(v8pp PUBLIC
    $<BUILD_INTERFACE:${V8PP_INCLUDES_DIR}>
    $<INSTALL_INTERFACE:include/v8pp>)

target_compile_definitions(v8pp PUBLIC V8PP_HEADER_ONLY=0)

target_compile_features(v8pp PUBLIC cxx_std_14)

target_link_libraries(v8pp PUBLIC V8::V8 ${CMAKE_DL_LIBS})

target_compile_options(v8pp PRIVATE ${V8PP_WARNING_OPTIONS})

set_property(TARGET v8pp PROPERTY POSITION_INDEPENDENT_CODE ON)
