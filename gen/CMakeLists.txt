project(generator)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if( CLANG_FORMAT_EXECUTABLE )
	# get the clang-format version string
	execute_process( COMMAND ${CLANG_FORMAT_EXECUTABLE} "--version" OUTPUT_VARIABLE clangFormatVersion )
	# filter out the actual version
	string( REGEX MATCH [0123456789.]+ clangFormatVersion "${clangFormatVersion}" )
	file( TO_NATIVE_PATH ${CLANG_FORMAT_EXECUTABLE} CLANG_FORMAT_EXECUTABLE )
	string( REPLACE "\\" "\\\\" CLANG_FORMAT_EXECUTABLE ${CLANG_FORMAT_EXECUTABLE} )

	message( STATUS " Found clang-format version <" ${clangFormatVersion} ">." )
else()
	message( WARNING " Could not find clang-format. Generated files will not be nicely formatted." )
endif()

add_executable(${PROJECT_NAME} generator.cpp)
target_link_libraries(${PROJECT_NAME} 
	tinyxml2::tinyxml2
)
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		CXX_STANDARD 23
)
target_compile_definitions(${PROJECT_NAME} 
	PRIVATE
		CLANG_FORMAT_EXECUTABLE="${CLANG_FORMAT_EXECUTABLE}"
)