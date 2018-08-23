
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuColorConvert)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,COLOUR_MANDRILL_512_512_RGB);
	
	vx_image output = vxCreateImage(context, 512, 512, VX_DF_IMAGE_IYUV);

    // Commented out due to ocasional memory issue
    vx_status status = vxuColorConvert(context, input, output);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&output); // TODO issue here with memory somehow allocated in colorConvert
	vxReleaseContext(&context);
}
