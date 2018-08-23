
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuAbsDiff)
{
	vx_context context = vxCreateContext();
	
	vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
	
    vx_status status = vxuAbsDiff(context, input1, input2, outimg);

    ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&outimg);
	vxReleaseContext(&context);
}
