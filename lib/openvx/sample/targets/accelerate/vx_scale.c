/*
* Copyright (c) 2015 Machines With Vision
*/

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>
#include <stdio.h>
#include <math.h>

static vx_status vxAccelerateScaling(vx_image src_image, vx_image dst_image, const vx_border_mode_t *borders)
{
    vx_status status = VX_SUCCESS;
    vx_int32 x1,y1,x2,y2;
    void *src_base = NULL, *dst_base = NULL;
    vx_rectangle_t src_rect, dst_rect;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
    //vx_float32 wr, hr;
    
    vxQueryImage(src_image, VX_IMAGE_ATTRIBUTE_WIDTH, &w1, sizeof(w1));
    vxQueryImage(src_image, VX_IMAGE_ATTRIBUTE_HEIGHT, &h1, sizeof(h1));
    vxQueryImage(dst_image, VX_IMAGE_ATTRIBUTE_WIDTH, &w2, sizeof(w2));
    vxQueryImage(dst_image, VX_IMAGE_ATTRIBUTE_HEIGHT, &h2, sizeof(h2));

    src_rect.start_x = src_rect.start_y = 0;
    src_rect.end_x = w1;
    src_rect.end_y = h1;
    
    dst_rect.start_x = dst_rect.start_y = 0;
    dst_rect.end_x = w2;
    dst_rect.end_y = h2;

    status = VX_SUCCESS;
    
    status |= vxAccessImagePatch(src_image, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
    if (status == VX_SUCCESS)
    {
        vImage_Buffer srcVimg = {
            .data = src_base,
            .height = src_rect.end_y-src_rect.start_y,
            .width = src_rect.end_x-src_rect.start_x,
            .rowBytes = sizeof(vx_uint8)*(src_rect.end_x-src_rect.start_x)
        };
        
        vImage_Buffer dstVimg = {
            .data = dst_base,
            .height = dst_rect.end_y-dst_rect.start_y, // TODO: assumes input tests confirm same dimensions!
            .width = dst_rect.end_x-dst_rect.start_x,
            .rowBytes = sizeof(vx_uint8)*(dst_rect.end_x-dst_rect.start_x)
        };
        
        // Do it!
        
        vImage_Error result;
        
        result = vImageScale_Planar8(&srcVimg, &dstVimg, NULL, kvImageNoFlags);
        
        if(result != kvImageNoError)
        {
            VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to scale!\n");
            return VX_FAILURE;
        }

    }

    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);
    return status;
}

    
    

vx_status vxScaleImage(vx_image src_image, vx_image dst_image, vx_scalar stype, vx_border_mode_t *bordermode, vx_float64 *interm, vx_size size)
{
    vx_status status = VX_FAILURE;
    vx_enum type = 0;
    
    vxReadScalarValue(stype, &type);
    if (interm && size)
    {
        status = vxAccelerateScaling(src_image, dst_image, bordermode);
        
        
        /*
        if (type == VX_INTERPOLATION_TYPE_BILINEAR)
        {
            //status = vxBilinearScaling(src_image, dst_image, bordermode);
        }
        else if (type == VX_INTERPOLATION_TYPE_AREA)
        {
#if AREA_SCALE_ENABLE // TODO FIX THIS
            //status = vxAreaScaling(src_image, dst_image, bordermode, interm, size);
#else
            //status = vxNearestScaling(src_image, dst_image, bordermode);
#endif
        }
        else if (type == VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR)
        {
            //status = vxNearestScaling(src_image, dst_image, bordermode);
        }
         */
    }
    else
    {
        status = VX_ERROR_NO_RESOURCES;
    }
    return status;
}



static vx_status VX_CALLBACK vxScaleImageKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 3)
    {
        vx_image  src_image = (vx_image) parameters[0];
        vx_image  dst_image = (vx_image) parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_border_mode_t bordermode = {VX_BORDER_MODE_UNDEFINED, 0};
        vx_float64 *interm = NULL;
        vx_size size = 0ul;

        vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        vxQueryNode(node, VX_NODE_ATTRIBUTE_LOCAL_DATA_PTR, &interm, sizeof(interm));
        vxQueryNode(node, VX_NODE_ATTRIBUTE_LOCAL_DATA_SIZE,&size, sizeof(size));

        return vxScaleImage(src_image, dst_image, stype, &bordermode, interm, size);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxScaleImageInitializer(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_image src = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
        vx_size size = 0;

        vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &w1, sizeof(w1));
        vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &h1, sizeof(h1));
        vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_WIDTH, &w2, sizeof(w2));
        vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_HEIGHT, &h2, sizeof(h2));

        size = 1;

        vxSetNodeAttribute(node, VX_NODE_ATTRIBUTE_LOCAL_DATA_SIZE, &size, sizeof(size));
        status = VX_SUCCESS;
    }
    return status;
}

static vx_status VX_CALLBACK vxScaleImageInputValidator(vx_node node, vx_uint32 index)
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
            if (format == VX_DF_IMAGE_U8)
            {
                status = VX_SUCCESS;
            }
            vxReleaseImage(&input);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar scalar = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &scalar, sizeof(scalar));
            if (scalar)
            {
                vx_enum stype = 0;
                vxQueryScalar(scalar, VX_SCALAR_ATTRIBUTE_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_ENUM)
                {
                    vx_enum interp = 0;
                    vxReadScalarValue(scalar, &interp);
                    if ((interp == VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR) ||
                        (interp == VX_INTERPOLATION_TYPE_BILINEAR) ||
                        (interp == VX_INTERPOLATION_TYPE_AREA))
                    {
                        status = VX_SUCCESS;
                    }
                    else
                    {
                        status = VX_ERROR_INVALID_VALUE;
                    }
                }
                else
                {
                    status = VX_ERROR_INVALID_TYPE;
                }
                vxReleaseScalar(&scalar);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxScaleImageOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if (src_param && dst_param)
        {
            vx_image src = 0;
            vx_image dst = 0;
            vxQueryParameter(src_param, VX_PARAMETER_ATTRIBUTE_REF, &src, sizeof(src));
            vxQueryParameter(dst_param, VX_PARAMETER_ATTRIBUTE_REF, &dst, sizeof(dst));
            if ((src) && (dst))
            {
                vx_uint32 w1 = 0, h1 = 0, w2 = 0, h2 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT, f2 = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_WIDTH, &w2, sizeof(w2));
                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_HEIGHT, &h2, sizeof(h2));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &f1, sizeof(f1));
                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_FORMAT, &f2, sizeof(f2));
                // output can not be virtual
                if ((w2 != 0) && (h2 != 0) && (f2 != VX_DF_IMAGE_VIRT) && (f1 == f2))
                {
                    // fill in the meta data with the attributes so that the checker will pass
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = f2;
                    ptr->dim.image.width = w2;
                    ptr->dim.image.height = h2;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&src);
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_param_description_t scale_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t scale_image_kernel = {
    VX_KERNEL_SCALE_IMAGE,
    "com.machineswithvision.openvx.scale_image",
    vxScaleImageKernel,
    scale_kernel_params, dimof(scale_kernel_params),
    vxScaleImageInputValidator,
    vxScaleImageOutputValidator,
    vxScaleImageInitializer,
    NULL,
};
