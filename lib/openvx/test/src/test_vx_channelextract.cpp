
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuChannelExtract)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_GRAPH);
	
	vx_image input = vxTestGetImage(context,COLOUR_MANDRILL_512_512_RGB);
	
	vx_image gray = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);

    vx_status status = vxuChannelExtract(context, input, VX_CHANNEL_R, gray);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&gray);
	vxReleaseContext(&context);
}
