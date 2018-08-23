/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

///
#include <stdio.h>

#define PERMUTATIONS 16
#define APERTURE 3

static vx_uint8 indexes[PERMUTATIONS][9] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8},
    {15, 0, 1, 2, 3, 4, 5, 6, 7},
    {14,15, 0, 1, 2, 3, 4, 5, 6},
    {13,14,15, 0, 1, 2, 3, 4, 5},
    {12,13,14,15, 0, 1, 2, 3, 4},
    {11,12,13,14,15, 0, 1, 2, 3},
    {10,11,12,13,14,15, 0, 1, 2},
    { 9,10,11,12,13,14,15, 0, 1},
    { 8, 9,10,11,12,13,14,15, 0},
    { 7, 8, 9,10,11,12,13,14,15},
    { 6, 7, 8, 9,10,11,12,13,14},
    { 5, 6, 7, 8, 9,10,11,12,13},
    { 4, 5, 6, 7, 8, 9,10,11,12},
    { 3, 4, 5, 6, 7, 8, 9,10,11},
    { 2, 3, 4, 5, 6, 7, 8, 9,10},
    { 1, 2, 3, 4, 5, 6, 7, 8, 9},
};

/* offsets from "p" */
static vx_int32 offsets[16][2] = {
    {  0, -3},
    {  1, -3},
    {  2, -2},
    {  3, -1},
    {  3,  0},
    {  3,  1},
    {  2,  2},
    {  1,  3},
    {  0,  3},
    { -1,  3},
    { -2,  2},
    { -3,  1},
    { -3,  0},
    { -3, -1},
    { -2, -2},
    { -1, -3},
};


static vx_bool vxIsFastCorner(const vx_uint8* buf, vx_uint8 p, vx_uint8 tolerance)
{
    vx_int32 i, a;
    for (a = 0; a < PERMUTATIONS; a++)
    {
        vx_bool isacorner = vx_true_e;
        for (i = 0; i < dimof(indexes[a]); i++)
        {
            vx_uint8 j = indexes[a][i];
            vx_uint8 v = buf[j];
            if (v <= (p + tolerance))
            {
                isacorner = vx_false_e;
            }
        }
        if (isacorner == vx_true_e)
            return isacorner;
        isacorner = vx_true_e;
        for (i = 0; i < dimof(indexes[a]); i++)
        {
            vx_uint8 j = indexes[a][i];
            vx_uint8 v = buf[j];
            if (v >= (p - tolerance))
            {
                isacorner = vx_false_e;
            }
        }
        if (isacorner == vx_true_e)
            return isacorner;
    }
    return vx_false_e;
}


static vx_uint8 vxGetFastCornerStrength(vx_int32 x, vx_int32 y, void* src_base,
                                        vx_imagepatch_addressing_t* src_addr,
                                        vx_uint8 tolerance)
{
    if (x < APERTURE || y < APERTURE || x >= src_addr->dim_x - APERTURE || y >= src_addr->dim_y - APERTURE)
        return 0;
    {
        vx_uint8 p = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x, y, src_addr);
        vx_uint8 buf[16];
        vx_int32 j;
        vx_uint8 a, b = 255;
        
        for (j = 0; j < 16; j++)
        {
            buf[j] = *(vx_uint8*)vxFormatImagePatchAddress2d(src_base, x+offsets[j][0], y+offsets[j][1], src_addr);
        }
        
        if (!vxIsFastCorner(buf, p, tolerance))
            return 0;
        
        a = tolerance;
        while (b - a > 1)
        {
            vx_uint8 c = (a + b)/2;
            if (vxIsFastCorner(buf, p, c))
                a = c;
            else
                b = c;
        }
        return a;
    }
}

// nodeless version of the Fast9Corners kernel
vx_status vxFast9Corners(vx_image src, vx_scalar sens, vx_scalar nonm, vx_array points,
                         vx_scalar s_num_corners, vx_border_mode_t *bordermode)
{
    vx_float32 b = 0.0f;
    vx_imagepatch_addressing_t src_addr;
    void *src_base = NULL;
    vx_rectangle_t rect;
    vx_uint8 tolerance = 0;
    vx_bool do_nonmax;
    vx_uint32 num_corners = 0;
    vx_size dst_capacity = 0;
    vx_keypoint_t kp;
    
    vx_status status = vxGetValidRegionImage(src, &rect);
    status |= vxReadScalarValue(sens, &b);
    //status |= vxReadScalarValue(nonm, &do_nonmax); // TODO: fix issue here
    /* remove any pre-existing points */
    status |= vxTruncateArray(points, 0);
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    tolerance = (vx_uint8)b;
    status |= vxQueryArray(points, VX_ARRAY_ATTRIBUTE_CAPACITY, &dst_capacity, sizeof(dst_capacity));
    
    memset(&kp, 0, sizeof(kp));
    
    if (status == VX_SUCCESS)
    {
        /*! \todo implement other Fast9 Corners border modes */
        if (bordermode->mode == VX_BORDER_MODE_UNDEFINED)
        {
            vx_int32 y, x;
            for (y = APERTURE; y < (vx_int32)(src_addr.dim_y - APERTURE); y++)
            {
                for (x = APERTURE; x < (vx_int32)(src_addr.dim_x - APERTURE); x++)
                {
                    vx_uint8 strength = vxGetFastCornerStrength(x, y, src_base, &src_addr, tolerance);
                    if (strength > 0)
                    {
                        if (do_nonmax)
                        {
                            if (strength >= vxGetFastCornerStrength(x-1, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x+1, y-1, src_base, &src_addr, tolerance) &&
                                strength >= vxGetFastCornerStrength(x-1, y, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x+1, y, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x-1, y+1, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x, y+1, src_base, &src_addr, tolerance) &&
                                strength >  vxGetFastCornerStrength(x+1, y+1, src_base, &src_addr, tolerance))
                                ;
                            else
                                continue;
                        }
                        if (num_corners < dst_capacity)
                        {
                            kp.x = x;
                            kp.y = y;
                            kp.strength = strength;
                            status |= vxAddArrayItems(points, 1, &kp, 0);
                        }
                        num_corners++;
                    }
                }
            }
        }
        else
        {
            status = VX_ERROR_NOT_IMPLEMENTED;
        }
        if (s_num_corners)
            status |= vxWriteScalarValue(s_num_corners, &num_corners);
        status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    }
    
    return status;
}


static vx_status VX_CALLBACK vxFast9CornersKernel(vx_node node, vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 5)
    {
        vx_border_mode_t bordermode;
        vx_image src = (vx_image)parameters[0];
        vx_scalar sens = (vx_scalar)parameters[1];
        //vx_bool nonm = (vx_bool)parameters[2];
        vx_scalar nonm = (vx_bool)parameters[2];
        vx_array points = (vx_array)parameters[3];
        vx_scalar num_corners = (vx_scalar)parameters[4];
       
        status = vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxFast9Corners(src, sens, nonm, points, num_corners, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxFast9InputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    status = VX_SUCCESS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image input = 0;
            status = vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if ((status == VX_SUCCESS) && (input))
            {
                vx_df_image format = 0;
                status = vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if ((status == VX_SUCCESS) && (format == VX_DF_IMAGE_U8))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar sens = 0;
            status = vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &sens, sizeof(sens));
            if ((status == VX_SUCCESS) && (sens))
            {
                vx_enum type = VX_TYPE_INVALID;
                vxQueryScalar(sens, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_FLOAT32)
                {
                    vx_float32 k = 0.0f;
                    status = vxReadScalarValue(sens, &k);
                    if ((status == VX_SUCCESS) && (k > 0) && (k < 256))
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
                vxReleaseScalar(&sens);
            }
            vxReleaseParameter(&param);
        }
    }
    if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar s_nonmax = 0;
            status = vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &s_nonmax, sizeof(s_nonmax));
            if ((status == VX_SUCCESS) && (s_nonmax))
            {
                vx_enum type = VX_TYPE_INVALID;
                vxQueryScalar(s_nonmax, VX_SCALAR_ATTRIBUTE_TYPE, &type, sizeof(type));
                if (type == VX_TYPE_BOOL)
                {
                    vx_bool nonmax = vx_false_e;
                    status = vxReadScalarValue(s_nonmax, &nonmax);
                    if ((status == VX_SUCCESS) && ((nonmax == vx_false_e) ||
                                                   (nonmax == vx_true_e)))
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
                vxReleaseScalar(&s_nonmax);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxFast9OutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        ptr->type = VX_TYPE_ARRAY;
        ptr->dim.array.item_type = VX_TYPE_KEYPOINT;
        ptr->dim.array.capacity = 0; // no defined capacity requirement // NOTE: this is illdefined (not in spec)
        status = VX_SUCCESS;
    }
    else if (index == 4)
    {
        ptr->dim.scalar.type = VX_TYPE_SIZE; // NOTE: correction as per 1.0.1. spec - was VX_TYPE_UINT32;
        status = VX_SUCCESS;
    }
    return status;
}

static vx_param_description_t fast9_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t fast9_kernel = {
    VX_KERNEL_FAST_CORNERS,
    "com.machineswithvision.openvx.fast_corners",
    vxFast9CornersKernel,
    fast9_kernel_params, dimof(fast9_kernel_params),
    vxFast9InputValidator,
    vxFast9OutputValidator,
};
