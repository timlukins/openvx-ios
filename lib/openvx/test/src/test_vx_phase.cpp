
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuPhase)
{
	vx_context context = vxCreateContext();
	
	//vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    // TODO: need signed version of test image method...
    vx_image xgrad = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16);
    vx_image ygrad = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16);
    
    vx_image output = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_status status = vxuPhase(context, xgrad, ygrad, output);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output);
	vxReleaseContext(&context);
}
