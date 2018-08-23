
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuGaussian3x3)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
	
	vx_status status = vxuGaussian3x3(context,input,outimg);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&outimg);
	vxReleaseContext(&context);
}
