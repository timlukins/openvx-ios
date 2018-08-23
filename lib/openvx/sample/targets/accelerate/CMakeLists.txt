# (c) Machines With Vision, 2015

macro(ADD_FRAMEWORK appname fwname)
	# From http://stackoverflow.com/questions/12547624/cant-link-macos-frameworks-with-cmake
	find_library(FRAMEWORK_${fwname}
		NAMES ${fwname}
		PATHS ${CMAKE_OSX_SYSROOT}/System/Library
		PATH_SUFFIXES Frameworks
		NO_DEFAULT_PATH)
	if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
		MESSAGE(ERROR ": Framework ${fwname} not found")
	else()
		TARGET_LINK_LIBRARIES(${appname} "${FRAMEWORK_${fwname}}/${fwname}")
		MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
	endif()
endmacro(ADD_FRAMEWORK)

# set target name
#set( TARGET_NAME openvx-accelerate)
set( TARGET_NAME invx)

include_directories( BEFORE
                    ${CMAKE_CURRENT_SOURCE_DIR}
										${CMAKE_CURRENT_SOURCE_DIR}/../../include
										${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/kernels/c_model					 
										${CMAKE_SOURCE_DIR}/debug
                    ${OPENCL_INCLUDE_PATH} )

FIND_SOURCES()

# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} SHARED ${SOURCE_FILES})

# Should only ever be built for Apple/IOS - but just in case...
if (APPLE AND IOS)
	SET (CMAKE_OSX_ARCHITECTURES "$(ARCHS_UNIVERSAL_IPHONE_OS)")
	set_xcode_property(${TARGET_NAME} IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET})	
	set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${IOS_CODE_SIGN_ID})
endif (APPLE AND IOS)
  
if (APPLE)
  # Crucially add the Accelerate framework to link against!
	add_framework(${TARGET_NAME} Accelerate)
endif (APPLE)

if (APPLE AND IOS) # NOTICE - only include static stuff for IOS
	target_link_libraries( ${TARGET_NAME} openvx-debug-lib openvx-extras-lib openvx-c_model-lib)
else (APPLE AND IOS)
	target_link_libraries( ${TARGET_NAME} openvx-debug-lib openvx-extras-lib openvx-helper openvx-c_model-lib openvx-lib vxu )
endif (APPLE AND IOS)

install ( TARGETS ${TARGET_NAME} 
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION lib
					LIBRARY DESTINATION lib ) # Updated to put lib in right place - TODO: maybe only MACOS
		  
set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER ${SAMPLE_TARGETS_FOLDER} )
