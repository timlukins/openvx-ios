
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuMinMaxLoc)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
    vx_uint8 min_val;
    vx_scalar min = vxCreateScalar(context, VX_TYPE_UINT8, &min_val);
    vx_uint8 max_val;
    vx_scalar max = vxCreateScalar(context, VX_TYPE_UINT8, &max_val);
    vx_array min_loc = vxCreateArray(context,VX_TYPE_COORDINATES2D,1000);
    vx_array max_loc = vxCreateArray(context,VX_TYPE_COORDINATES2D,1000);
    vx_uint32 min_count_val;
    vx_scalar min_count = vxCreateScalar(context, VX_TYPE_UINT32, &min_count_val);
    vx_uint32 max_count_val;
    vx_scalar max_count = vxCreateScalar(context, VX_TYPE_UINT32, &max_count_val);
    
    vx_status status = vxuMinMaxLoc(context,input,min,max,min_loc,max_loc,min_count,max_count);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseContext(&context);
}
