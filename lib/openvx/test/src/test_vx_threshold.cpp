
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuThreshold)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_threshold thresh = vxCreateThreshold(context, VX_THRESHOLD_TYPE_BINARY, VX_TYPE_UINT8); // Can also be TYPE_RANGE
    vx_int32 bound = 50;
    vxSetThresholdAttribute(thresh, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_VALUE, &bound, sizeof(bound));
    
    vx_status status = vxuThreshold(context,input,thresh,result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
