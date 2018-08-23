
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuSubtract)
{
	vx_context context = vxCreateContext();
	
	vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_enum policy = VX_CONVERT_POLICY_SATURATE; // Must set to something!
    //vx_enum policy_val = VX_CONVERT_POLICY_WRAP;
    //vx_scalar policy = vxCreateScalar(context, VX_TYPE_ENUM, &policy_val);
    
    vx_status status = vxuSubtract(context,input1,input2,policy,result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
