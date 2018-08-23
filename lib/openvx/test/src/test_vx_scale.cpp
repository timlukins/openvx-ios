
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuScaleImage)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image output = vxCreateImage(context, 128, 128, VX_DF_IMAGE_U8);
    
    vx_enum inter_type = VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR;
    
    vx_status status = vxuScaleImage(context, input, output, inter_type);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output);
	vxReleaseContext(&context);
}
