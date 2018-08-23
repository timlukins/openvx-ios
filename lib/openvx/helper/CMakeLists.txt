#
# Copyright (c) 2011-2014 The Khronos Group Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
#

# set target name
set( TARGET_NAME openvx-helper )

include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR} 
					 ${CMAKE_SOURCE_DIR}/include )

FIND_SOURCES()

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} ${SOURCE_FILES})

if (APPLE AND IOS)
	SET (CMAKE_OSX_ARCHITECTURES "$(ARCHS_UNIVERSAL_IPHONE_OS)")
	set_xcode_property(${TARGET_NAME} IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET})	
	set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${IOS_CODE_SIGN_ID})
endif (APPLE AND IOS)

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION lib  
					LIBRARY DESTINATION lib ) # Update to put all libraries in right place TODO: maybe only MACOS
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${HELPER_FOLDER} )
