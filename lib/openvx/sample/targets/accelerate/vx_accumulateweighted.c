/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxAccumulateWeighted(vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_float32 alpha = 0.0f;
    vxReadScalarValue(scalar, &alpha);
    
    vx_df_image inp_format = 0;
    vx_df_image acc_format = 0;
    vx_status status = VX_SUCCESS;
    
    vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &inp_format, sizeof(inp_format));
    vxQueryImage(accum, VX_IMAGE_ATTRIBUTE_FORMAT, &acc_format, sizeof(acc_format));
    
    // Access data...
    void* inp_ptr = NULL;
    void* acc_ptr = NULL;
    
    vx_rectangle_t rect;// = {0, 0, input->width, input->height};
    vx_imagepatch_addressing_t inp_addr, acc_addr;
    
    status = vxGetValidRegionImage(input, &rect);
    status |= vxAccessImagePatch(input, &rect, 0, &inp_addr, &inp_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    status |= vxGetValidRegionImage(accum, &rect);
    status |= vxAccessImagePatch(accum, &rect, 0, &acc_addr, &acc_ptr, VX_READ_AND_WRITE);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    // Convert internally/explicitly
    
    float *A = (float*)malloc(width*height*sizeof(float));
    float *B = (float*)malloc(width*height*sizeof(float));
    float *C = (float*)malloc(width*height*sizeof(float));
    float *D = (float*)malloc(width*height*sizeof(float));
    float *E = (float*)malloc(width*height*sizeof(float));
    
    // TODO: lots of stuff here to do with overflow and depending on image types
    if (inp_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* inp_data = vxFormatImagePatchAddress1d(inp_ptr,0,&inp_addr);
        vx_int16* acc_data = vxFormatImagePatchAddress1d(acc_ptr,0,&acc_addr);
        
        vDSP_vflt16(inp_data, 1, A, 1, width*height);
        vDSP_vflt16(acc_data, 1, B, 1, width*height);
    } // TODO: other input formats!
    
    // Do it!
    
    vx_float32 recip = (1.0 - alpha);
    
    vDSP_vsmul(A,1,&recip,C,1,width*height);
    vDSP_vsmul(B,1,&alpha,D,1,width*height);
    vDSP_vadd(C, 1, D, 1, E, 1, width*height);
    
    // Convert to output...
    
    if (acc_format == VX_DF_IMAGE_S16) // Should be!
    {
        vx_int16* acc_data = vxFormatImagePatchAddress1d(acc_ptr,0,&acc_addr);
        
        vDSP_vfix16(E, 1, acc_data, 1, width*height);
    }
    
    // Free extra memory used
    
    free(A);
    free(B);
    free(C);
    free(D);
    free(E);
    
    // Commit
    
    status |= vxCommitImagePatch(input, NULL, 0, &inp_addr, inp_ptr);
    status |= vxCommitImagePatch(accum, NULL, 0, &acc_addr, acc_ptr);
    
    return status;
    
    


}

static vx_status VX_CALLBACK vxAccumulateWeightedKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image input = (vx_image)parameters[0];
        vx_scalar scalar = (vx_scalar)parameters[1];
        vx_image accum = (vx_image)parameters[2];
        return vxAccumulateWeighted(input, scalar, accum); 
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxAccumulateWeightedInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0 )
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8)
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 2)
    {
        vx_image images[2];
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 2),
        };
        vxQueryParameter(param[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
        vxQueryParameter(param[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
        if (images[0] && images[1])
        {
            vx_uint32 width[2], height[2];
            vx_df_image format[2];

            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_FORMAT, &format[0], sizeof(format[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &format[1], sizeof(format[1]));
            if (width[0] == width[1] &&
               height[0] == height[1] &&
               format[0] == VX_DF_IMAGE_U8 &&
               format[1] == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    else if (index == 1) // only weighted/squared average
    {
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum type = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 alpha = 0.0f;
                    if ((vxReadScalarValue(scalar, &alpha) == VX_SUCCESS) &&
                        (0.0f <= alpha) && (alpha <= 1.0f))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxAccumulateWeightedOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    return status;
}

static vx_param_description_t accumulate_weighted_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_BIDIRECTIONAL, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t accumulate_weighted_kernel = {
    VX_KERNEL_ACCUMULATE_WEIGHTED,
    "com.machineswithvision.openvx.accumulate_weighted",
    vxAccumulateWeightedKernel,
    accumulate_weighted_kernel_params, dimof(accumulate_weighted_kernel_params),
    vxAccumulateWeightedInputValidator,
    vxAccumulateWeightedOutputValidator,
    NULL,
    NULL,
};
