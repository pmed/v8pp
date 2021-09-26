find_package(Doxygen REQUIRED)

configure_file(Doxyfile.in Doxyfile @ONLY)
add_custom_target(doxygen
	COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
	COMMENT "Generating API documentation with Doxygen"
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html TYPE DOC)
