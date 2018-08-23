
#include "vxtest.h"

TEST(OpenVXTestCase,DISABLED_test_vxuConvolve)
{
	vx_context context = vxCreateContext();
	
	vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
	
	vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
	// A horizontal Scharr gradient operator with different scale.
	vx_int16 gx[3][3] = {
		{  3, 0, -3},
		{ 10, 0,-10},
		{  3, 0, -3},
	};
	vx_uint32 scale = 9;
	vx_convolution scharr_x = vxCreateConvolution(context, 3, 3);
	vxReadConvolutionCoefficients(scharr_x, NULL);
	vxWriteConvolutionCoefficients(scharr_x, (vx_int16*)gx);
	vxSetConvolutionAttribute(scharr_x, VX_CONVOLUTION_ATTRIBUTE_SCALE, &scale, sizeof(scale));
	
	vx_status status = vxuConvolve(context,input,scharr_x,outimg);

	ASSERT_EQ(status, VX_SUCCESS);
    
	vxReleaseConvolution(&scharr_x);
	vxReleaseImage(&outimg);
	vxReleaseContext(&context);

}
