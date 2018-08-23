
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuHarrisCorners)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_GRAPH);
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    //ASSERT(input && (input->format == VX_DF_IMAGE_U8));
	
    vx_float32 strength_val = 10;
    vx_scalar strength_thresh = vxCreateScalar(context, VX_TYPE_FLOAT32, &strength_val);
    vx_float32 dist_val = 3.0f;
    vx_scalar min_dist = vxCreateScalar(context, VX_TYPE_FLOAT32, &dist_val);
    vx_float32 sensitivity_val = 0.10f;
    vx_scalar sensitivity = vxCreateScalar(context, VX_TYPE_FLOAT32, &sensitivity_val);
    vx_size num_corners_val = 100;
    vx_scalar num_corners = vxCreateScalar(context, VX_TYPE_SIZE, &num_corners_val); // TODO: spec says TYPE_SIZE but had to change C_code from UINT32
    vx_array corners = vxCreateArray(context,VX_TYPE_KEYPOINT,num_corners_val); // Again, code says 0 capacity expected, hmmm, does it matter...
    
    vx_status status = vxuHarrisCorners(context,input,strength_thresh,min_dist,sensitivity,3,5,corners,num_corners);

	ASSERT_EQ(status, VX_SUCCESS);

    vxReleaseArray(&corners);
    vxReleaseScalar(&num_corners);
    vxReleaseScalar(&sensitivity);
    vxReleaseScalar(&min_dist);
    vxReleaseScalar(&strength_thresh);
	vxReleaseContext(&context);
}
