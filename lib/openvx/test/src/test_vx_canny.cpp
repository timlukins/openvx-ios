
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuCanny)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image edges = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
	
	vx_threshold hyst = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
	vx_int32 lower = 50, upper = 100;
	vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_LOWER, &lower, sizeof(lower));
	vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_UPPER, &upper, sizeof(upper));

	vx_status status = vxuCannyEdgeDetector(context,input,hyst,3,VX_NORM_L1,edges);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&edges);
	vxReleaseContext(&context);
}
