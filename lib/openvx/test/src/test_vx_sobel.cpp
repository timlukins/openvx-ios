#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuSobel3x3)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image dximg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16);
	vx_image dyimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16);
	
	vx_status status = vxuSobel3x3(context,input,dximg,dyimg);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&dximg);
	vxReleaseImage(&dyimg);
	vxReleaseContext(&context);
}
