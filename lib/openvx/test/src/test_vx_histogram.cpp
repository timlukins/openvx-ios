
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuHistogram)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);

	vx_distribution dist = vxCreateDistribution(context,256,0,256);

	vx_status status = vxuHistogram(context,input,dist);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseDistribution(&dist);
	vxReleaseContext(&context);
}
