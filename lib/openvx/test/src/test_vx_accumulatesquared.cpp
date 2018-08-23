
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuAccumulateSquareImage)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_SCALAR);
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image accum = vxCreateImage(context, 512, 512, VX_DF_IMAGE_S16); // Note accum is 16bit
	
    // TODO: check ambiguity with spec: vx_float32 or vx_uint32??? Code says this:
    vx_uint32 shift_val = 10;
    vx_scalar shift = vxCreateScalar(context, VX_TYPE_UINT32, &shift_val);
    
    vx_status status = vxuAccumulateSquareImage(context,input,shift,accum);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&accum);
	vxReleaseContext(&context);
}
