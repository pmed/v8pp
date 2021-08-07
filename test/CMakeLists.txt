# Test target
file(GLOB v8pp_test_cpp *.cpp)

list(SORT v8pp_test_cpp)

add_executable(v8pp_test ${v8pp_test_cpp})

target_link_libraries(v8pp_test PRIVATE v8pp::v8pp)

target_compile_options(v8pp_test PRIVATE ${V8PP_WARNING_OPTIONS})
