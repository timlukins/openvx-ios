
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuMeanStdDev)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_float32 mean;
    vx_float32 stddev; // A bit weird not scalars...
    
    vx_status status = vxuMeanStdDev(context, input, &mean, &stddev);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseContext(&context);
}
