
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuXor)
{
	vx_context context = vxCreateContext();
	
    vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_status status = vxuXor(context,input1,input2,result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
