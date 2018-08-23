/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxThreshold(vx_image src_image, vx_threshold threshold, vx_image dst_image)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    void *src_base = NULL, *dst_base = NULL;
    vx_uint32 y = 0, x = 0;
    vx_int32 value = 0, lower = 0, upper = 0;
    vx_status status = VX_FAILURE;
    
    vxQueryThreshold(threshold, VX_THRESHOLD_ATTRIBUTE_TYPE, &type, sizeof(type));
    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_VALUE, &value, sizeof(value));
    }
    else if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        vxQueryThreshold(threshold, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_LOWER, &lower, sizeof(lower));
        vxQueryThreshold(threshold, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_UPPER, &upper, sizeof(upper));
    }
    status = vxGetValidRegionImage(src_image, &rect);
    status |= vxAccessImagePatch(src_image, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    VX_PRINT(VX_ZONE_INFO, "threshold = %u\n", value);
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    // Convert internally/explicitly
    
    float *A = (float*)malloc(width*height*sizeof(float));
    float *C = (float*)malloc(width*height*sizeof(float));
    float *D = (float*)malloc(width*height*sizeof(float));
    
    // We know this should be u8 image data - but need to convert to float for op
    
    vx_uint8* src_data = vxFormatImagePatchAddress1d(src_base,0,&src_addr);
        
    vDSP_vfltu8(src_data, 1, A, 1, width*height);
    
    /*
     
     vDSP_vthr
     
     https://developer.apple.com/library/prerelease/ios/documentation/Accelerate/Reference/vDSPRef/index.html#//apple_ref/c/func/vDSP_vthr
     
     */
    
    // Do it!
    
    if (type == VX_THRESHOLD_TYPE_BINARY)
    {
        float bthres = value; // Cast from int32 to float
        float cval = 255.0;
        vDSP_vthrsc(A, 1, &bthres, &cval, D, 1, width*height); // Output Range 255 or -255
        float bval = 0.0;
        vDSP_vthres(D,1,&bval,C,1,width*height); // If > 0 then keep 255 else reset to 0.
    }
    else if (type == VX_THRESHOLD_TYPE_RANGE)
    {
        // TODO:
    }
    
    // Convert back to u8
    
    vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
    
    vDSP_vfixu8(C, 1, dst_data, 1, width*height);
    
    // Free memory.
    
    free(A); free(C); free(D);
    
    // Commit it
    
    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &rect, 0, &dst_addr, dst_base);
    
    return status;
}

static vx_status VX_CALLBACK vxThresholdKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image     src_image = (vx_image)    parameters[0];
        vx_threshold threshold = (vx_threshold)parameters[1];
        vx_image     dst_image = (vx_image)    parameters[2];
        
        return vxThreshold(src_image, threshold, dst_image);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxThresholdInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if (input)
            {
                vx_df_image format = 0;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if (format == VX_DF_IMAGE_U8)
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_FORMAT;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_threshold threshold = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &threshold, sizeof(threshold));
            if (threshold)
            {
                vx_enum type = 0;
                vxQueryThreshold(threshold, VX_THRESHOLD_ATTRIBUTE_TYPE, &type, sizeof(type));
                if ((type == VX_THRESHOLD_TYPE_BINARY) ||
                     (type == VX_THRESHOLD_TYPE_RANGE))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseThreshold(&threshold);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxThresholdOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        if (src_param)
        {
            vx_image src = 0;
            vxQueryParameter(src_param, VX_PARAMETER_ATTRIBUTE_REF, &src, sizeof(src));
            if (src)
            {
                vx_uint32 width = 0, height = 0;

                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(height));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));

                // fill in the meta data with the attributes so that the checker will pass
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&src);
            }
            vxReleaseParameter(&src_param);
        }
    }
    return status;
}

static vx_param_description_t threshold_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_THRESHOLD,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT,VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t threshold_kernel = {
    VX_KERNEL_THRESHOLD,
    "com.machineswithvision.openvx.threshold",
    vxThresholdKernel,
    threshold_kernel_params, dimof(threshold_kernel_params),
    vxThresholdInputValidator,
    vxThresholdOutputValidator,
    NULL,
    NULL,
};


