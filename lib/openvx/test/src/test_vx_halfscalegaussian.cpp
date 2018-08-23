
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuHalfScaleGaussian)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_ERROR);
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image outimg = vxCreateImage(context, 256, 256, VX_DF_IMAGE_U8); //  Has to be half the input size!
	
    vx_status status = vxuHalfScaleGaussian(context, input, outimg, 3); // 3 and 5 supported

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&outimg);
	vxReleaseContext(&context);
}
