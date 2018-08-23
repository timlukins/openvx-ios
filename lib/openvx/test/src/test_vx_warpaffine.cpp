
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuWarpAffine)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_matrix matrix = vxCreateMatrix(context,VX_TYPE_FLOAT32, 2, 3); // Note 2x3 matrix  6 coeffs
    
    vx_enum warp_type = VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR; // NOTE: TYPE_AREA not supported for warp for OpenVX1.0
    
    vx_status status = vxuWarpAffine(context,input,matrix,warp_type,result);
    
	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
