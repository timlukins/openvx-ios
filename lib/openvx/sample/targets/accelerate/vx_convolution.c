/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxConvolve(vx_image src, vx_convolution conv, vx_image dst, vx_border_mode_t *bordermode)
{
    //vx_int32 y, x, i;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_rectangle_t rect;
    vx_size conv_width, conv_height;
    vx_int32 conv_radius_x, conv_radius_y;
    //vx_int16 conv_mat[C_MAX_CONVOLUTION_DIM * C_MAX_CONVOLUTION_DIM] = {0};
    vx_int16 conv_mat[13*13] = {0};
    //vx_int32 sum = 0, value = 0;
    vx_uint32 scale = 1;
    vx_df_image src_format = 0;
    vx_df_image dst_format = 0;
    vx_status status  = VX_SUCCESS;
    vx_int32 low_x, low_y, high_x, high_y;
    
    // Access and setup vImages
    status |= vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &src_format, sizeof(src_format));
    status |= vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_FORMAT, &dst_format, sizeof(dst_format));
    status |= vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
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
        .rowBytes = sizeof(vx_uint8)*dst->width
    };
    
    // Access convolution
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ATTRIBUTE_COLUMNS, &conv_width, sizeof(conv_width));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ATTRIBUTE_ROWS, &conv_height, sizeof(conv_height));
    status |= vxQueryConvolution(conv, VX_CONVOLUTION_ATTRIBUTE_SCALE, &scale, sizeof(scale));
    conv_radius_x = (vx_int32)conv_width / 2;
    conv_radius_y = (vx_int32)conv_height / 2;
    status |= vxReadConvolutionCoefficients(conv, conv_mat);
    
    // Tweak offset depending on bordermode...
    if (bordermode->mode == VX_BORDER_MODE_UNDEFINED)
    {
        low_x = conv_radius_x;
        high_x = ((src_addr.dim_x >= (vx_uint32)conv_radius_x) ? src_addr.dim_x - conv_radius_x : 0);
        low_y = conv_radius_y;
        high_y = ((src_addr.dim_y >= (vx_uint32)conv_radius_y) ? src_addr.dim_y - conv_radius_y : 0);
        vxAlterRectangle(&rect, conv_radius_x, conv_radius_y, -conv_radius_x, -conv_radius_y);
    }
    else
    {
        low_x = 0;
        high_x = src_addr.dim_x;
        low_y = 0;
        high_y = src_addr.dim_y;
    }
    
    // Do convoultion
    vImage_Error result;
    
    result = vImageConvolve_Planar8(&input, &output, NULL, low_x, low_y, conv_mat, conv_height, conv_width, 0, 0, kvImageBackgroundColorFill);
    
    if(result != kvImageNoError)
    {
        VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to do image convolve on planar 8\n");
        return VX_FAILURE;
    }

    // Write results
    status |= vxWriteConvolutionCoefficients(conv, NULL);
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    
    return status;
}

static vx_status VX_CALLBACK vxConvolveKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_border_mode_t bordermode;
        vx_image       src  = (vx_image)parameters[0];
        vx_convolution conv = (vx_convolution)parameters[1];
        vx_image       dst  = (vx_image)parameters[2];
        status = vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxConvolve(src, conv, dst, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxConvolveInputValidator(vx_node node, vx_uint32 index)
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
            vx_uint32 width = 0, height = 0;
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if ((width > VX_INT_MAX_CONVOLUTION_DIM) &&
                (height > VX_INT_MAX_CONVOLUTION_DIM) &&
                ((format == VX_DF_IMAGE_U8)
#if defined(EXPERIMENTAL_USE_S16)
                 || (format == VX_DF_IMAGE_S16)
#endif
                 ))
            {
                status = VX_SUCCESS;
            }

            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    if (index == 1)
    {
        vx_convolution conv = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &conv, sizeof(conv));
        if (conv)
        {
            vx_df_image dims[2] = {0,0};
            vxQueryConvolution(conv, VX_CONVOLUTION_ATTRIBUTE_COLUMNS, &dims[0], sizeof(dims[0]));
            vxQueryConvolution(conv, VX_CONVOLUTION_ATTRIBUTE_ROWS, &dims[1], sizeof(dims[1]));
            if ((dims[0] <= VX_INT_MAX_CONVOLUTION_DIM) &&
                (dims[1] <= VX_INT_MAX_CONVOLUTION_DIM))
            {
                status = VX_SUCCESS;
            }

            vxReleaseConvolution(&conv);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxConvolveOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter params[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, index),
        };
        if (params[0] && params[1])
        {
            vx_image input = 0;
            vx_image output = 0;
            vxQueryParameter(params[0], VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            vxQueryParameter(params[1], VX_PARAMETER_ATTRIBUTE_REF, &output, sizeof(output));
            if (input && output)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = 0;
                vx_df_image output_format = 0;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));

                vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &output_format, sizeof(output_format));

                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = output_format == VX_DF_IMAGE_U8 ? VX_DF_IMAGE_U8 : VX_DF_IMAGE_S16;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;

                vxReleaseImage(&input);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&params[0]);
            vxReleaseParameter(&params[1]);
        }
    }
    return status;
}

static vx_param_description_t convolution_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_CONVOLUTION, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t convolution_kernel = {
    VX_KERNEL_CUSTOM_CONVOLUTION,
    "com.machineswithvision.openvx.custom_convolution",
    vxConvolveKernel,
    convolution_kernel_params, dimof(convolution_kernel_params),
    vxConvolveInputValidator,
    vxConvolveOutputValidator,
    NULL,
    NULL,
};
