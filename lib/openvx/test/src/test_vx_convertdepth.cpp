
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuConvertDepth)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image output = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16);

    vx_enum policy = VX_CONVERT_POLICY_WRAP;
    
    vx_int32 shift_val = 10;
    vx_scalar shift = vxCreateScalar(context, VX_TYPE_INT32, &shift_val);
    // Commented out due to occasional memory issue
    vx_status status = vxuConvertDepth(context,input,output,policy,shift_val);//shift); // TODO: this is inconsistent with vxAccumSq etc.

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output);
	vxReleaseContext(&context);
}
