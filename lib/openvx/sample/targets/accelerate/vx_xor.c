/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxXor(vx_image in1, vx_image in2, vx_image output)
{
    vx_uint32 width = 0, height = 0;
    void *dst_base   = NULL;
    void *src_base[2] = {NULL, NULL};
    vx_imagepatch_addressing_t dst_addr, src_addr[2];
    vx_rectangle_t rect;
    vx_status status = VX_SUCCESS;
    
    status = vxGetValidRegionImage(in1, &rect);
    status |= vxAccessImagePatch(in1, &rect, 0, &src_addr[0], (void **)&src_base[0], VX_READ_ONLY);
    status |= vxAccessImagePatch(in2, &rect, 0, &src_addr[1], (void **)&src_base[1], VX_READ_ONLY);
    status |= vxAccessImagePatch(output, &rect, 0, &dst_addr, (void **)&dst_base, VX_WRITE_ONLY);
    
    width = src_addr[0].dim_x;
    height = src_addr[0].dim_y;
    
    // We should only be dealing with U8 data here...
    vx_uint8* in0_data = vxFormatImagePatchAddress1d(src_base[0],0,&src_addr[0]);
    vx_uint8* in1_data = vxFormatImagePatchAddress1d(src_base[0],0,&src_addr[0]);
    
    vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
    
    // copy in0 into dst so operation is performed in array...
    
    memcpy(dst_data,in0_data,width*height*sizeof(vx_uint8));
    
    // Iterate in reverse and include decrement for speed
    for (unsigned long i = width*height; i-- > 0; )
        dst_data[i] ^= in1_data[i];
    
    status |= vxCommitImagePatch(in1, NULL, 0, &src_addr[0], src_base[0]);
    status |= vxCommitImagePatch(in2, NULL, 0, &src_addr[1], src_base[1]);
    status |= vxCommitImagePatch(output, &rect, 0, &dst_addr, dst_base);
    
    return status;
    
}

static vx_status VX_CALLBACK vxXorKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image in1 = (vx_image)parameters[0];
        vx_image in2 = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        
        return vxXor(in1, in2, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxXorInputValidator(vx_node node, vx_uint32 index)
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
            vx_df_image format[2];

            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width[0], sizeof(width[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_WIDTH, &width[1], sizeof(width[1]));
            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[0], sizeof(height[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[1], sizeof(height[1]));
            vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_FORMAT, &format[0], sizeof(format[0]));
            vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &format[1], sizeof(format[1]));
            if (width[0] == width[1] && height[0] == height[1] && format[0] == format[1])
                status = VX_SUCCESS;
            vxReleaseImage(&images[1]);
            vxReleaseImage(&images[0]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    return status;
}

static vx_status VX_CALLBACK vxXorOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        if (param0)
        {
            vx_image image0 = 0;
            vxQueryParameter(param0, VX_PARAMETER_ATTRIBUTE_REF, &image0, sizeof(image0));
            
             // * When passing on the geometry to the output image, we only look at image 0, as
             // * both input images are verified to match, at input validation.
            
            if (image0)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(image0, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(image0, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&image0);
            }
            vxReleaseParameter(&param0);
        }
    }
    return status;
}

static vx_param_description_t xor_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};


vx_kernel_description_t xor_kernel = {
    VX_KERNEL_XOR,
    "com.machineswithvision.openvx.xor",
    vxXorKernel,
    xor_kernel_params, dimof(xor_kernel_params),
    vxXorInputValidator,
    vxXorOutputValidator,
    NULL,
    NULL,
};
