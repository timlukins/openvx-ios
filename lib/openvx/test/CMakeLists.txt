if (NOT (APPLE AND IOS)) # Don't bother for iOS

	add_subdirectory( lib/gtest-1.7.0 )
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ./include ../include)

	add_subdirectory( src )

	file(GLOB TEST_SRC_FILES src/*.cpp)

	add_executable(testinvx ../build-macos/vxtest.cpp ${TEST_SRC_FILES}) # For this wrapper - just add vx.cpp and the contents of src
	
	target_link_libraries(testinvx
		gtest_main # Building for MacOS || Linux - provide generic main method
		openvx-test
		openvx-lib
		vxu
		)

	# Have to add this for cmake/ctest to "see" the test executable
	add_test(testinvx testinvx)

else (NOT (APPLE AND IOS))

	# For iOS add the src directory so that the openvx-test library gets built!
	add_subdirectory( src )

endif (NOT (APPLE AND IOS))
