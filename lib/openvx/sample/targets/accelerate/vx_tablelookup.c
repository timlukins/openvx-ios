/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

#include <math.h>

vx_status vxTableLookup(vx_image src, vx_lut lut, vx_image dst)
{
    vx_enum type = 0;
    vx_rectangle_t rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    void *src_base = NULL, *dst_base = NULL, *lut_ptr = NULL;
    vx_uint32 y = 0, x = 0;
    vx_size count = 0;
    vx_status status = VX_SUCCESS;
    
    vxQueryLUT(lut, VX_LUT_ATTRIBUTE_TYPE, &type, sizeof(type));
    vxQueryLUT(lut, VX_LUT_ATTRIBUTE_COUNT, &count, sizeof(count));
    
    status = vxGetValidRegionImage(src, &rect);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    status |= vxAccessLUT(lut, &lut_ptr, VX_READ_ONLY);

    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    if (status == VX_SUCCESS)
    {
        // Create temporary float array destination memory...
        
        float *A = (float*)malloc(width*height*sizeof(float));
        
        // Images...
        
        vImage_Buffer input = {
            .data = src_base,
            .height = height,
            .width = width,
            .rowBytes = sizeof(vx_uint8)*width
        };
        
        vImage_Buffer output = {
            .data = A, // Use our float target here
            .height = height,
            .width = width,
            .rowBytes = sizeof(float)*width // NOTE: float output!
        };
        
        // Build Lut... // TODO: what if not 256 values???
        
        Pixel_F table[256];
        for (int c=0;c<count;c++)
        {
            if (type == VX_TYPE_UINT8)
            {
                vx_uint8 *lut_tmp = (vx_uint8 *)lut_ptr;
                table[c] = lut_tmp[c];
            }
            else if (type == VX_TYPE_INT16)
            {
                vx_int16 *lut_tmp = (vx_int16 *)lut_ptr;
                table[c] = lut_tmp[c];
                
            }
        }

        // Do it!
        
        vImage_Error result;
        
        result = vImageLookupTable_Planar8toPlanarF(&input, &output, table, kvImageNoFlags);
        
        if(result != kvImageNoError)
        {
            VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to scale!\n");
            return VX_FAILURE;
        }
        
        // Convert float data back to u8
        
        vx_uint8* dst_data = vxFormatImagePatchAddress1d(dst_base,0,&dst_addr);
        
        vDSP_vfixu8(A, 1, dst_data, 1, width*height);
        
    }
    
    // Commit
    
    status |= vxCommitLUT(lut, lut_ptr);
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    
    return status;
}
    

static vx_status VX_CALLBACK vxTableLookupKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image src_image = (vx_image) parameters[0];
        vx_lut   lut       = (vx_lut)parameters[1];
        vx_image dst_image = (vx_image) parameters[2];
        
        return vxTableLookup(src_image, lut, dst_image);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxTableLookupInputValidator(vx_node node, vx_uint32 index)
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
                || format == VX_DF_IMAGE_S16
#endif
                )
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        vx_lut lut = 0;
        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &lut, sizeof(lut));
        if (lut)
        {
            vx_enum type = 0;
            vxQueryLUT(lut, VX_LUT_ATTRIBUTE_TYPE, &type, sizeof(type));
            if (type == VX_TYPE_UINT8 || type == VX_TYPE_INT16)
            {
                status = VX_SUCCESS;
            }
            vxReleaseLUT(&lut);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxTableLookupOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
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
                // * output is equal type and size
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

static vx_param_description_t lut_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_LUT,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT,VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t lut_kernel = {
    VX_KERNEL_TABLE_LOOKUP,
    "com.machineswithvision.openvx.table_lookup",
    vxTableLookupKernel,
    lut_kernel_params, dimof(lut_kernel_params),
    vxTableLookupInputValidator,
    vxTableLookupOutputValidator,
    NULL,
    NULL,
};


