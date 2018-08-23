
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuTable)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
	vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    vx_lut lut = vxCreateLUT(context, VX_TYPE_UINT8, 256); // Note: these are the only possible vals for OpenVX1.0
    
    vx_status status = vxuTableLookup(context, input, lut, result);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
