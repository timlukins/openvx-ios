
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuFastCorners)
{
	vx_context context = vxCreateContext();
    //vx_set_debug_zone(VX_ZONE_ERROR);
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
	
    vx_float32 strength_val = 10.0f; // NOTE: 0<x<256
    vx_scalar strength_thresh = vxCreateScalar(context, VX_TYPE_FLOAT32, &strength_val);
    vx_bool dononmax_val = vx_true_e; // NOTE: this will "cast" to scalar internally
    //vx_scalar dononmax = vxCreateScalar(context, VX_TYPE_BOOL, &dononmax_val);
    vx_size num_corners_val = 100; // NOTE: must do it his way an use a vx_size value to intialise array as well! Otherwise it fails...
    vx_scalar num_corners = vxCreateScalar(context, VX_TYPE_SIZE, &num_corners_val); // Optional - but note TYPE_SIZE
    vx_array corners = vxCreateArray(context,VX_TYPE_KEYPOINT,num_corners_val); // From looking at code, 0 is undefined
    
    vx_status status = vxuFastCorners(context,input,strength_thresh,vx_true_e,corners,num_corners);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&outimg);
	vxReleaseContext(&context);
}
