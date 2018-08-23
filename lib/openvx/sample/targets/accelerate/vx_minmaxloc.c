/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxMinMaxLoc(vx_image input, vx_scalar minVal, vx_scalar maxVal, vx_array minLoc, vx_array maxLoc, vx_scalar minCount, vx_scalar maxCount)
{
    vx_uint32 y, x;
    void *src_base = NULL;
    vx_imagepatch_addressing_t src_addr;
    vx_rectangle_t rect;
    vx_df_image format;
    // Change these to floats (TODO: check no loss of precision)
    //vx_int64 iMinVal = INT64_MAX;
    //vx_int64 iMaxVal = INT64_MIN;
    float iMinVal;
    float iMaxVal;
    vx_uint32 iMinCount = 0;
    vx_uint32 iMaxCount = 0;
    vx_status status = VX_SUCCESS;
    
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(input, &rect);
    status |= vxAccessImagePatch(input, &rect, 0, &src_addr, (void **)&src_base, VX_READ_ONLY);
    
    // Access memory and convert to floats...
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    float *A = (float*)malloc(width*height*sizeof(float));
    vx_uint8* a_data = vxFormatImagePatchAddress1d(src_base,0,&src_addr);
    vDSP_vfltu8(a_data, 1, A, 1, width*height);
    
    // Find the maxium and minimum
    // SEE: https://developer.apple.com/library/prerelease/ios/documentation/Accelerate/Reference/vDSPRef/index.html#//apple_ref/doc/uid/TP40009464-CH16-SW2
    
    vDSP_Length maxindex;
    vDSP_maxvi(A, 1, &iMaxVal, &maxindex, width*height);
    vDSP_Length minindex;
    vDSP_maxvi(A, 1, &iMinVal, &minindex, width*height);
    
    // TODO: this only returns the index of the first max/min value
    // need to build array (and count) of all found min/max values...
    
    // Commit
    
    VX_PRINT(VX_ZONE_INFO, "Min = %ld Max = %ld\n", iMinVal, iMaxVal);
    status |= vxCommitImagePatch(input, NULL, 0, &src_addr, src_base);
    
    // Free
    
    free(A);
    
    // Cast the min and max value appropriately
    
    switch (format)
    {
        case VX_DF_IMAGE_U8:
        {
            vx_uint8 min = (vx_uint8)iMinVal, max = (vx_uint8)iMaxVal;
            if (minVal) vxWriteScalarValue(minVal, &min);
            if (maxVal) vxWriteScalarValue(maxVal, &max);
        }
            break;
        case VX_DF_IMAGE_U16:
        {
            vx_uint16 min = (vx_uint16)iMinVal, max = (vx_uint16)iMaxVal;
            if (minVal) vxWriteScalarValue(minVal, &min);
            if (maxVal) vxWriteScalarValue(maxVal, &max);
        }
            break;
        case VX_DF_IMAGE_U32:
        {
            vx_uint32 min = (vx_uint32)iMinVal, max = (vx_uint32)iMaxVal;
            if (minVal) vxWriteScalarValue(minVal, &min);
            if (maxVal) vxWriteScalarValue(maxVal, &max);
        }
            break;
        case VX_DF_IMAGE_S16:
        {
            vx_int16 min = (vx_int16)iMinVal, max = (vx_int16)iMaxVal;
            if (minVal) vxWriteScalarValue(minVal, &min);
            if (maxVal) vxWriteScalarValue(maxVal, &max);
        }
            break;
        case VX_DF_IMAGE_S32:
        {
            vx_int32 min = (vx_int32)iMinVal, max = (vx_int32)iMaxVal;
            if (minVal) vxWriteScalarValue(minVal, &min);
            if (maxVal) vxWriteScalarValue(maxVal, &max);
        }
            break;
    }
    
    if (minCount) vxWriteScalarValue(minCount, &iMinCount);
    if (maxCount) vxWriteScalarValue(maxCount, &iMaxCount);
    
    return status;
}

static vx_status VX_CALLBACK vxMinMaxLocKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 7)
    {
        vx_image input = (vx_image)parameters[0];
        vx_scalar minVal = (vx_scalar)parameters[1];
        vx_scalar maxVal = (vx_scalar)parameters[2];
        vx_array minLoc = (vx_array)parameters[3];
        vx_array maxLoc = (vx_array)parameters[4];
        vx_scalar minCount = (vx_scalar)parameters[5];
        vx_scalar maxCount = (vx_scalar)parameters[6];
        
        return vxMinMaxLoc(input, minVal, maxVal, minLoc, maxLoc, minCount, maxCount);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxMinMaxLocInputValidator(vx_node node, vx_uint32 index)
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
            if ((format == VX_DF_IMAGE_U8)
                || (format == VX_DF_IMAGE_S16)
#if defined(EXPERIMENTAL_USE_S16)
                || (format == VX_DF_IMAGE_U16)
                || (format == VX_DF_IMAGE_U32)
                || (format == VX_DF_IMAGE_S32)
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

static vx_status VX_CALLBACK vxMinMaxLocOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if ((index == 1) || (index == 2))
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if (input)
            {
                vx_df_image format;
                vx_enum type = VX_TYPE_INVALID;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                switch (format)
                {
                    case VX_DF_IMAGE_U8:
                        type = VX_TYPE_UINT8;
                        break;
                    case VX_DF_IMAGE_U16:
                        type = VX_TYPE_UINT16;
                        break;
                    case VX_DF_IMAGE_U32:
                        type = VX_TYPE_UINT32;
                        break;
                    case VX_DF_IMAGE_S16:
                        type = VX_TYPE_INT16;
                        break;
                    case VX_DF_IMAGE_S32:
                        type = VX_TYPE_INT32;
                        break;
                    default:
                        type = VX_TYPE_INVALID;
                        break;
                }
                if (type != VX_TYPE_INVALID)
                {
                    status = VX_SUCCESS;
                    ptr->type = VX_TYPE_SCALAR;
                    ptr->dim.scalar.type = type;
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    if ((index == 3) || (index == 4))
    {
        // nothing to check here
        ptr->dim.array.item_type = VX_TYPE_COORDINATES2D;
        ptr->dim.array.capacity = 1;
        status = VX_SUCCESS;
    }
    if ((index == 5) || (index == 6))
    {
        ptr->dim.scalar.type = VX_TYPE_UINT32;
        status = VX_SUCCESS;
    }
    return status;
}

static vx_param_description_t minmaxloc_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t minmaxloc_kernel = {
    VX_KERNEL_MINMAXLOC,
    "com.machineswithvision.openvx.min_max_loc",
    vxMinMaxLocKernel,
    minmaxloc_kernel_params, dimof(minmaxloc_kernel_params),
    vxMinMaxLocInputValidator,
    vxMinMaxLocOutputValidator,
    NULL,
    NULL,
};


