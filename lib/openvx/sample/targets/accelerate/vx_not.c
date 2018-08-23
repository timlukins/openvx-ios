/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxNot(vx_image input, vx_image output)
{
    vx_uint32 width = 0, height = 0;
    void *dst_base = NULL;
    void *src_base = NULL;
    vx_imagepatch_addressing_t dst_addr, src_addr;
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;
    
    status = vxGetValidRegionImage(input, &rect);
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    
    height = src_addr.dim_y;
    width = src_addr.dim_x;
    
    // We should only be dealing with U8 data here...
    vx_uint8* in_data = vxFormatImagePatchAddress1d(src_base,0,&src_addr);
    
    vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
    
    // Iterate in reverse and include decrement for speed
    for (unsigned long i = width*height; i-- > 0; )
        dst_data[i] = ~in_data[i];
    
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
    
    return status;
}


static vx_status VX_CALLBACK vxNotKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 2)
    {
        vx_image input = (vx_image)parameters[0];
        vx_image output = (vx_image)parameters[1];
        
        return vxNot(input, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}


// * The Not kernel is an unary operator, requiring separate validators.

static vx_status VX_CALLBACK vxNotInputValidator(vx_node node, vx_uint32 index)
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
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxNotOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image inimage = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &inimage, sizeof(inimage));
            if (inimage)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(inimage, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(inimage, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&inimage);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t not_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};




vx_kernel_description_t not_kernel = {
    VX_KERNEL_NOT,
    "com.machineswithvision.openvx.not",
    vxNotKernel,
    not_kernel_params, dimof(not_kernel_params),
    vxNotInputValidator,
    vxNotOutputValidator,
    NULL,
    NULL,
};
