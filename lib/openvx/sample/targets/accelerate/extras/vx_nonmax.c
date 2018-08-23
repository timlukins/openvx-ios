/*
 * Copyright (c) 2012-2014 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

/*!
 * \file
 * \brief The Non-Maxima Suppression Kernel (Extras)
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <VX/vx.h>
#include <VX/vx_lib_extras.h>
#include <VX/vx_helper.h>
#include <math.h>
#include <stdlib.h>
//#include <extras_k.h>

struct kp_elem {
    int x;
    int y;
    vx_float32 resp;
};

void swap(struct kp_elem *x, struct kp_elem *y)
{
    struct kp_elem temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

int choose_pivot(int i,int j )
{
    return((i+j) /2);
}

void quicksort(struct kp_elem list[], int m, int n)
{
    int i, j, k;
    struct kp_elem key;
    if ( m < n )
    {
        k = choose_pivot(m,n);
        swap(&list[m],&list[k]);
        key = list[m];
        i = m+1;
        j = n;
        while(i <= j)
        {
            while((i <= n) && (list[i].resp >= key.resp))
                i++;
            while((j >= m) && (list[j].resp < key.resp))
                j--;
            if( i < j)
                swap(&list[i],&list[j]);
        }
        /* swap two elements */
        swap(&list[m],&list[j]);
        
        /* recursively sort the lesser list */
        quicksort(list,m,j-1);
        quicksort(list,j+1,n);
    }
}

// nodeless version of the EuclideanNonMaxSuppression kernel
vx_status vxEuclideanNonMaxSuppressionHarris(vx_image src, vx_scalar thr, vx_scalar rad, vx_image dst)
{
    vx_status status = VX_SUCCESS;
    void *src_base = NULL, *dst_base = NULL;
    vx_imagepatch_addressing_t src_addr, dst_addr;
    vx_float32 radius = 0.0f;
    vx_int32 r = 0;
    vx_float32 thresh = 0;
    vx_rectangle_t rect;
    vx_df_image format = VX_DF_IMAGE_VIRT;
    
    status = vxGetValidRegionImage(src, &rect);
    status |= vxReadScalarValue(rad, &radius);
    status |= vxReadScalarValue(thr, &thresh);
    status |= vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
    r = (vx_uint32)radius;
    r = (r <=0 ? 1 : r);
    if (status == VX_SUCCESS)
    {
        vx_int32 y, x;
        vx_int32 i, j;
        vx_float32 d = 0;
        int n = 0;
        
        struct kp_elem *kp_list = (struct kp_elem *)malloc(src_addr.dim_x*src_addr.dim_y*sizeof(struct kp_elem));
        int nb_kp = 0;
        
        for (y = 0; y < src_addr.dim_y; y++){
            for (x = 0; x < src_addr.dim_x; x++) {
                // Init to 0
                vx_int32 *out = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *out = 0;
                
                // Fast non max suppression & keypoint list building
                if ( (x>0) && (x<src_addr.dim_x-1) &&
                    (y>0) && (y<src_addr.dim_y-1) ) {
                    vx_float32 *ptr = vxFormatImagePatchAddress2d(src_base, x, y, &src_addr);
                    vx_float32 *ptr99 = vxFormatImagePatchAddress2d(src_base, x-1, y-1, &src_addr);
                    vx_float32 *ptr90 = vxFormatImagePatchAddress2d(src_base, x, y-1, &src_addr);
                    vx_float32 *ptr91 = vxFormatImagePatchAddress2d(src_base, x+1, y-1, &src_addr);
                    vx_float32 *ptr09 = vxFormatImagePatchAddress2d(src_base, x-1, y, &src_addr);
                    vx_float32 *ptr01 = vxFormatImagePatchAddress2d(src_base, x+1, y, &src_addr);
                    vx_float32 *ptr19 = vxFormatImagePatchAddress2d(src_base, x-1, y+1, &src_addr);
                    vx_float32 *ptr10 = vxFormatImagePatchAddress2d(src_base, x, y+1, &src_addr);
                    vx_float32 *ptr11 = vxFormatImagePatchAddress2d(src_base, x+1, y+1, &src_addr);
                    if ( (*ptr >= thresh) &&
                        ((*ptr >= *ptr99) &&
                         (*ptr >= *ptr90) &&
                         (*ptr >= *ptr91) &&
                         (*ptr >= *ptr09) &&
                         (*ptr > *ptr01) &&
                         (*ptr > *ptr19) &&
                         (*ptr > *ptr10) &&
                         (*ptr > *ptr11))
                        ) {
                        kp_list[nb_kp].x = x;
                        kp_list[nb_kp].y = y;
                        kp_list[nb_kp].resp = *ptr;
                        nb_kp++;
                    }
                }
            }
        }
        
        // Sort keypoints
        quicksort(kp_list, 0,nb_kp-1);
        
        // Enclidean norm
        for(n = 0; n<nb_kp; n++) {
            int found = 0;
            x = kp_list[n].x;
            y = kp_list[n].y;
            //printf("src(%d,%d) = %d > %d (r2=%u)\n",x,y,*ptr,threshold,r2);
            for (j = -r; j <= r; j++) {
                if ((y+j >= 0) && (y+j < src_addr.dim_y)) {
                    for (i = -r; i <= r; i++) {
                        if ((x+i >= 0) && (x+i < src_addr.dim_x)) {
                            vx_float32 dx = i;
                            vx_float32 dy = j;
                            d = sqrtf((dx*dx) + (dy*dy));
                            //printf("{%d,%d} is %lf from {%d,%d} radius=%lf\n",x+i,y+j,d,x,y,radius);
                            if (d < radius) {
                                vx_float32 *non = vxFormatImagePatchAddress2d(dst_base, x+i, y+j, &dst_addr);
                                if (*non) {
                                    found = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (found == 0) {
                vx_float32 *out = vxFormatImagePatchAddress2d(dst_base, x, y, &dst_addr);
                *out = kp_list[n].resp;
            }
        }
        free(kp_list);
    }
    status |= vxCommitImagePatch(src, NULL, 0, &src_addr, src_base);
    status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);
    
    return status;
}


static const int neighbor_indexes[][2] = {
    {3, 5},
    {6, 2},
    {7, 1},
    {8, 0},
    {5, 3},
    {2, 6},
    {1, 7},
    {0, 8},
    {3, 5},
};

// nodeless version of the NonMaxSuppression kernel
vx_status vxNonMaxSuppression(vx_image i_mag, vx_image i_ang, vx_image i_edge, vx_border_mode_t *borders)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 y = 0, x = 0;
    void *mag_base = NULL;
    void *ang_base = NULL;
    void *edge_base = NULL;
    vx_imagepatch_addressing_t mag_addr, ang_addr, edge_addr;
    vx_rectangle_t rect;
    vx_df_image format = 0;
    vx_uint32 low_x = 0, high_x;
    vx_uint32 low_y = 0, high_y;
    
    status  = VX_SUCCESS; // assume success until an error occurs.
    status |= vxQueryImage(i_mag, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    status |= vxGetValidRegionImage(i_mag, &rect);
    status |= vxAccessImagePatch(i_mag, &rect, 0, &mag_addr, &mag_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(i_ang, &rect, 0, &ang_addr, &ang_base, VX_READ_ONLY);
    status |= vxAccessImagePatch(i_edge, &rect, 0, &edge_addr, &edge_base, VX_WRITE_ONLY);
    
    high_x = edge_addr.dim_x;
    high_y = edge_addr.dim_y;
    
    if (borders->mode == VX_BORDER_MODE_UNDEFINED)
    {
        ++low_x; --high_x;
        ++low_y; --high_y;
        vxAlterRectangle(&rect, 1, 1, -1, -1);
    }
    
    for (y = low_y; y < high_y; y++)
    {
        for (x = low_x; x < high_x; x++)
        {
            vx_uint8 *ang = vxFormatImagePatchAddress2d(ang_base, x, y, &ang_addr);
            vx_uint8 angle = *ang;
            if (format == VX_DF_IMAGE_U8)
            {
                vx_uint8 mag[9];
                vx_uint8 *edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                const int *ni = neighbor_indexes[(angle + 16) / 32];
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
            else if (format == VX_DF_IMAGE_S16)
            {
                vx_int16 mag[9];
                vx_int16 *edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                const int *ni = neighbor_indexes[(angle + 16) / 32];
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
            else if (format == VX_DF_IMAGE_U16)
            {
                vx_uint16 mag[9];
                vx_uint16 *edge = vxFormatImagePatchAddress2d(edge_base, x, y, &edge_addr);
                const int *ni = neighbor_indexes[(angle + 16) / 32];
                vxReadRectangle(mag_base, &mag_addr, borders, format, x, y, 1, 1, mag);
                *edge = mag[4] > mag[ni[0]] && mag[4] > mag[ni[1]] ? mag[4] : 0;
            }
        }
    }
    
    status |= vxCommitImagePatch(i_mag, NULL, 0, &mag_addr, mag_base);
    status |= vxCommitImagePatch(i_ang, NULL, 0, &ang_addr, ang_base);
    status |= vxCommitImagePatch(i_edge, &rect, 0, &edge_addr, edge_base);
    
    return status;
}



static vx_status VX_CALLBACK vxEuclideanNonMaxSuppressionHarrisKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == 4)
    {
        vx_image  src = (vx_image) parameters[0];
        vx_scalar thr = (vx_scalar)parameters[1];
        vx_scalar rad = (vx_scalar)parameters[2];
        vx_image  dst = (vx_image) parameters[3];
        return vxEuclideanNonMaxSuppressionHarris(src, thr, rad, dst);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxNonMaxSuppressionKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    if (num == 3)
    {
        vx_image i_mag  = (vx_image)parameters[0];
        vx_image i_ang  = (vx_image)parameters[1];
        vx_image i_edge = (vx_image)parameters[2];
        vx_border_mode_t borders;
        vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &borders, sizeof(borders));
        return vxNonMaxSuppression(i_mag, i_ang, i_edge, &borders);
    }
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxEuclideanNonMaxHarrisInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0) /* image */
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &img, sizeof(img));
            if (img)
            {
                vx_df_image format = VX_DF_IMAGE_VIRT;
                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if (format == VX_DF_IMAGE_F32)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&img);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 1) /* strength_thresh */
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
                if (stype == VX_TYPE_FLOAT32)
                {
                    status = VX_SUCCESS;
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
    else if (index == 2) /* min_distance */
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
                if (stype == VX_TYPE_FLOAT32)
                {
                    vx_float32 radius = 0;
                    vxReadScalarValue(scalar, &radius);
                    if ((0.0 <= radius) && (radius <= 30.0))
                    {
                        status = VX_SUCCESS;
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

static vx_status VX_CALLBACK vxNonMaxSuppressionInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0) /* magnitude */
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &img, sizeof(img));
            if (img)
            {
                vx_df_image format = VX_DF_IMAGE_VIRT;
                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if (format == VX_DF_IMAGE_U8 || format == VX_DF_IMAGE_S16 || format == VX_DF_IMAGE_U16)
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&img);
            }
            vxReleaseParameter(&param);
        }
    }
    if (index == 1)
    {
        vx_parameter params[] = {
            vxGetParameterByIndex(node, 0),
            vxGetParameterByIndex(node, 1),
        };
        if (params[0] && params[1])
        {
            vx_image images[] = {0, 0};

            vxQueryParameter(params[0], VX_PARAMETER_ATTRIBUTE_REF, &images[0], sizeof(images[0]));
            vxQueryParameter(params[1], VX_PARAMETER_ATTRIBUTE_REF, &images[1], sizeof(images[1]));
            if (images[0] && images[1])
            {
                vx_uint32 width[2], height[2];
                vx_df_image format = VX_DF_IMAGE_VIRT;
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_WIDTH, &width[0], sizeof(width[0]));
                vxQueryImage(images[0], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[0], sizeof(height[0]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_WIDTH, &width[1], sizeof(width[1]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_HEIGHT, &height[1], sizeof(height[1]));
                vxQueryImage(images[1], VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                if ((format == VX_DF_IMAGE_U8) &&
                    (width[0] == width[1]) &&
                    (height[0] == height[1]))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseImage(&images[0]);
                vxReleaseImage(&images[1]);
            }
            vxReleaseParameter(&params[0]);
            vxReleaseParameter(&params[1]);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxNonMaxSuppressionOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &img, sizeof(img));
            if (img)
            {
                vx_df_image format = VX_DF_IMAGE_VIRT;
                vx_uint32 width, height;

                status = VX_SUCCESS;
                status |= vxQueryImage(img, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                status |= vxQueryImage(img, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                status |= vxQueryImage(img, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                status |= vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                status |= vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                status |= vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxReleaseImage(&img);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxEuclideanNonMaxHarrisOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &img, sizeof(img));
            if (img)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT; /* takes the format of the input image */

                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                vxReleaseImage(&img);

                status = VX_SUCCESS;
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t nonmaxsuppression_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

static vx_param_description_t euclidean_non_max_suppression_harris_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, /* strength_thresh */
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED}, /* min_distance */
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t nonmax_kernel = {
    VX_KERNEL_EXTRAS_NONMAXSUPPRESSION,
    "org.khronos.extra.nonmaximasuppression",
    vxNonMaxSuppressionKernel,
    nonmaxsuppression_params, dimof(nonmaxsuppression_params),
    vxNonMaxSuppressionInputValidator,
    vxNonMaxSuppressionOutputValidator,
    NULL,
    NULL,
};

vx_kernel_description_t euclidian_nonmax_harris_kernel = {
    VX_KERNEL_EXTRAS_EUCLIDEAN_NONMAXSUPPRESSION_HARRIS,
    "org.khronos.extra.euclidean_nonmaxsuppression_harris",
    vxEuclideanNonMaxSuppressionHarrisKernel,
    euclidean_non_max_suppression_harris_params, dimof(euclidean_non_max_suppression_harris_params),
    vxEuclideanNonMaxHarrisInputValidator,
    vxEuclideanNonMaxHarrisOutputValidator,
    NULL,
    NULL,
};
