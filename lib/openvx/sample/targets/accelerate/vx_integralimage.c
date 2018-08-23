/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxIntegralImage(vx_image src, vx_image dst)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    
    vx_status status = VX_SUCCESS;
    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
    for (y = 0; (y < src_addr.dim_y) && (status == VX_SUCCESS); y++)
    {
        vx_uint8 *pixels = vxFormatImagePatchAddress2d(src_base, 0, y, &src_addr);
        vx_uint32 *sums = vxFormatImagePatchAddress2d(dst_base, 0, y, &dst_addr);
        
        if (y == 0)
        {
            sums[0] = pixels[0];
            for (x = 1; x < src_addr.dim_x; x++)
            {
                sums[x] = sums[x-1] + pixels[x];
            }
        }
        else
        {
            vx_uint32 *prev_sums = vxFormatImagePatchAddress2d(dst_base, 0, y-1, &dst_addr);
            sums[0] = prev_sums[0] + pixels[0];
            for (x = 1; x < src_addr.dim_x; x++)
            {
                sums[x] = pixels[x] + sums[x-1] + prev_sums[x] - prev_sums[x-1];
            }
        }
    }
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    
    return status;
}

static vx_status VX_CALLBACK vxIntegralImageKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 2)
    {
        vx_image src = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        
        return vxIntegralImage(src, dst);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxIntegralInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxIntegralOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); // we reference the input image
        if (param)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if (input)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U32;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t integral_image_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t integral_image_kernel = {
    VX_KERNEL_INTEGRAL_IMAGE,
    "com.machineswithvision.openvx.integral_image",
    vxIntegralImageKernel,
    integral_image_kernel_params, dimof(integral_image_kernel_params),
    vxIntegralInputValidator,
    vxIntegralOutputValidator,
    NULL,
    NULL,
};

