
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuNot)
{
	vx_context context = vxCreateContext();
	
    vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_status status = vxuNot(context,input,result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
