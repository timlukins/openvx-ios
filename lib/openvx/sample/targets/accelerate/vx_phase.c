/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>
#include <math.h>


// nodeless version of the Phase kernel
vx_status vxPhase(vx_image grad_x, vx_image grad_y, vx_image output)
{
    vx_uint32 y, x;
    vx_uint8 *dst_base   = NULL;
    vx_int16 *src_base_x = NULL;
    vx_int16 *src_base_y = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr_x, src_addr_y;
    vx_rectangle_t rect;
    vx_status status = VX_FAILURE;
    if (grad_x == 0 && grad_y == 0)
        return VX_ERROR_INVALID_PARAMETERS;
    
    status = vxGetValidRegionImage(grad_x, &rect);
    status |= vxAccessImagePatch(grad_x, &rect, 0, &src_addr_x, (void **)&src_base_x, VX_READ_ONLY);
    status |= vxAccessImagePatch(grad_y, &rect, 0, &src_addr_y, (void **)&src_base_y, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    
    // Set up
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    float *X = (float*)malloc(width*height*sizeof(float));
    float *Y = (float*)malloc(width*height*sizeof(float));
    float *A = (float*)malloc(width*height*sizeof(float)*2); // Pairs of data
    float *B = (float*)malloc(width*height*sizeof(float)*2);
    float *C = (float*)malloc(width*height*sizeof(float));
    float *D = (float*)malloc(width*height*sizeof(float));
    
    // We know this should be u8 image data - but need to convert to float for op
    
    vx_uint8* x_data = vxFormatImagePatchAddress1d(src_base_x,0,&src_addr_x);
    vx_uint8* y_data = vxFormatImagePatchAddress1d(src_base_y,0,&src_addr_y);
    
    vDSP_vfltu8(x_data, 1, X, 1, width*height);
    vDSP_vfltu8(y_data, 1, Y, 1, width*height);
    
    // Copy so [X,Y] data is contiguous in A
    
    memcpy(&A[0],X,width*height*sizeof(float));
    memcpy(&A[width*height],Y,width*height*sizeof(float));
    
    // Do it!
    
    // Need to arrange in matrix then transpose to interleave elments.
    // Pass that through vDSP_polar, then transpose back to extract [mag,theta] 
    // See: https://developer.apple.com/library/mac/documentation/Accelerate/Reference/vDSPRef/#//apple_ref/doc/uid/TP40009464-CH13-SW5
    
    vDSP_mtrans(A, 1, B, 1, height*width, 2);
    vDSP_polar(B, 1, A, 1, height*width);
    vDSP_mtrans(A, 1, B, 1, 2, height*width);
    
    // Multiply by 128/pi and add 128 to normalise from [-pi,pi] to range [0,255] // TODO: check! I don't think this will round ok
    
    memcpy(C,&B[width*height],width*height*sizeof(float)); // Extract second values - thetas
    
    float norm = 128.0/M_PI;
    float shift = 128.0;
    
    vDSP_vsmsa(C, 1, &norm, &shift, D, 1, height*width);
    
    // Store normalised data into result
    
    vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
    
    vDSP_vfixu8(D, 1, dst_data, 1, width*height);
    
    // Free
    
    free(X); free(Y); free(A); free(B); free(C); free(D);
    
    // Commit
    
    status |= vxCommitImagePatch(grad_x, NULL, 0, &src_addr_x, src_base_x);
    status |= vxCommitImagePatch(grad_y, NULL, 0, &src_addr_y, src_base_y);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
    
    return status;
}

static vx_status VX_CALLBACK vxPhaseKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 3)
    {
        vx_image grad_x = (vx_image)parameters[0];
        vx_image grad_y = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        
        return vxPhase(grad_x, grad_y, output); // TODO:
    }
    return status;
}

static vx_status VX_CALLBACK vxPhaseInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0 || index == 1)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_S16)
            {
                if (index == 0)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    vx_parameter param0 = vxGetParameterByIndex(node, index);
                    vx_image input0 = 0;

                    vxQueryParameter(param0, VX_PARAMETER_ATTRIBUTE_REF, &input0, sizeof(input0));
                    if (input0)
                    {
                        vx_uint32 width0 = 0, height0 = 0, width1 = 0, height1 = 0;
                        vxQueryImage(input0, VX_IMAGE_ATTRIBUTE_WIDTH, &width0, sizeof(width0));
                        vxQueryImage(input0, VX_IMAGE_ATTRIBUTE_HEIGHT, &height0, sizeof(height0));
                        vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width1, sizeof(width1));
                        vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height1, sizeof(height1));

                        if (width0 == width1 && height0 == height1)
                            status = VX_SUCCESS;
                        vxReleaseImage(&input0);
                    }
                    vxReleaseParameter(&param0);
                }
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxPhaseOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, 0);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_uint32 width = 0, height = 0;
            vx_df_image format = 0;


            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            ptr->type = VX_TYPE_IMAGE;
            ptr->dim.image.format = VX_DF_IMAGE_U8;
            ptr->dim.image.width = width;
            ptr->dim.image.height = height;
            status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_param_description_t phase_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t phase_kernel = {
    VX_KERNEL_PHASE,
    "com.machineswithvision.openvx.phase",
    vxPhaseKernel,
    phase_kernel_params, dimof(phase_kernel_params),
    vxPhaseInputValidator,
    vxPhaseOutputValidator,
    NULL,
    NULL,
};

