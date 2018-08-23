
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuIntegralImage)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);

    vx_image output = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U32);
    
    vx_status status = vxuIntegralImage(context, input, output);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output);
	vxReleaseContext(&context);
}
