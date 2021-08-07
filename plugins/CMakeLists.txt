# sample plugin targets

if(BUILD_SHARED_LIBS)
	add_library(console MODULE console.cpp)
	set_target_properties(console PROPERTIES PREFIX "")
	target_link_libraries(console v8pp)

	add_library(file MODULE file.cpp)
	set_target_properties(file PROPERTIES PREFIX "")
	target_link_libraries(file v8pp)
endif()
