/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

static vx_int16 sobel_x[] = {
    -1, 0, +1,
    -2, 0, +2,
    -1, 0, +1,
};

static vx_int16 sobel_y[] = {
    -1, -2, -1,
     0,  0,  0,
    +1, +2, +1,
};

vx_status vxConvolution3x3(vx_image src, vx_image dst, vx_int16 conv[], const vx_border_mode_t *borders)
{
    //vx_uint32 y, x;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_enum dst_format = VX_DF_IMAGE_VIRT;
    vx_status status = VX_SUCCESS;
    vx_uint32 low_x = 0, low_y = 0, high_x, high_y;
    
    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    status |= vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_FORMAT, &dst_format, sizeof(dst_format));
    
    high_x = src_addr.dim_x;
    high_y = src_addr.dim_y;
    
    if (borders->mode == VX_BORDER_MODE_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }
    // TODO: better check/use modified rectangle here...
    
    vImage_Buffer input = {
        .data = src_base,
        .height = src->height,
        .width = src->width,
        .rowBytes = sizeof(vx_uint8)*src->width
    };
    
    vImage_Buffer output = {
        .data = dst_base,
        .height = dst->height,
        .width = dst->width,
        .rowBytes = sizeof(vx_uint8)*dst->width // TODO: this might have to be s16 format?
    };
    
    vImage_Error result;
    
    result = vImageConvolve_Planar8(&input, &output, NULL, 0, 0, conv, 3, 3, 16.0, 0, kvImageBackgroundColorFill);
    if(result != kvImageNoError)
    {
        VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to do sobel convolve on planar 8\n");
        return VX_FAILURE;
    }
    
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    return status;
}


// nodeless version of the Sobel3x3 kernel
vx_status vxSobel3x3(vx_image input, vx_image grad_x, vx_image grad_y, vx_border_mode_t *bordermode)
{
    if (grad_x) {
        vx_status status = vxConvolution3x3(input, grad_x, sobel_x, bordermode);
        if (status != VX_SUCCESS) return status;
    }
    
    if (grad_y) {
        vx_status status = vxConvolution3x3(input, grad_y, sobel_y, bordermode);
        if (status != VX_SUCCESS) return status;
    }

    return VX_SUCCESS;
}

static vx_status VX_CALLBACK vxSobel3x3Kernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_border_mode_t bordermode;
        vx_image input  = (vx_image)parameters[0];
        vx_image grad_x = (vx_image)parameters[1];
        vx_image grad_y = (vx_image)parameters[2];
        status = vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxSobel3x3(input, grad_x, grad_y, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxSobelInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_image input = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
        if (input)
        {
            vx_uint32 width = 0, height = 0;
            vx_df_image format = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (width >= 3 && height >= 3 && format == VX_DF_IMAGE_U8)
                status = VX_SUCCESS;
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxSobelOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1 || index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); // * we reference the input image
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
                ptr->dim.image.format = VX_DF_IMAGE_S16;
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

static vx_param_description_t gradient_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t sobel3x3_kernel = {
    VX_KERNEL_SOBEL_3x3,
    "com.machineswithvision.openvx.sobel3x3",
    vxSobel3x3Kernel,
    gradient_kernel_params, dimof(gradient_kernel_params),
    vxSobelInputValidator,
    vxSobelOutputValidator,
    NULL,
    NULL,
};