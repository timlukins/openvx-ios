/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxWarpAffine(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_mode_t *borders)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL;
    void *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_uint32 dst_width, dst_height;
    vx_rectangle_t src_rect;
    vx_rectangle_t dst_rect;
    
    vx_float32 m[9];
    vx_enum type = 0;

    //vx_uint32 y = 0u, x = 0u;
    
    vxQueryImage(dst_image, VX_IMAGE_ATTRIBUTE_WIDTH, &dst_width, sizeof(dst_width));
    vxQueryImage(dst_image, VX_IMAGE_ATTRIBUTE_HEIGHT, &dst_height, sizeof(dst_height));
    
    vxGetValidRegionImage(src_image, &src_rect);
    dst_rect.start_x = 0;
    dst_rect.start_y = 0;
    dst_rect.end_x = dst_width;
    dst_rect.end_y = dst_height;
    
    status |= vxAccessImagePatch(src_image, &src_rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst_image, &dst_rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    
    status |= vxReadMatrix(matrix, m);
    status |= vxReadScalarValue(stype, &type);
    
    if (status == VX_SUCCESS)
    {
        // Get images...
        
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
        
        // Transform matrix...
        
        vImage_AffineTransform T;
        T.a = m[0];
        T.b = m[1];
        T.c = m[2];
        T.d = m[3];
        T.tx = m[4];
        T.ty = m[5];
        
        // Do it!
        
        vImage_Error result;
        
        result = vImageAffineWarp_Planar8(&srcVimg, &dstVimg,NULL,&T,0, kvImageBackgroundColorFill);
        
        if(result != kvImageNoError)
        {
            VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to perform affine warp\n");
            return VX_FAILURE;
        }

        
    }
    
    status |= vxWriteMatrix(matrix, m);
    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);

    return status;

}


static vx_status VX_CALLBACK vxWarpAffineKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  src_image = (vx_image) parameters[0];
        vx_matrix matrix    = (vx_matrix)parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_image  dst_image = (vx_image) parameters[3];

        vx_border_mode_t borders;
        vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &borders, sizeof(borders));

        return vxWarpAffine(src_image, matrix, stype, dst_image, &borders);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status vxWarpAffineInputValidator(vx_node node, vx_uint32 index)
{
    vx_size mat_columns = 2;
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
    else if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_matrix matrix;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &matrix, sizeof(matrix));
            if (matrix)
            {
                vx_enum data_type = 0;
                vx_size rows = 0ul, columns = 0ul;
                vxQueryMatrix(matrix, VX_MATRIX_ATTRIBUTE_TYPE, &data_type, sizeof(data_type));
                vxQueryMatrix(matrix, VX_MATRIX_ATTRIBUTE_ROWS, &rows, sizeof(rows));
                vxQueryMatrix(matrix, VX_MATRIX_ATTRIBUTE_COLUMNS, &columns, sizeof(columns));
                if ((data_type == VX_TYPE_FLOAT32) && (columns == mat_columns) && (rows == 3))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseMatrix(&matrix);
            }
            vxReleaseParameter(&param);
        }
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
                        (interp == VX_INTERPOLATION_TYPE_BILINEAR))
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

static vx_status VX_CALLBACK vxWarpAffineOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter dst_param = vxGetParameterByIndex(node, index);
        if (dst_param)
        {
            vx_image dst = 0;
            vxQueryParameter(dst_param, VX_PARAMETER_ATTRIBUTE_REF, &dst, sizeof(dst));
            if (dst)
            {
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_FORMAT, &f1, sizeof(f1));
                // output can not be virtual
                if ((w1 != 0) && (h1 != 0) && (f1 == VX_DF_IMAGE_U8))
                {
                    // fill in the meta data with the attributes so that the checker will pass
                    ptr->type = VX_TYPE_IMAGE;
                    ptr->dim.image.format = VX_DF_IMAGE_U8;
                    ptr->dim.image.width = w1;
                    ptr->dim.image.height = h1;
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&dst);
            }
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_param_description_t warp_affine_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_MATRIX, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t warp_affine_kernel = {
    VX_KERNEL_WARP_AFFINE,
    "com.machineswithvision.openvx.warp_affine",
    vxWarpAffineKernel,
    warp_affine_kernel_params, dimof(warp_affine_kernel_params),
    vxWarpAffineInputValidator,
    vxWarpAffineOutputValidator,
    NULL,
    NULL,
};

