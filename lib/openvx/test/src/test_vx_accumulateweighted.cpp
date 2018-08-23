
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuAccumulateWeightedImage)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
    // TODO: check ambiguity with spec: VX_DF_IMAGE_U8 or U16???? Code says this:
	vx_image accum = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
	
    vx_float32 alpha_val = 0.2f;
    vx_scalar alpha = vxCreateScalar(context, VX_TYPE_FLOAT32, &alpha_val);
                                     
    vx_status status = vxuAccumulateWeightedImage(context,input,alpha,accum);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&accum);
	vxReleaseContext(&context);
}
