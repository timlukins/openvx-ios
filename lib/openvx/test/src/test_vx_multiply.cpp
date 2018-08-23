
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuMultiply)
{
	vx_context context = vxCreateContext();
	
	vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_enum overflow_policy = VX_CONVERT_POLICY_SATURATE; // Must set to something!
    vx_enum round_policy = VX_ROUND_POLICY_TO_ZERO;
    
    vx_status status = vxuMultiply(context, input1, input2, 1.0, overflow_policy, round_policy, result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
