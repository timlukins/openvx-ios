/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxMultiply(vx_image in0, vx_image in1, vx_scalar scale_param, vx_scalar opolicy_param, vx_scalar rpolicy_param, vx_image output)
{
    vx_float32 scale = 0.0f;
    vx_enum overflow_policy = -1;
    vx_enum rounding_policy = -1;
    //vx_uint32 y, x;
    //void *dst_base   = NULL;
    //void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_df_image in0_format = 0;
    vx_df_image in1_format = 0;
    vx_df_image out_format = 0;
    
    vx_status status = VX_FAILURE;
    
    vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &out_format, sizeof(out_format));
    vxQueryImage(in0, VX_IMAGE_ATTRIBUTE_FORMAT, &in0_format, sizeof(in0_format));
    vxQueryImage(in1, VX_IMAGE_ATTRIBUTE_FORMAT, &in1_format, sizeof(in1_format));
    
    status |= vxReadScalarValue(scale_param, &scale);
    status |= vxReadScalarValue(opolicy_param, &overflow_policy);
    status |= vxReadScalarValue(rpolicy_param, &rounding_policy);

    // Access data...
    void* in0_ptr = NULL;
    void* in1_ptr = NULL;
    void* out_ptr = NULL;
    
    //vx_rectangle_t rect;// = {0, 0, input->width, input->height};
    vx_imagepatch_addressing_t in0_addr, in1_addr, out_addr;
    
    status = vxGetValidRegionImage(in0, &rect);
    status |= vxAccessImagePatch(in0, &rect, 0, &in0_addr, &in0_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    status |= vxGetValidRegionImage(in1, &rect);
    status |= vxAccessImagePatch(in1, &rect, 0, &in1_addr, &in1_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    status |= vxGetValidRegionImage(output, &rect);
    status |= vxAccessImagePatch(output, &rect, 0, &out_addr, &out_ptr, VX_WRITE_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    // Convert internally/explicitly
    
    float *A = (float*)malloc(width*height*sizeof(float));
    float *B = (float*)malloc(width*height*sizeof(float));
    float *C = (float*)malloc(width*height*sizeof(float));
    
    // TODO: lots of stuff here to do with overflow and depending on image types
    if (in0_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* in0_data = vxFormatImagePatchAddress1d(in0_ptr,0,&in0_addr);
        vx_uint8* in1_data = vxFormatImagePatchAddress1d(in1_ptr,0,&in1_addr);
        
        vDSP_vfltu8(in0_data, 1, A, 1, width*height);
        vDSP_vfltu8(in1_data, 1, B, 1, width*height);
    } // TODO: other input formats!
    
    // Do it!
    
    vDSP_vmul(A, 1, B, 1, C, 1, width*height);
    
    // Convert to output...
    
    if (out_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* out_data = vxFormatImagePatchAddress1d(out_ptr,0,&out_addr);
        
        vDSP_vfixu8(C, 1, out_data, 1, width*height);
    }
    
    // Free extra memory used
    
    free(A);
    free(B);
    free(C);
    
    // Commit
    
    status |= vxCommitImagePatch(in0, NULL, 0, &in0_addr, in0_ptr);
    status |= vxCommitImagePatch(in1, NULL, 0, &in1_addr, in1_ptr);
    status |= vxCommitImagePatch(output, &rect, 0, &out_addr, out_ptr);
    
    return status;
}
    

static vx_status VX_CALLBACK vxMultiplyKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 6)
    {
        vx_image in0 = (vx_image)parameters[0];
        vx_image in1 = (vx_image)parameters[1];
        vx_scalar scale_param = (vx_scalar)parameters[2];
        vx_scalar opolicy_param = (vx_scalar)parameters[3];
        vx_scalar rpolicy_param = (vx_scalar)parameters[4];
        vx_image output = (vx_image)parameters[5];
        
        return vxMultiply(in0, in1, scale_param, opolicy_param, rpolicy_param, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxMultiplyInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format == VX_DF_IMAGE_U8 || format == VX_DF_IMAGE_S16)
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_image images[2];
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        vxQueryParameter(param[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
        vxQueryParameter(param[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
        if (images[0] && images[1])
        {
            vx_uint32 width[2], height[2];
            vx_df_image format1;

            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &format1, sizeof(format1));
            if (width[0] == width[1] && height[0] == height[1] &&
                (format1 == VX_DF_IMAGE_U8 || format1 == VX_DF_IMAGE_S16))
                status = VX_SUCCESS;
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    else if (index == 2)        // scale: must be non-negative.
    {
        vx_scalar scalar = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum type = -1;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 scale = 0.0f;
                    if ((vxReadScalarValue(scalar, &scale) == VX_SUCCESS) &&
                        (scale >= 0))
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
    else if (index == 3)        // overflow_policy: truncate or saturate.
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum overflow_policy = 0;
                    vxReadScalarValue(scalar, &overflow_policy);
                    if ((overflow_policy == VX_CONVERT_POLICY_WRAP) ||
                        (overflow_policy == VX_CONVERT_POLICY_SATURATE))
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
    else if (index == 4)        // rounding_policy: truncate or saturate.
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum rouding_policy = 0;
                    vxReadScalarValue(scalar, &rouding_policy);
                    if ((rouding_policy == VX_ROUND_POLICY_TO_ZERO) ||
                        (rouding_policy == VX_ROUND_POLICY_TO_NEAREST_EVEN))
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

static vx_status VX_CALLBACK vxMultiplyOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 5)
    {
        // * We need to look at both input images, but only for the format:
        // * if either is S16 or the output type is not U8, then it's S16.
        // * The geometry of the output image is copied from the first parameter:
        // * the input images are known to match from input parameters validation.

        vx_parameter param[] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
            vxGetParameterByIndex(node, index),
        };
        if (param[0] && param[1] && param[2])
        {
            vx_image images[3];
            vxQueryParameter(param[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
            vxQueryParameter(param[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
            vxQueryParameter(param[2], VX_PARAMETER_ATTRIBUTE_REF, &images[2], sizeof(images[2]));
            if (images[0] && images[1] && images[2])
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image informat[2] = {VX_DF_IMAGE_VIRT, VX_DF_IMAGE_VIRT};
                vx_df_image outformat = VX_DF_IMAGE_VIRT;

                // * When passing on the geometry to the output image, we only look at
                // * image 0, as both input images are verified to match, at input
                // * validation.
 
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_FORMAT, &informat[0], sizeof(informat[0]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &informat[1], sizeof(informat[1]));
                vxQueryImage(images[2], VX_IMAGE_ATTRIBUTE_FORMAT, &outformat, sizeof(outformat));

                if (informat[0] == VX_DF_IMAGE_U8 && informat[1] == VX_DF_IMAGE_U8 && outformat == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_SUCCESS;
                    outformat = VX_DF_IMAGE_S16;
                }

                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = outformat;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
                vxReleaseImage(&images[2]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
            vxReleaseParameter(&param[2]);
        }
    }
    return status;
}

static vx_param_description_t multiply_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};


vx_kernel_description_t multiply_kernel = {
    VX_KERNEL_MULTIPLY,
    "com.machineswithvision.openvx.multiply",
    vxMultiplyKernel,
    multiply_kernel_params, dimof(multiply_kernel_params),
    vxMultiplyInputValidator,
    vxMultiplyOutputValidator,
    NULL,
    NULL,
};