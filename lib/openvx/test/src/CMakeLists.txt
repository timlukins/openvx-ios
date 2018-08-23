# set target name
set( TARGET_NAME openvx-test )

include_directories( BEFORE
            ${CMAKE_CURRENT_SOURCE_DIR} 
					 ${CMAKE_SOURCE_DIR}/include 
					 ../include) # NB> so can find vxtest.h

FIND_SOURCES()

#message(STATUS ">>>>>>>>>" ${SOURCE_FILES})
# add a target named ${TARGET_NAME}
add_library (${TARGET_NAME} ${SOURCE_FILES})

if (APPLE AND IOS)
	SET (CMAKE_OSX_ARCHITECTURES "$(ARCHS_UNIVERSAL_IPHONE_OS)")
	set_xcode_property(${TARGET_NAME} IPHONEOS_DEPLOYMENT_TARGET ${IOS_DEPLOYMENT_TARGET})	
	set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${IOS_CODE_SIGN_ID})
endif (APPLE AND IOS)

set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER "Test" )
