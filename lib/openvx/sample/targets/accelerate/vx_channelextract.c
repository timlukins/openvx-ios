/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

static vx_status vxCopyPlaneToImage(vx_image src,
                                    vx_uint32 src_plane,
                                    vx_uint8 src_component,
                                    vx_uint32 x_subsampling,
                                    vx_image dst)
{
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr = {0};
    vx_imagepatch_addressing_t dst_addr = {0};
    vx_rectangle_t src_rect, dst_rect;
    vx_uint32 x, y;
    vx_status status = VX_SUCCESS;
    
    status = vxGetValidRegionImage(src, &src_rect);
    status |= vxAccessImagePatch(src, &src_rect, src_plane, &src_addr, &src_base, VX_READ_ONLY);
    
    if (status == VX_SUCCESS)
    {
        // TODO: check this subsampling makes sense?
        
        dst_rect = src_rect;
        //dst_rect.start_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        //dst_rect.start_y /= VX_SCALE_UNITY / src_addr.scale_y;
        //dst_rect.end_x /= VX_SCALE_UNITY / src_addr.scale_x * x_subsampling;
        //dst_rect.end_y /= VX_SCALE_UNITY / src_addr.scale_y;
        
        status = vxAccessImagePatch(dst, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
        
        if (status == VX_SUCCESS)
        {
            // Set up vImage buffers...
            
            vImage_Buffer rgbSrc = {
                .data = src_base,
                .height = src_rect.end_y-src_rect.start_y,
                .width = src_rect.end_x-src_rect.start_x,
                .rowBytes = sizeof(vx_uint8)*(src_rect.end_x-src_rect.start_x)
            };
            
            vImage_Buffer planeDest = {
                .data = dst_base,
                .height = dst_rect.end_y-dst_rect.start_y, // TODO: assumes input tests confirm same dimensions!
                .width = dst_rect.end_x-dst_rect.start_x,
                .rowBytes = sizeof(vx_uint8)*(dst_rect.end_x-dst_rect.start_x)
            };
            
            vImage_Error result;
            
            result = vImageExtractChannel_ARGB8888(&rgbSrc, &planeDest, src_plane, kvImageNoFlags);
             
            if(result != kvImageNoError)
            {
                VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to combined 3 r,g,b to RGB image\n");
                return VX_FAILURE;
            }
            
            vxCommitImagePatch(dst, &dst_rect, 0, &dst_addr, dst_base);
        }
        vxCommitImagePatch(src, NULL, src_plane, &src_addr, src_base);
    }
    return status;
}

vx_status vxChannelExtract(vx_image src, vx_scalar channel, vx_image dst)
{
    vx_enum chan = -1;
    vx_df_image format = 0;
    vx_uint32 cidx = 0;
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    
    vxReadScalarValue(channel, &chan);
    vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    
    switch (format)
    {
        case VX_DF_IMAGE_RGB:
        case VX_DF_IMAGE_RGBX:
            switch (chan)
            {
                case VX_CHANNEL_R:
                    cidx = 0; break;
                case VX_CHANNEL_G:
                    cidx = 1; break;
                case VX_CHANNEL_B:
                    cidx = 2; break;
                case VX_CHANNEL_A:
                    cidx = 3; break;
                default:
                    return VX_ERROR_INVALID_PARAMETERS;
            }
            status = vxCopyPlaneToImage(src, 0, cidx, 1, dst);
            break;
        case VX_DF_IMAGE_NV12:
        case VX_DF_IMAGE_NV21:
            if (chan == VX_CHANNEL_Y)
                status = vxCopyPlaneToImage(src, 0, 0, 1, dst);
            else if ((chan == VX_CHANNEL_U && format == VX_DF_IMAGE_NV12) ||
                     (chan == VX_CHANNEL_V && format == VX_DF_IMAGE_NV21))
                status = vxCopyPlaneToImage(src, 1, 0, 1, dst);
            else
                status = vxCopyPlaneToImage(src, 1, 1, 1, dst);
            break;
        case VX_DF_IMAGE_IYUV:
        case VX_DF_IMAGE_YUV4:
            switch (chan)
            {
                case VX_CHANNEL_Y:
                    cidx = 0; break;
                case VX_CHANNEL_U:
                    cidx = 1; break;
                case VX_CHANNEL_V:
                    cidx = 2; break;
                default:
                    return VX_ERROR_INVALID_PARAMETERS;
            }
            status = vxCopyPlaneToImage(src, cidx, 0, 1, dst);
            break;
        case VX_DF_IMAGE_YUYV:
        case VX_DF_IMAGE_UYVY:
            if (chan == VX_CHANNEL_Y)
                status = vxCopyPlaneToImage(src, 0, format == VX_DF_IMAGE_YUYV ? 0 : 1, 1, dst);
            else
                status = vxCopyPlaneToImage(src, 0,
                                            (format == VX_DF_IMAGE_YUYV ? 1 : 0) + (chan == VX_CHANNEL_U ? 0 : 2),
                                            2, dst);
            break;
    }
    return status;
}

static vx_status VX_CALLBACK vxChannelExtractKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image src = (vx_image)parameters[0];
        vx_scalar channel = (vx_scalar)parameters[1];
        vx_image dst = (vx_image)parameters[2];
        
        return vxChannelExtract(src, channel, dst);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxChannelExtractInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_SUCCESS;
    vx_parameter param = vxGetParameterByIndex(node, index);
    if (index == 0)
    {
        vx_image image = 0;
        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));
        if (image)
        {
            vx_df_image format = 0;
            vx_uint32 width, height;
            vxQueryImage(image, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            vxQueryImage(image, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
            vxQueryImage(image, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
            // check to make sure the input format is supported.
            switch (format)
            {
                case VX_DF_IMAGE_RGB:
                case VX_DF_IMAGE_RGBX:
                case VX_DF_IMAGE_YUV4:
                    status = VX_SUCCESS;
                    break;
                // 4:2:0
                case VX_DF_IMAGE_NV12:
                case VX_DF_IMAGE_NV21:
                case VX_DF_IMAGE_IYUV:
                    if (width % 2 != 0 || height % 2 != 0)
                        status = VX_ERROR_INVALID_DIMENSION;
                    else
                        status = VX_SUCCESS;
                    break;
                // 4:2:2
                case VX_DF_IMAGE_UYVY:
                case VX_DF_IMAGE_YUYV:
                    if (width % 2 != 0)
                        status = VX_ERROR_INVALID_DIMENSION;
                    else
                        status = VX_SUCCESS;
                    break;
                default:
                    status = VX_ERROR_INVALID_FORMAT;
                    break;
            }
            vxReleaseImage(&image);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else if (index == 1)
    {
        vx_scalar scalar;
        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
        if (scalar)
        {
            vx_enum type = 0;
            vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
            if (type == VX_TYPE_ENUM)
            {
                vx_enum channel = 0;
                vx_parameter param0;

                vxReadScalarValue(scalar, &channel);
                param0 = vxGetParameterByIndex(node, 0);

                if (param0)
                {
                    vx_image image = 0;
                    vxQueryParameter(param0, VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));

                    if (image)
                    {
                        vx_df_image format = VX_DF_IMAGE_VIRT;
                        vx_enum max_channel;
                        vxQueryImage(image, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                        // TODO: this only works for RGBX image formats?????
                        max_channel = (format == VX_DF_IMAGE_RGBX ? VX_CHANNEL_3 : VX_CHANNEL_2);
                        // Take out for the moment.
                        //if (VX_CHANNEL_0 <= channel && channel <= max_channel)
                        //{
                            status = VX_SUCCESS;
                        //}
                        //else
                        //{
                        //    status = VX_ERROR_INVALID_VALUE;
                        //}

                        vxReleaseImage(&image);
                    }

                    vxReleaseParameter(&param0);
                }
            }
            else
            {
                status = VX_ERROR_INVALID_TYPE;
            }
            vxReleaseScalar(&scalar);
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    vxReleaseParameter(&param);
    return status;
}

static vx_status VX_CALLBACK vxChannelExtractOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, 1);

        if ((param0) && (param1))
        {
            vx_image input = 0;
            vx_scalar chan = 0;
            vx_enum channel = 0;
            vxQueryParameter(param0, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            vxQueryParameter(param1, VX_PARAMETER_ATTRIBUTE_REF, &chan, sizeof(chan));
            vxReadScalarValue(chan, &channel);

            if ((input) && (chan))
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT;

                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                if (channel != VX_CHANNEL_0)
                    switch (format) {
                        case VX_DF_IMAGE_IYUV:
                        case VX_DF_IMAGE_NV12:
                        case VX_DF_IMAGE_NV21:
                            width /= 2;
                            height /= 2;
                            break;
                        case VX_DF_IMAGE_YUYV:
                        case VX_DF_IMAGE_UYVY:
                            width /= 2;
                            break;
                    }

                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&input);
                vxReleaseScalar(&chan);
            }
            vxReleaseParameter(&param0);
            vxReleaseParameter(&param1);
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    VX_PRINT(VX_ZONE_API, "%s:%u returned %d\n", __FUNCTION__, index, status);
    return status;
}


static vx_param_description_t channel_extract_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

 vx_kernel_description_t channelextract_kernel = {
    VX_KERNEL_CHANNEL_EXTRACT,
    "com.machineswithvision.openvx.channel_extract",
    vxChannelExtractKernel,
    channel_extract_kernel_params, dimof(channel_extract_kernel_params),
    vxChannelExtractInputValidator,
    vxChannelExtractOutputValidator,
    NULL,
    NULL,
};
