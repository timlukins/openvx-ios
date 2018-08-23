/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

#include <math.h>

vx_status vxHistogram(vx_image src, vx_distribution dist)
{
    vx_rectangle_t src_rect;
    vx_imagepatch_addressing_t src_addr;
    void* src_base = NULL;
    void* dist_ptr = NULL;
    vx_df_image format = 0;
    vx_uint32 x = 0;
    vx_uint32 y = 0;
    vx_size offset = 0;
    vx_size range = 0;
    vx_size numBins = 0;
    vx_uint32 window_size = 0;
    vx_status status = VX_SUCCESS;
    
    vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    vxQueryDistribution(dist, VX_DISTRIBUTION_ATTRIBUTE_BINS, &numBins, sizeof(numBins));
    vxQueryDistribution(dist, VX_DISTRIBUTION_ATTRIBUTE_RANGE, &range, sizeof(range));
    vxQueryDistribution(dist, VX_DISTRIBUTION_ATTRIBUTE_OFFSET, &offset, sizeof(offset));
    vxQueryDistribution(dist, VX_DISTRIBUTION_ATTRIBUTE_WINDOW, &window_size, sizeof(window_size));
    
    status = vxGetValidRegionImage(src, &src_rect);
    status |= vxAccessImagePatch(src, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessDistribution(dist, &dist_ptr, VX_WRITE_ONLY);
    //printf("distribution:%p bins:%u off:%u ws:%u range:%u\n", dist_ptr, numBins, offset, window_size, range);
    
    if (status == VX_SUCCESS)
    {
        // TODO: loads of stuff here with numBins, ranghe, offset, window_size - DON'T THINK CAN DO IN ACCELERATE
        
        int width  = src_rect.end_x - src_rect.start_x;
        int height = src_rect.end_y - src_rect.start_y;
        
        vImage_Buffer srcVimg = {
            .data = src_base,
            .height = height,
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width
        };
        
        vImagePixelCount count[256];
        
        // Do it!
        
        vImage_Error result;
        
        result = vImageHistogramCalculation_Planar8(&srcVimg, count, kvImageNoFlags);
        
        if(result != kvImageNoError)
        {
            VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to combined 3 r,g,b to RGB image\n");
            return VX_FAILURE;
        }
        
        // Have to copy values into distribution
        
        vx_int32 *dist_tmp = dist_ptr;
        
        for (int i=0;i<256;i++) dist_tmp[i] = count[i];
        
        /*
        vx_int32 *dist_tmp = dist_ptr;
        
        for (x = 0; x < numBins; x++)
        {
            dist_tmp[x] = 0;
        }
        
        for (y = 0; y < src_addr.dim_y; y++)
        {
            for (x = 0; x < src_addr.dim_x; x++)
            {
                if (format == VX_DF_IMAGE_U8)
                {
                    vx_uint8 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                    vx_uint8 pixel = *src_ptr;
                    if ((offset <= (vx_size)pixel) && ((vx_size)pixel < (offset+range)))
                    {
                        vx_size index = (pixel - (vx_uint16)offset) / window_size;
                        dist_tmp[index]++;
                    }
                }
                else if (format == VX_DF_IMAGE_U16)
                {
                    vx_uint16 *src_ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                    vx_uint16 pixel = *src_ptr;
                    if ((offset <= (vx_size)pixel) && ((vx_size)pixel < (offset+range)))
                    {
                        vx_size index = (pixel - (vx_uint16)offset) / window_size;
                        dist_tmp[index]++;
                    }
                }
            }
        }
        */
    }
    status |= vxCommitDistribution(dist, dist_ptr);
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    
    return status;
}



static vx_status VX_CALLBACK vxHistogramKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 2)
    {
        vx_image src_image   = (vx_image) parameters[0];
        vx_distribution dist = (vx_distribution)parameters[1];
        return vxHistogram(src_image, dist); // TODO
    }
    return VX_ERROR_INVALID_PARAMETERS;
}


static vx_status VX_CALLBACK vxHistogramInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8
#if defined(EXPERIMENTAL_USE_S16)
                || format == VX_DF_IMAGE_U16
#endif
                )
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxHistogramOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_image src = 0;
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, 1);
        vx_distribution dist;

        vxQueryParameter(src_param, VX_PARAMETER_ATTRIBUTE_REF, &src, sizeof(src));
        vxQueryParameter(dst_param, VX_PARAMETER_ATTRIBUTE_REF, &dist, sizeof(dist));
        if ((src) && (dist))
        {
            vx_uint32 width = 0, height = 0;
            vx_df_image format;
            vx_size numBins = 0;
            vxQueryDistribution(dist, VX_DISTRIBUTION_ATTRIBUTE_BINS, &numBins, sizeof(numBins));
            vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(height));
            vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
            vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            // fill in the meta data with the attributes so that the checker will pass
            ptr->type = VX_TYPE_DISTRIBUTION;
            status = VX_SUCCESS;
            vxReleaseDistribution(&dist);
            vxReleaseImage(&src);
        }
        vxReleaseParameter(&dst_param);
        vxReleaseParameter(&src_param);
    }
    return status;
}


static vx_param_description_t histogram_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_DISTRIBUTION, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t histogram_kernel = {
    VX_KERNEL_HISTOGRAM,
    "com.machineswithvision.openvx.histogram",
    vxHistogramKernel,
    histogram_kernel_params, dimof(histogram_kernel_params),
    vxHistogramInputValidator,
    vxHistogramOutputValidator,
    NULL,
    NULL,
};

