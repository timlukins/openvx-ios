
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuGaussianPyramid)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_GRAPH);
    //vx_set_debug_zone(VX_ZONE_ERROR);
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_size levels = 4;
    
    vx_pyramid pyramid = vxCreatePyramid(context, levels, VX_SCALE_PYRAMID_HALF, 512, 512, VX_DF_IMAGE_U8);
	
	vx_status status = vxuGaussianPyramid(context,input,pyramid);

	ASSERT_EQ(status, VX_SUCCESS);
    
    vxReleasePyramid(&pyramid);
	vxReleaseContext(&context);
}
