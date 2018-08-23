/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxConvertDepth(vx_image input, vx_image output, vx_scalar spol, vx_scalar sshf)
{
    vx_uint32 y, x;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_enum format[2];
    vx_enum policy = 0;
    vx_int32 shift = 0;
    
    vx_status status = VX_SUCCESS;
    
    status |= vxReadScalarValue(spol, &policy);
    status |= vxReadScalarValue(sshf, &shift);
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format[0], sizeof(format[0]));
    status |= vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &format[1], sizeof(format[1]));
    status |= vxGetValidRegionImage(input, &rect);
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
    // Dimensions...
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    // This is an up-conversion...
    if ((format[0] == VX_DF_IMAGE_U8) && (format[1] == VX_DF_IMAGE_S16))
    {
        // Do this safely by converting to float then back...
        vx_uint8* src_data = vxFormatImagePatchAddress1d(src_base,0,&src_addr);

        float* A = (float*)malloc(width*height*sizeof(float));
        
        vDSP_vfltu8(src_data, 1, A, 1, width*height);
        
        vx_int16* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
        
        vDSP_vfix16(A, 1, dst_data, 1, width*height);

        free(A);
        
    }
    // This is down-conversion... (need to respect shift)
    else if ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U8))
    {
        // Go the otherway
        vx_uint16* src_data = vxFormatImagePatchAddress1d(src_base,0,&src_addr);
        
        float* A = (float*)malloc(width*height*sizeof(float));
        
        vDSP_vfltu16(src_data, 1, A, 1, width*height);
        
        vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
        
        vDSP_vfixu8(A, 1, dst_data, 1, width*height);
        
        free(A);
        
        // TODO: really need to work out the application of the shift in response to the policy...
        // 0 <= shift < 8;
        
        // Should use cliping or limit functions first on data before conversion?
        // https://developer.apple.com/library/prerelease/ios/documentation/Accelerate/Reference/vDSPRef/index.html#//apple_ref/doc/uid/TP40009464-CH13-SW11
        
        if (policy == VX_CONVERT_POLICY_WRAP)
        {
            //*dst = (vx_uint8)((*src) >> shift);
        }
        else if (policy == VX_CONVERT_POLICY_SATURATE)
        {
            //vx_uint16 value = (*src) >> shift;
            //value = (value > UINT8_MAX ? UINT8_MAX : value);
            //*dst = (vx_uint8)value;
        }
        
    }
    // TODO: other formants once not EXPERIMENTAL in spec...
    
    
    return status;
}

static vx_status VX_CALLBACK vxConvertDepthKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  input  = (vx_image) parameters[0];
        vx_image  output = (vx_image) parameters[1];
        vx_scalar spol   = (vx_scalar)parameters[2];
        vx_scalar sshf   = (vx_scalar)parameters[3];
        
        return vxConvertDepth(input, output, spol, sshf);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxConvertDepthInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image input = 0;
            status = vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if ((status == VX_SUCCESS) && input)
            {
                vx_df_image format = 0;
                status = vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if ((status != VX_SUCCESS) ||
                    (format == VX_DF_IMAGE_U8)  ||
#if defined(EXPERIMENTAL_USE_S16)
                    (format == VX_DF_IMAGE_U16) ||
                    (format == VX_DF_IMAGE_U32) ||
                    (format == VX_DF_IMAGE_S32) ||
                    (format == VX_DF_IMAGE_F32) ||
#endif
                    (format == VX_DF_IMAGE_S16))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 2)
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
                        printf("Overflow given as %08x\n", overflow_policy);
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
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar scalar = 0;
            status = vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (status == VX_SUCCESS)
            {
                vx_enum type = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_INT32)
                {
                    vx_int32 shift = 0;
                    status = vxReadScalarValue(scalar, &shift);
                    if (status == VX_SUCCESS)
                    {
                        // *! \internal Allowing \f$ 0 \le shift < 32 \f$ could
                        // * produce weird results for smaller bit depths
                        if (shift < 0 || shift >= 32)
                        {
                            status = VX_ERROR_INVALID_VALUE;
                        }
                        // * status should be VX_SUCCESS from call
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

static vx_status VX_CALLBACK vxConvertDepthOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        if (param[0] && param[1])
        {
            vx_image images[2] = {0,0};
            status  = VX_SUCCESS;
            status |= vxQueryParameter(param[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
            status |= vxQueryParameter(param[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
            if ((status == VX_SUCCESS) && (images[0]) && (images[1]))
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format[2] = {VX_DF_IMAGE_VIRT, VX_DF_IMAGE_VIRT};
                status |= vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                status |= vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                status |= vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_FORMAT, &format[0], sizeof(format[0]));
                status |= vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &format[1], sizeof(format[1]));
                if (((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_S16)) ||
#if defined(EXPERIMENTAL_USE_S16)
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_U16)) ||
                    ((format[0] == VX_DF_IMAGE_U8)  && (format[1] == VX_DF_IMAGE_U32)) ||
                    ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U8))  ||
                    ((format[0] == VX_DF_IMAGE_U16) && (format[1] == VX_DF_IMAGE_U32)) ||
                    ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_S32)) ||
                    ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U8))  ||
                    ((format[0] == VX_DF_IMAGE_U32) && (format[1] == VX_DF_IMAGE_U16)) ||
                    ((format[0] == VX_DF_IMAGE_S32) && (format[1] == VX_DF_IMAGE_S16)) ||
                    ((format[0] == VX_DF_IMAGE_F32) && (format[1] == VX_DF_IMAGE_U8))  || // non-specification
#endif
                    ((format[0] == VX_DF_IMAGE_S16) && (format[1] == VX_DF_IMAGE_U8)))
                {
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = format[1];
                    ptr->dim.image.width = width;
                    ptr->dim.image.height = height;
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
        }
    }
    return status;
}

static vx_param_description_t convertdepth_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR,  VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR,  VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t convertdepth_kernel = {
    VX_KERNEL_CONVERTDEPTH,
    "com.machineswithvision.openvx.convert_depth",
    vxConvertDepthKernel,
    convertdepth_kernel_params, dimof(convertdepth_kernel_params),
    vxConvertDepthInputValidator,
    vxConvertDepthOutputValidator,
    NULL,
    NULL,
};
