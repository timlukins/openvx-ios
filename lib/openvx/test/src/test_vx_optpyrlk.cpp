
#include "vxtest.h"

TEST(OpenVXTestCase,test_vxuOpticalFlowPyrLK )
{
	vx_context context = vxCreateContext();
	
    vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    // TODO: how to load these into the pyramid???
    
    vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_pyramid old_images = vxCreatePyramid(context, 3, 0.5, 512, 512, VX_DF_IMAGE_U8);
    vx_pyramid new_images = vxCreatePyramid(context, 3, 0.5, 512, 512, VX_DF_IMAGE_U8);
    vx_array old_points = vxCreateArray(context,VX_TYPE_KEYPOINT,1000);
    vx_array new_points_estimates = vxCreateArray(context,VX_TYPE_KEYPOINT,1000);
    vx_array new_points = vxCreateArray(context,VX_TYPE_KEYPOINT,1000);
    vx_enum termination = VX_TERM_CRITERIA_BOTH;
    vx_float32 epsilon_val = 0.3f;
    vx_scalar epsilon = vxCreateScalar(context, VX_TYPE_FLOAT32, &epsilon_val);
    vx_uint32 iters_val = 500;
    vx_scalar num_iterations = vxCreateScalar(context, VX_TYPE_UINT32, &iters_val);
    vx_bool initial_val = vx_true_e; // or vx_false_e
    vx_scalar use_initial_estimate = vxCreateScalar(context, VX_TYPE_BOOL, &initial_val);
    vx_size window_dimension = 3;
    
    vx_status status = vxuOpticalFlowPyrLK(context,old_images,new_images,old_points,new_points_estimates,new_points,termination,epsilon,num_iterations,use_initial_estimate,window_dimension);

	ASSERT_EQ(status, VX_SUCCESS);

	vxReleaseImage(&result);
	vxReleaseContext(&context);
}
