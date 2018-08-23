
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuRemap)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image output = vxCreateImage(context, 128, 128, VX_DF_IMAGE_U8);
    
    vx_remap table = vxCreateRemap(context, 512, 512, 128, 128);
    
    for (int i = 0; i < 128; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            vxSetRemapPoint(table, j, i, (vx_float32)j, (vx_float32)i);
        }
    }
    
    vx_enum policy = VX_INTERPOLATION_TYPE_BILINEAR;
    
    vx_status status = vxuRemap(context, input, table, policy, output);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output);
	vxReleaseContext(&context);
}
