/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxAbsDiff(vx_image in1, vx_image in2, vx_image output)
{
    vx_status status = VX_SUCCESS;

    // Check format of input and output...
    vx_df_image in1_format, out_format;
    vxQueryImage(in1, VX_IMAGE_ATTRIBUTE_FORMAT, &in1_format, sizeof(in1_format));
    vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &out_format, sizeof(out_format));
    
    // Access data...
    void* in1_ptr = NULL;
    void* in2_ptr = NULL;
    void* out_ptr = NULL;
    
    vx_rectangle_t rect;// = {0, 0, input->width, input->height};
    vx_imagepatch_addressing_t in1_addr, in2_addr, out_addr;
    
    status = vxGetValidRegionImage(in1, &rect);
    status |= vxAccessImagePatch(in1, &rect, 0, &in1_addr, &in1_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    status |= vxGetValidRegionImage(in2, &rect);
    status |= vxAccessImagePatch(in2, &rect, 0, &in2_addr, &in2_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    status |= vxGetValidRegionImage(output, &rect);
    status |= vxAccessImagePatch(output, &rect, 0, &out_addr, &out_ptr, VX_WRITE_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;

    // Convert internally/explicitly
    
    float *A = (float*)malloc(width*height*sizeof(float));
    float *B = (float*)malloc(width*height*sizeof(float));
    float *C = (float*)malloc(width*height*sizeof(float));
    
    if (in1_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* in1_data = vxFormatImagePatchAddress1d(in1_ptr,0,&in1_addr);
        vx_uint8* in2_data = vxFormatImagePatchAddress1d(in2_ptr,0,&in2_addr);
        
        vDSP_vfltu8(in1_data, 1, A, 1, width*height);
        vDSP_vfltu8(in2_data, 1, B, 1, width*height);
    } // TODO: other input formats!
    
    // Do it! In this case, use the vector distance function...
    
    vDSP_vdist(A, 1, B, 1, C, 1, width*height);
    
    // Convert to output...
    
    if (out_format == VX_DF_IMAGE_U8)
    {
        vx_uint8* out_data = vxFormatImagePatchAddress1d(out_ptr,0,&out_addr);
        
        vDSP_vfixu8(C, 1, out_data, 1, width*height);
    }
    
    // Free extra memory used
    
    free(A);
    free(B);
    free(C);

    // Commit
    
    status |= vxCommitImagePatch(in1, NULL, 0, &in1_addr, in1_ptr);
    status |= vxCommitImagePatch(in2, NULL, 0, &in2_addr, in2_ptr);
    status |= vxCommitImagePatch(output, &rect, 0, &out_addr, out_ptr);
    
    return status;
}

static vx_status VX_CALLBACK vxAbsDiffKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_image in1 = (vx_image)parameters[0];
        vx_image in2 = (vx_image)parameters[1];
        vx_image output = (vx_image)parameters[2];
        status = VX_SUCCESS;
        status = vxAbsDiff(in1, in2, output);
    }
    
    return status;
}

static vx_status VX_CALLBACK vxAbsDiffInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0 )
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
                || format == VX_DF_IMAGE_S16 || format == VX_DF_IMAGE_U16
#endif
                )
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
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&images[0]);
            vxReleaseImage(&images[1]);
        }
        vxReleaseParameter(&param[0]);
        vxReleaseParameter(&param[1]);
    }
    return status;
}

static vx_status VX_CALLBACK vxAbsDiffOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param[2] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        if (param[0] && param[1])
        {
            vx_image images[2];
            vxQueryParameter(param[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
            vxQueryParameter(param[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
            if (images[0] && images[1])
            {
                vx_uint32 width[2], height[2];
                vx_df_image format = 0;
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width[0], sizeof(width[0]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_WIDTH, &width[1], sizeof(width[1]));
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[0], sizeof(height[0]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[1], sizeof(height[1]));
                if (width[0] == width[1] && height[0] == height[1] &&
                    (format == VX_DF_IMAGE_U8
#if defined(EXPERIMENTAL_USE_S16)
                     || format == VX_DF_IMAGE_U16
#endif
                     ))
                {
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = format;
                    ptr->dim.image.width = width[0];
                    ptr->dim.image.height = height[1];
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
            }
            vxReleaseParameter(&param[0]);
            vxReleaseParameter(&param[1]);
        }
    }
    return status;
}

static vx_param_description_t absdiff_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t absdiff_kernel = {
    VX_KERNEL_ABSDIFF,
    "com.machineswithvision.openvx.absdiff",
    vxAbsDiffKernel,
    absdiff_kernel_params,
    dimof(absdiff_kernel_params),
    vxAbsDiffInputValidator,
    vxAbsDiffOutputValidator,
    NULL,
    NULL,
};

