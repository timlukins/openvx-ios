#if (__APPLE__)
    #include "TargetConditionals.h" // To look up what we are targetting
    #if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR) // Actually both the same - but could be different
        #import <gtest/gtest.h> // Framework version!
        #import <OpenVX/vx.h>
        #import <OpenVX/vxu.h>
        #import <OpenVX/vx_debug.h>
    #else
        #include "gtest/gtest.h" // Library version
        #include <VX/vx.h>
        #include <VX/vxu.h>
    #endif

    //#import <Foundation/Foundation.h>
    //#define TICK   NSDate *startTime = [NSDate date]
    //#define TOCK NSLog(@"%s Time: %f", __func__, -[startTime timeIntervalSinceNow])

#endif


// Generic filename/resource references for all tests 

#define ORIGINAL_MANDRILL_512_512_1U8				"original_mandrill_512_512_1U8"
#define COLOUR_MANDRILL_512_512_RGB                 "colour_mandrill_512_512_RGB"
#define BOX_FILTERED_MANDRILL_512_512_1U8           "box_filtered_mandrill_512_512_1U8"
#define CONST_VALUE_TEN_512_512_1U8                 "const_value_ten_512_512_1U8"

// I/O and utility functions to be implemented

vx_image vxTestGetImage(vx_context,const char*);
void vxTestSaveImage(vx_image,char*);
//void vxTestCompareImage(vx_image,vx_image);

// Platform methods for performance timings...
void vxTestLogStart();
void vxTestLogEnd(char*);