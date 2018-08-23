
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuChannelCombine)
{
	vx_context context = vxCreateContext();
	
    vx_image plane0 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	vx_image plane1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image plane2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image combined = vxCreateImage(context, 512, 512, VX_DF_IMAGE_RGB);

	vx_status status = vxuChannelCombine(context,plane0,plane1,plane2,NULL,combined);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&combined);
	vxReleaseContext(&context);
}
