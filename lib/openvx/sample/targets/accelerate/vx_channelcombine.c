/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxChannelCombine(vx_image inputs[4], vx_image output)
{
    vx_df_image format = 0;
    vx_rectangle_t rect;
    vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    vxGetValidRegionImage(inputs[0], &rect);
    if ((format == VX_DF_IMAGE_RGB) || (format == VX_DF_IMAGE_RGBX))
    {
        /* write all the channels back out in interleaved format */
        vx_imagepatch_addressing_t src_addrs[4];
        vx_imagepatch_addressing_t dst_addr;
        void *base_src_ptrs[4] = {NULL, NULL, NULL, NULL};
        void *base_dst_ptr = NULL;
        uint32_t p;
        uint32_t numplanes = 3;
        
        if (format == VX_DF_IMAGE_RGBX)
        {
            numplanes = 4;
        }
        
        // get the planes
        for (p = 0; p < numplanes; p++)
        {
            vxAccessImagePatch(inputs[p], &rect, 0, &src_addrs[p], &base_src_ptrs[p], VX_READ_ONLY);
        }
        
        int width  = rect.end_x - rect.start_x;
        int height = rect.end_y - rect.start_y;
        
        // And the output img...
        vxAccessImagePatch(output, &rect, 0, &dst_addr, &base_dst_ptr, VX_WRITE_ONLY);
        
        // Set up vImage buffers...
        
        vImage_Buffer planarRed = {
            .data = base_src_ptrs[0],
            .height = height,
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width
        };
        
        vImage_Buffer planarGreen = {
            .data = base_src_ptrs[1],
            .height = height,
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width
        };
        
        vImage_Buffer planarBlue = {
            .data = base_src_ptrs[2],
            .height = height,
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width
        };
        
        vImage_Buffer rgbDest = {
            .data = base_dst_ptr,
            .height = height, // TODO: assumes input tests confirm same dimensions!
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width*numplanes// times number of planes
        };
        
        // Convert it!
    
        vImage_Error result;
        
        result = vImageConvert_Planar8toRGB888(&planarRed, &planarGreen, &planarBlue, &rgbDest, kvImageNoFlags);
        
        if(result != kvImageNoError)
        {
            VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to combined 3 r,g,b to RGB image\n");
            return VX_FAILURE;
        }
        
        // write the data back
        vxCommitImagePatch(output, &rect, 0, &dst_addr, base_dst_ptr);
        
        // release the planes
        for (p = 0; p < numplanes; p++)
        {
            vxCommitImagePatch(inputs[p], 0, 0, &src_addrs[p], &base_src_ptrs[p]);
        }
        
    }
    else if ((format == VX_DF_IMAGE_YUV4) || (format == VX_DF_IMAGE_IYUV))
    {
            // TODO: all these other types of destination format...
    }
    else if ((format == VX_DF_IMAGE_NV12) || (format == VX_DF_IMAGE_NV21))
    {
    }
    else if ((format == VX_DF_IMAGE_YUYV) || (format == VX_DF_IMAGE_UYVY))
    {
    }

    return VX_SUCCESS;
}

static vx_status VX_CALLBACK vxChannelCombineKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 5)
    {
        vx_image inputs[4] = {
            (vx_image)parameters[0],
            (vx_image)parameters[1],
            (vx_image)parameters[2],
            (vx_image)parameters[3],
        };
        vx_image output = (vx_image)parameters[4];

        return vxChannelCombine(inputs, output);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxChannelCombineInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index < 4)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image image = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));
            if (image)
            {
                vx_df_image format = 0;
                vxQueryImage(image, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if (format == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&image);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxChannelCombineOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 4)
    {
        vx_uint32 p, width = 0, height = 0;
        vx_uint32 uv_x_scale = 0, uv_y_scale = 0;
        vx_parameter params[] = {
                vxGetParameterByIndex(node, 0),
                vxGetParameterByIndex(node, 1),
                vxGetParameterByIndex(node, 2),
                vxGetParameterByIndex(node, 3),
                vxGetParameterByIndex(node, index)
        };
        vx_bool planes_present[4] = { vx_false_e, vx_false_e, vx_false_e, vx_false_e };
        // check for equal plane sizes and determine plane presence
        for (p = 0; p < index; p++)
        {
            if (params[p])
            {
                vx_image image = 0;
                vxQueryParameter(params[p], VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));
                planes_present[p] = image != 0;

                if (image)
                {
                    uint32_t w = 0, h = 0;
                    vxQueryImage(image, VX_IMAGE_ATTRIBUTE_WIDTH, &w, sizeof(w));
                    vxQueryImage(image, VX_IMAGE_ATTRIBUTE_HEIGHT, &h, sizeof(h));
                    if (width == 0 && height == 0)
                    {
                        width = w;
                        height = h;
                    }
                    else if (uv_x_scale == 0 && uv_y_scale == 0)
                    {
                        uv_x_scale = width == w ? 1 : (width == 2*w ? 2 : 0);
                        uv_y_scale = height == h ? 1 : (height == 2*h ? 2 : 0);
                        if (uv_x_scale == 0 || uv_y_scale == 0 || uv_y_scale > uv_x_scale)
                        {
                            status = VX_ERROR_INVALID_DIMENSION;
                            vxAddLogEntry((vx_reference)image, status, "Input image channel %u does not match in dimensions!\n", p);
                            goto exit;
                        }
                    }
                    else if (width != w * uv_x_scale || height != h * uv_y_scale)
                    {
                        status = VX_ERROR_INVALID_DIMENSION;
                        vxAddLogEntry((vx_reference)image, status, "Input image channel %u does not match in dimensions!\n", p);
                        goto exit;
                    }
                    vxReleaseImage(&image);
                }
            }
        }
        if (params[index])
        {
            vx_image output = 0;
            vxQueryParameter(params[index], VX_PARAMETER_ATTRIBUTE_REF, &output, sizeof(output));
            if (output)
            {
                vx_df_image format = VX_DF_IMAGE_VIRT;
                vx_bool supported_format = vx_true_e;
                vx_bool correct_planes = planes_present[0] && planes_present[1] && planes_present[2];

                vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                switch (format)
                {
                    case VX_DF_IMAGE_RGB:
                    case VX_DF_IMAGE_YUV4:
                        correct_planes = correct_planes && uv_y_scale == 1 && uv_x_scale == 1;
                        break;
                    case VX_DF_IMAGE_RGBX:
                        correct_planes = correct_planes && planes_present[3] && uv_y_scale == 1 && uv_x_scale == 1;
                        break;
                    case VX_DF_IMAGE_YUYV:
                    case VX_DF_IMAGE_UYVY:
                        correct_planes = correct_planes && uv_y_scale == 1 && uv_x_scale == 2;
                        break;
                    case VX_DF_IMAGE_NV12:
                    case VX_DF_IMAGE_NV21:
                    case VX_DF_IMAGE_IYUV:
                        correct_planes = correct_planes && uv_y_scale == 2 && uv_x_scale == 2;
                        break;
                    default:
                        supported_format = vx_false_e;
                }
                if (supported_format)
                {
                    if (correct_planes)
                    {
                        ptr->type = VX_TYPE_IMAGE;
                        ptr->dim.image.format = format;
                        ptr->dim.image.width = width;
                        ptr->dim.image.height = height;
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        VX_PRINT(VX_ZONE_API, "Valid format but missing planes!\n");
                    }
                }
                vxReleaseImage(&output);
            }
        }
exit:
        for (p = 0; p < dimof(params); p++)
        {
            if (params[p])
            {
                vxReleaseParameter(&params[p]);
            }
        }
    }
    VX_PRINT(VX_ZONE_API, "%s:%u returned %d\n", __FUNCTION__, index, status);
    return status;
}


static vx_param_description_t channel_combine_kernel_params[] = {
    {VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL},
    {VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t channelcombine_kernel = {
    VX_KERNEL_CHANNEL_COMBINE,
    "com.machineswithvision.openvx.channel_combine",
    vxChannelCombineKernel,
    channel_combine_kernel_params, dimof(channel_combine_kernel_params),
    vxChannelCombineInputValidator,
    vxChannelCombineOutputValidator,
    NULL,
    NULL,
};
