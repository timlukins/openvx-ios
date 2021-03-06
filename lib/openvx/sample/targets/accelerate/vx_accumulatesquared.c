/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxAccumulateSquare(vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_uint32 shift = 0u;
    vxReadScalarValue(scalar, &shift);
    
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
    
    // TODO: lots of stuff here to do with overflow and depending on image types
    if (inp_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* inp_data = vxFormatImagePatchAddress1d(inp_ptr,0,&inp_addr);
        vx_int16* acc_data = vxFormatImagePatchAddress1d(acc_ptr,0,&acc_addr);
        
        vDSP_vflt16(inp_data, 1, A, 1, width*height);
        vDSP_vflt16(acc_data, 1, B, 1, width*height);
    } // TODO: other input formats!
    
    // Do it!
    
    vDSP_vsq(A,1,C,1,width*height);
    vDSP_vadd(C, 1, B, 1, D, 1, width*height);
    
    // TODO: how to shift as in original...
    /*
     vx_uint8 *srcp = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
     vx_int16 *dstp = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
     vx_int32 res = ((vx_int32)(*srcp) * (vx_int32)(*srcp));
     res = ((vx_int32)*dstp) + (res >> shift);
     if (res > INT16_MAX) // saturate to S16
     res = INT16_MAX;
     *dstp = (vx_int16)(res);
     */
    
    // Convert to output...
    
    if (acc_format == VX_DF_IMAGE_S16) // Should be!
    {
        vx_int16* acc_data = vxFormatImagePatchAddress1d(acc_ptr,0,&acc_addr);
        
        vDSP_vfix16(D, 1, acc_data, 1, width*height);
    }
    
    // Free extra memory used
    
    free(A);
    free(B);
    free(C);
    free(D);
    
    // Commit
    
    status |= vxCommitImagePatch(input, NULL, 0, &inp_addr, inp_ptr);
    status |= vxCommitImagePatch(accum, NULL, 0, &acc_addr, acc_ptr);
    
    return status;
    
}


static vx_status VX_CALLBACK vxAccumulateSquareKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image input = (vx_image)parameters[0];
        vx_scalar scalar = (vx_scalar)parameters[1];
        vx_image accum = (vx_image)parameters[2];
        return vxAccumulateSquare(input, scalar, accum);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxAccumulateSquareInputValidator(vx_node node, vx_uint32 index)
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
               format[1] == VX_DF_IMAGE_S16)
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
        //status = VX_SUCCESS;
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum type = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_UINT32)
                {
                    vx_uint32 shift = 0u;
                    if ((vxReadScalarValue(scalar, &shift) == VX_SUCCESS) &&
                        (0 <= shift) && (shift <= 15))
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

static vx_status VX_CALLBACK vxAccumulateSquareOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    return status;
}

static vx_param_description_t accumulate_squared_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_BIDIRECTIONAL, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};


vx_kernel_description_t accumulate_square_kernel = {
    VX_KERNEL_ACCUMULATE_SQUARE,
    "com.machineswithvision.openvx.accumulate_square",
    vxAccumulateSquareKernel,
    accumulate_squared_kernel_params, dimof(accumulate_squared_kernel_params),
    vxAccumulateSquareInputValidator,
    vxAccumulateSquareOutputValidator,
    NULL,
    NULL,
};