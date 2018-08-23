/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxDilate3x3(vx_image src, vx_image dst, vx_border_mode_t *borders)
{
    vx_uint32 low_y = 0, low_x = 0, high_y, high_x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    
    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
    /* TODO: handle border mode... might be related to offset ROI in dilate call
    high_y = src_addr.dim_y;
    high_x = src_addr.dim_x;
    
    if (borders->mode == VX_BORDER_MODE_UNDEFINED)
    {
        //shrink by 1
        vxAlterRectangle(&rect, 1, 1, -1, -1);
        low_x += 1; high_x -= 1;
        low_y += 1; high_y -= 1;
    }
    */
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    vImage_Buffer srcVimg = {
        .data = src_base,
        .height = height,
        .width = width,
        .rowBytes = sizeof(vx_uint8)*width
    };
    
    vImage_Buffer dstVimg = {
        .data = dst_base,
        .height = height,
        .width = width,
        .rowBytes = sizeof(vx_uint8)*width
    };

    // Kernel (structuring element) to use (3x3 rectangle in this case)
    
    uint8_t kernel[] = {
        1,1,1,
        1,1,1,
        1,1,1,
    };
    
    // Do it!
    
    vImage_Error result;
    
    result = vImageDilate_Planar8(&srcVimg, &dstVimg, 0, 0, kernel, 3, 3, kvImageNoFlags);
    
    if(result != kvImageNoError)
    {
        VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to combined 3 r,g,b to RGB image\n");
        return VX_FAILURE;
    }

    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    
    return status;

}


static vx_status VX_CALLBACK vxDilate3x3Kernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 2)
    {
        vx_border_mode_t bordermode;
        vx_image src = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        status = vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxDilate3x3(src, dst, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxDilateInputValidator(vx_node node, vx_uint32 index)
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

static vx_status VX_CALLBACK vxDilateOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
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
                ptr->dim.image.format = VX_DF_IMAGE_U8;
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

static vx_param_description_t dilate_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};


vx_kernel_description_t dilate3x3_kernel = {
    VX_KERNEL_DILATE_3x3,
    "org.khronos.openvx.dilate3x3",
    vxDilate3x3Kernel,
    dilate_kernel_params, dimof(dilate_kernel_params),
    vxDilateInputValidator,
    vxDilateOutputValidator,
    NULL,
    NULL,
};

