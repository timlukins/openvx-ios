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

if ( EXPERIMENTAL_USE_OPENCL )
    # set target name
    set( TARGET_NAME openvx-opencl )

    include_directories( BEFORE
                     ${CMAKE_CURRENT_SOURCE_DIR}
					 ${CMAKE_CURRENT_SOURCE_DIR}/../../include
					 ${CMAKE_SOURCE_DIR}/include
					 ${CMAKE_SOURCE_DIR}/debug
                     ${OPENCL_INCLUDE_PATH} )
					     					 
    set( INVERTED_COMMA "\"" )
	set( CL_SOURCE_DIR ${INVERTED_COMMA}${CMAKE_SOURCE_DIR}/kernels/opencl${INVERTED_COMMA} )
					 
	add_definitions( -DVX_CL_SOURCE_DIR=${CL_SOURCE_DIR} )
			
    FIND_SOURCES()
	
	if ((WIN32) OR (CYGWIN))
		set( DEF_FILE openvx-target.def )
	endif ((WIN32) OR (CYGWIN))

    # add a target named ${TARGET_NAME}
    add_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${DEF_FILE})
	
    if (CYGWIN)
        set_target_properties( ${TARGET_NAME} PROPERTIES LINK_FLAGS ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} )
    endif (CYGWIN)

    target_link_libraries( ${TARGET_NAME} openvx )

    install ( TARGETS ${TARGET_NAME} 
              RUNTIME DESTINATION bin
              ARCHIVE DESTINATION lib
              LIBRARY DESTINATION bin )
			  
	set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${SAMPLE_TARGETS_FOLDER} )
		  
endif ( EXPERIMENTAL_USE_OPENCL )