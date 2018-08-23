/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxMeanStdDev(vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_float32 fmean = 0.0f, fstddev = 0.0f;
    vx_df_image format = 0;
    vx_rectangle_t rect ;
    vx_imagepatch_addressing_t addrs;
    void *base_ptr = NULL;
    vx_uint32 x, y;
    vx_status status  = VX_SUCCESS;
    
    vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    status = vxGetValidRegionImage(input, &rect);
    //VX_PRINT(VX_ZONE_INFO, "Rectangle = {%u,%u x %u,%u}\n",rect.start_x, rect.start_y, rect.end_x, rect.end_y);
    status |= vxAccessImagePatch(input, &rect, 0, &addrs, &base_ptr, VX_READ_ONLY);
    
    // Access memory and convert to float...
    
    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    float *A = (float*)malloc(width*height*sizeof(float));
    vx_uint8* a_data = vxFormatImagePatchAddress1d(base_ptr,0,&addrs);
    vDSP_vfltu8(a_data, 1, A, 1, width*height);
    
    // Calculate mean (easy enough)
    vDSP_meanv(A,1,&fmean,width*height); // find the mean of the vector
    
    // Then do this to find stddev...
    // FROM: http://stackoverflow.com/questions/17654633/standard-deviation-of-a-uiimage-cgimage

    float nmean = -1*fmean; // Invert mean so when we add it is actually subtraction
    float *subMeanVec  = (float*)calloc(width*height,sizeof(float)); // placeholder vector
    vDSP_vsadd(A,1,&nmean,subMeanVec,1,width*height); // subtract mean from vector
    float *squared = (float*)calloc(width*height,sizeof(float)); // placeholder for squared vector
    vDSP_vsq(subMeanVec,1,squared,1,width*height); // Square vector element by element
    float sum = 0; // place holder for sum
    vDSP_sve(squared,1,&sum,width*height); //sum entire vector
    fstddev = sqrt(sum/(width*height)); // calculated std deviati
    
    // Clear memory
    free(A); free(subMeanVec); free(squared);
    
    // ORIGINALLY fstddev = (vx_float32)sqrt(sum_diff_sqrs / (addrs.dim_x*addrs.dim_y));
    status |= vxWriteScalarValue(mean, &fmean);
    status |= vxWriteScalarValue(stddev, &fstddev);
    status |= vxCommitImagePatch(input, &rect, 0, &addrs, base_ptr);
    
    return status;
}


static vx_status VX_CALLBACK vxMeanStdDevKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image  input    = (vx_image) parameters[0];
        vx_scalar s_mean   = (vx_scalar)parameters[1];
        vx_scalar s_stddev = (vx_scalar)parameters[2];
        
        return vxMeanStdDev(input, s_mean, s_stddev);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxMeanStdDevInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        vx_image image = 0;
        vx_df_image format = 0;

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));
        if (image == 0)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
        else
        {
            status = VX_SUCCESS;
            vxQueryImage(image, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
            if (format != VX_DF_IMAGE_U8
#if defined(EXPERIMENTAL_USE_S16)
                && format != VX_DF_IMAGE_U16
#endif
                )
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            vxReleaseImage(&image);
        }
        vxReleaseParameter(&param);
    }
    return status;
}

static vx_status VX_CALLBACK vxMeanStdDevOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1 || index == 2)
    {
        ptr->type = VX_TYPE_SCALAR;
        ptr->dim.scalar.type = VX_TYPE_FLOAT32;
        status = VX_SUCCESS;
    }
    VX_PRINT(VX_ZONE_API, "%s:%u returned %d\n", __FUNCTION__, index, status);
    return status;
}


static vx_param_description_t mean_stddev_kernel_params[] = {
    {VX_INPUT,  VX_TYPE_IMAGE,   VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t mean_stddev_kernel = {
    VX_KERNEL_MEAN_STDDEV,
    "com.machineswithvision.openvx.mean_stddev",
    vxMeanStdDevKernel,
    mean_stddev_kernel_params, dimof(mean_stddev_kernel_params),
    vxMeanStdDevInputValidator,
    vxMeanStdDevOutputValidator,
    NULL,
    NULL,
};

