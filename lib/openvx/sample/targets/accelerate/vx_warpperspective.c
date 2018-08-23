/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

typedef void (*transform_f)(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y);

static void transform_perspective(vx_uint32 dst_x, vx_uint32 dst_y, const vx_float32 m[], vx_float32 *src_x, vx_float32 *src_y)
{
    vx_float32 z = dst_x * m[2] + dst_y * m[5] + m[8];
    
    *src_x = (dst_x * m[0] + dst_y * m[3] + m[6]) / z;
    *src_y = (dst_x * m[1] + dst_y * m[4] + m[7]) / z;
}

static vx_bool read_pixel(void *base, vx_imagepatch_addressing_t *addr,
                          vx_float32 x, vx_float32 y, const vx_border_mode_t *borders, vx_uint8 *pixel)
{
    vx_bool out_of_bounds = (x < 0 || y < 0 || x >= addr->dim_x || y >= addr->dim_y);
    vx_uint32 bx, by;
    vx_uint8 *bpixel;
    if (out_of_bounds)
    {
        if (borders->mode == VX_BORDER_MODE_UNDEFINED)
            return vx_false_e;
        if (borders->mode == VX_BORDER_MODE_CONSTANT)
        {
            *pixel = borders->constant_value;
            return vx_true_e;
        }
    }
    
    // bounded x/y
    bx = x < 0 ? 0 : x >= addr->dim_x ? addr->dim_x - 1 : (vx_uint32)x;
    by = y < 0 ? 0 : y >= addr->dim_y ? addr->dim_y - 1 : (vx_uint32)y;
    
    bpixel = vxFormatImagePatchAddress2d(base, bx, by, addr);
    *pixel = *bpixel;
    
    return vx_true_e;
}

static vx_status vxWarpGeneric(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image,
                               const vx_border_mode_t *borders, transform_f transform)
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

    vx_uint32 y = 0u, x = 0u;

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
        for (y = 0u; y < dst_addr.dim_y; y++)
        {
            for (x = 0u; x < dst_addr.dim_x; x++)
            {
                vx_uint8 *dst = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                
                vx_float32 xf;
                vx_float32 yf;
                transform(x, y, m, &xf, &yf);
                xf -= (vx_float32)src_rect.start_x;
                yf -= (vx_float32)src_rect.start_y;
                
                if (type == VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR)
                {
                    read_pixel(src_base, &src_addr, xf, yf, borders, dst);
                }
                else if (type == VX_INTERPOLATION_TYPE_BILINEAR)
                {
                    vx_uint8 tl = 0, tr = 0, bl = 0, br = 0;
                    vx_bool defined = vx_true_e;
                    defined &= read_pixel(src_base, &src_addr, floorf(xf), floorf(yf), borders, &tl);
                    defined &= read_pixel(src_base, &src_addr, floorf(xf) + 1, floorf(yf), borders, &tr);
                    defined &= read_pixel(src_base, &src_addr, floorf(xf), floorf(yf) + 1, borders, &bl);
                    defined &= read_pixel(src_base, &src_addr, floorf(xf) + 1, floorf(yf) + 1, borders, &br);
                    if (defined)
                    {
                        vx_float32 ar = xf - floorf(xf);
                        vx_float32 ab = yf - floorf(yf);
                        vx_float32 al = 1.0f - ar;
                        vx_float32 at = 1.0f - ab;
                        *dst = tl * al * at + tr * ar * at + bl * al * ab + br * ar * ab;
                    }
                }
            }
        }

        /*! \todo compute maximum area rectangle */
    }

    status |= vxWriteMatrix(matrix, m);
    status |= vxCommitImagePatch(src_image, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst_image, &dst_rect, 0, &dst_addr, dst_base);

    return status;
}

vx_status vxWarpPerspective(vx_image src_image, vx_matrix matrix, vx_scalar stype, vx_image dst_image, const vx_border_mode_t *borders)
{
    return vxWarpGeneric(src_image, matrix, stype, dst_image, borders, transform_perspective);
}


static vx_status VX_CALLBACK vxWarpPerspectiveKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  src_image = (vx_image) parameters[0];
        vx_matrix matrix    = (vx_matrix)parameters[1];
        vx_scalar stype     = (vx_scalar)parameters[2];
        vx_image  dst_image = (vx_image) parameters[3];

        vx_border_mode_t borders;
        vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &borders, sizeof(borders));

        return vxWarpPerspective(src_image, matrix, stype, dst_image, &borders);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status vxWarpPerspectiveInputValidator(vx_node node, vx_uint32 index)
{
    vx_size mat_columns = 3;
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

static vx_status VX_CALLBACK vxWarpPerspectiveOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
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

static vx_param_description_t warp_perspective_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_MATRIX, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t warp_perspective_kernel = {
    VX_KERNEL_WARP_PERSPECTIVE,
    "com.machineswithvision.openvx.warp_perspective",
    vxWarpPerspectiveKernel,
    warp_perspective_kernel_params, dimof(warp_perspective_kernel_params),
    vxWarpPerspectiveInputValidator,
    vxWarpPerspectiveOutputValidator,
    NULL,
    NULL,
};
