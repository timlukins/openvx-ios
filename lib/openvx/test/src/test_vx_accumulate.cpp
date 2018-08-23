
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuAccumulateImage)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image accum = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16); // Note accum is 16 bit
	
    vx_status status = vxuAccumulateImage(context,input,accum);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&accum);
	vxReleaseContext(&context);
}
