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
 * \brief The File IO Object Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 * \defgroup group_debug_ext Debugging Extension
 */

#include <VX/vx.h>
#include <VX/vx_lib_debug.h>
#include <VX/vx_helper.h>
//#include <debug_k.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Copy kernel code directly here!

vx_status vxFWriteImage(vx_image input, vx_array file)
{
    vx_char *filename = NULL;
    vx_size filename_stride = 0;
    vx_uint8 *src[4] = {NULL, NULL, NULL, NULL};
    vx_uint32 p, y, sx, ex, sy, ey, width, height;
    vx_size planes;
    vx_imagepatch_addressing_t addr[4];
    vx_df_image format;
    FILE *fp = NULL;
    vx_char *ext = NULL;
    size_t wrote = 0ul;
    vx_rectangle_t rect;
    
    vx_status status = vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
    if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
    {
        vxCommitArrayRange(file, 0, 0, filename);
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
        return VX_FAILURE;
    }
    //VX_PRINT(VX_ZONE_INFO, "filename=%s\n",filename);
    fp = fopen(filename, "wb+");
    if (fp == NULL) {
        vxCommitArrayRange(file, 0, 0, filename);
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n",filename);
        return VX_FAILURE;
    }
    
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH,  &width,  sizeof(width));
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_PLANES, &planes, sizeof(planes));
    status |= vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    
    status |= vxGetValidRegionImage(input, &rect);
    
    sx = rect.start_x;
    sy = rect.start_y;
    ex = rect.end_x;
    ey = rect.end_y;
    
    ext = strrchr(filename, '.');
    if (ext && (strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
    {
        fprintf(fp, "P5\n# %s\n",filename);
        fprintf(fp, "%u %u\n", width, height);
        if (format == VX_DF_IMAGE_U8)
            fprintf(fp, "255\n");
        else if (format == VX_DF_IMAGE_S16)
            fprintf(fp, "65535\n");
        else if (format == VX_DF_IMAGE_U16)
            fprintf(fp, "65535\n");
    }
    for (p = 0u; p < planes; p++)
    {
        status |= vxAccessImagePatch(input, &rect, p, &addr[p], (void **)&src[p], VX_READ_ONLY);
    }
    for (p = 0u; (p < planes) && (status == VX_SUCCESS); p++)
    {
        size_t len = addr[p].stride_x * (addr[p].dim_x * addr[p].scale_x)/VX_SCALE_UNITY;
        for (y = 0u; y < height; y+=addr[p].step_y)
        {
            vx_uint32 i = 0;
            vx_uint8 *ptr = NULL;
            uint8_t value = 0u;
            
            if (y < sy || y >= ey)
            {
                for (i = 0; i < width; ++i) {
                    wrote += fwrite(&value, sizeof(value), 1, fp);
                }
                continue;
            }
            
            for (i = 0; i < sx; ++i)
                wrote += fwrite(&value, sizeof(value), 1, fp);
            
            ptr = vxFormatImagePatchAddress2d(src[p], 0, y - sy, &addr[p]);
            wrote += fwrite(ptr, 1, len, fp);
            
            for (i = 0; i < width - ex; ++i)
                wrote += fwrite(&value, sizeof(value), 1, fp);
        }
        if (wrote == 0)
        {
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write to file!\n");
            status = VX_FAILURE;
            break;
        }
        if (status == VX_FAILURE)
        {
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write image to file correctly\n");
            break;
        }
    }
    for (p = 0u; p < planes; p++)
    {
        status |= vxCommitImagePatch(input, NULL, p, &addr[p], src[p]);
    }
    if (status != VX_SUCCESS)
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to write image to file correctly\n");
    }
    fflush(fp);
    fclose(fp);
    if (vxCommitArrayRange(file, 0, 0, filename) != VX_SUCCESS)
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to release handle to filename array!\n");
    }
    
    return status;
}

vx_status vxFReadImage(vx_array file, vx_image output)
{
    vx_char *filename = NULL;
    vx_size filename_stride = 0;
    vx_uint8 *src = NULL;
    vx_uint32 p = 0u, y = 0u;
    vx_size planes = 0u;
    vx_imagepatch_addressing_t addr = {0};
    vx_df_image format = VX_DF_IMAGE_VIRT;
    FILE *fp = NULL;
    vx_char tmp[VX_MAX_FILE_NAME] = {0};
    vx_char *ext = NULL;
    vx_rectangle_t rect;
    vx_uint32 width = 0, height = 0;
    
    vx_status status = vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
    if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
    {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
        return VX_FAILURE;
    }
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n",filename);
        return VX_FAILURE;
    }
    
    vxQueryImage(output, VX_IMAGE_ATTRIBUTE_PLANES, &planes, sizeof(planes));
    vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
    
    ext = strrchr(filename, '.');
    if (ext && (strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
    {
        FGETS(tmp, fp); // PX
        FGETS(tmp, fp); // comment
        FGETS(tmp, fp); // W H
        sscanf(tmp, "%u %u", &width, &height);
        FGETS(tmp, fp); // BPP
        // ! \todo double check image size?
    }
    else if (ext && (strcmp(ext, ".yuv") == 0 ||
                     strcmp(ext, ".rgb") == 0 ||
                     strcmp(ext, ".bw") == 0))
    {
        sscanf(filename, "%*[^_]_%ux%u_%*s", &width, &height);
    }
    
    rect.start_x = rect.start_y = 0;
    rect.end_x = width;
    rect.end_y = height;
    
    for (p = 0; p < planes; p++)
    {
        status = vxAccessImagePatch(output, &rect, p, &addr, (void **)&src, VX_WRITE_ONLY);
        if (status == VX_SUCCESS)
        {
            for (y = 0; y < addr.dim_y; y+=addr.step_y)
            {
                vx_uint8 *srcp = vxFormatImagePatchAddress2d(src, 0, y, &addr);
                vx_size len = ((addr.dim_x * addr.scale_x)/VX_SCALE_UNITY);
                vx_size rlen = fread(srcp, addr.stride_x, len, fp);
                if (rlen != len)
                {
                    status = VX_FAILURE;
                    break;
                }
            }
            if (status == VX_SUCCESS)
            {
                status = vxCommitImagePatch(output, &rect, p, &addr, src);
            }
            if (status != VX_SUCCESS)
            {
                break;
            }
        }
    }
    fclose(fp);
    vxCommitArrayRange(file, 0, 0, filename);
    
    return status;
}

static vx_status VX_CALLBACK vxFWriteImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 2)
    {
        vx_image input = (vx_image)parameters[0];
        vx_array file = (vx_array)parameters[1];
        status = vxFWriteImage(input, file);
    }
    return status;
}

static vx_status VX_CALLBACK vxFWriteArrayKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 2)
    {
        vx_array arr = (vx_array)parameters[0];
        vx_array file = (vx_array)parameters[1];

        vx_size num_items, item_size, stride;
        void *arrptr = NULL;
        vx_char *filename = NULL;
        vx_size filename_stride = 0;
        FILE *fp = NULL;
        vx_size i;

        status = vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
        if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
        {
            vxCommitArrayRange(file, 0, 0, filename);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
            return VX_FAILURE;
        }

        fp = fopen(filename, "wb+");
        if (fp == NULL) {
            vxCommitArrayRange(file, 0, 0, filename);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n", filename);
            return VX_FAILURE;
        }

        vxQueryArray(arr, VX_ARRAY_ATTRIBUTE_NUMITEMS, &num_items, sizeof(num_items));
        vxQueryArray(arr, VX_ARRAY_ATTRIBUTE_ITEMSIZE, &item_size, sizeof(item_size));
        status = vxAccessArrayRange(arr, 0, num_items, &stride, &arrptr, VX_READ_ONLY);

        if (status == VX_SUCCESS)
        {
            if (fwrite(&num_items, sizeof(num_items), 1, fp) == sizeof(num_items) &&
                fwrite(&item_size, sizeof(item_size), 1, fp) == sizeof(item_size))
            {
                for (i = 0; i < num_items; ++i)
                {
                    if (fwrite(vxFormatArrayPointer(arrptr, i, stride), item_size, 1, fp) != item_size)
                    {
                        status = VX_FAILURE;
                        break;
                    }
                }
            }
            else
            {
                status = VX_FAILURE;
            }

            vxCommitArrayRange(arr, 0, 0, arrptr);
        }

        fclose(fp);
        vxCommitArrayRange(file, 0, 0, filename);
    }
    return status;
}

static vx_status VX_CALLBACK vxFReadImageKernel(vx_node node, const vx_reference parameters[], vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 2)
    {
        vx_array file = (vx_array)parameters[0];
        vx_image output = (vx_image)parameters[1];
        status = vxFReadImage(file, output);
    }
    return status;
}

static vx_status VX_CALLBACK vxFReadArrayKernel(vx_node node, const vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 2)
    {
        vx_array file = (vx_array)parameters[0];
        vx_array arr = (vx_array)parameters[1];

        vx_size num_items, item_size, arr_capacity, arr_item_size;
        vx_char *filename = NULL;
        vx_size filename_stride = 0;
        FILE *fp = NULL;

        status = vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
        if (status != VX_SUCCESS || filename_stride != sizeof(vx_char))
        {
            vxCommitArrayRange(file, 0, 0, filename);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
            return VX_FAILURE;
        }

        fp = fopen(filename, "wb+");
        if (fp == NULL) {
            vxCommitArrayRange(file, 0, 0, filename);
            vxAddLogEntry((vx_reference)file, VX_FAILURE, "Failed to open file %s\n", filename);
            return VX_FAILURE;
        }

        if (fread(&num_items, sizeof(num_items), 1, fp) == sizeof(num_items) &&
            fread(&item_size, sizeof(item_size), 1, fp) == sizeof(item_size))
        {
            vxQueryArray(arr, VX_ARRAY_ATTRIBUTE_CAPACITY, &arr_capacity, sizeof(arr_capacity));
            vxQueryArray(arr, VX_ARRAY_ATTRIBUTE_ITEMSIZE, &arr_item_size, sizeof(arr_item_size));

            if (arr_capacity >= num_items && arr_item_size == item_size)
            {
                void *tmpbuf = malloc(num_items * item_size);
                if (tmpbuf)
                {
                    if (fread(tmpbuf, item_size, num_items, fp) == (num_items * item_size))
                    {
                        status = VX_SUCCESS;
                        status |= vxTruncateArray(arr, 0);
                        status |= vxAddArrayItems(arr, num_items, tmpbuf, 0);
                    }
                    free(tmpbuf);
                }
                else
                {
                    status = VX_ERROR_NO_MEMORY;
                }
            }
            else
            {
                vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect destination array "VX_FMT_REF"\n", arr);
                status = VX_FAILURE;
            }
        }
        else
        {
            status = VX_FAILURE;
        }

        fclose(fp);
        vxCommitArrayRange(file, 0, 0, filename);
    }
    return status;
}

static vx_status VX_CALLBACK vxFWriteImageInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if(param)
        {
            vx_image img = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &img, sizeof(img));
            if (img)
            {
                vx_df_image formats[] = {VX_DF_IMAGE_U8, VX_DF_IMAGE_S16, VX_DF_IMAGE_U16, VX_DF_IMAGE_U32, VX_DF_IMAGE_UYVY, VX_DF_IMAGE_YUYV, VX_DF_IMAGE_IYUV, VX_DF_IMAGE_RGB};
                vx_df_image format = 0;
                vx_uint32 i = 0;
                vxQueryImage(img, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                for (i = 0; i < dimof(formats); i++)
                {
                    if (formats[i] == format)
                    {
                        status = VX_SUCCESS;
                        break;
                    }
                }
                vxReleaseImage(&img);
            }
            vxReleaseParameter(&param);
        }
    }
    else if(index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if(param)
        {
            vx_array file = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &file, sizeof(file));
            if(file)
            {
                vx_char *filename = NULL;
                vx_size filename_stride = 0;
                status = vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
                if (status == VX_SUCCESS && filename_stride == sizeof(vx_char))
                {
                    if (strncmp(filename, "", VX_MAX_FILE_NAME) == 0)
                    {
                        vxAddLogEntry((vx_reference)node, VX_FAILURE, "Empty file name. %s\n",filename);
                        status = VX_ERROR_INVALID_VALUE;
                    }
                    else
                    {
                        status = VX_SUCCESS;
                    }
                    vxCommitArrayRange(file, 0, 0, filename);
                }
                vxReleaseArray(&file);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxReadImageInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_array file = 0;
            vx_char *filename = NULL;
            vx_size filename_stride = 0;

            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &file, sizeof(file));
            if (file)
            {
                vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
                if (filename_stride == sizeof(vx_char))
                {
                    if (strncmp(filename, "",VX_MAX_FILE_NAME) == 0)
                    {
                        vxAddLogEntry((vx_reference)node, VX_FAILURE, "Empty file name. %s\n",filename);
                        status = VX_ERROR_INVALID_VALUE;
                    }
                    else
                    {
                        status = VX_SUCCESS;
                    }
                }
                vxCommitArrayRange(file, 0, VX_MAX_FILE_NAME, filename);
                vxReleaseArray(&file);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxReadImageOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if(index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_array file = 0;
            vx_char *filename = NULL;
            vx_size filename_stride = 0;
            vx_char *ext = NULL;
            FILE *fp = NULL;
            vx_char tmp[VX_MAX_FILE_NAME];
            vx_uint32 width = 0, height = 0;
            vx_df_image format = VX_DF_IMAGE_VIRT;

            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &file, sizeof(file));

            if (file)
            {
                //find metadata
                vxAccessArrayRange(file, 0, VX_MAX_FILE_NAME, &filename_stride, (void **)&filename, VX_READ_ONLY);
                if (filename_stride != sizeof(vx_char))
                {
                    vxCommitArrayRange(file, 0, 0, filename);
                    vxAddLogEntry((vx_reference)file, VX_FAILURE, "Incorrect array "VX_FMT_REF"\n", file);
                    return VX_FAILURE;
                }
                fp = fopen(filename, "rb");
                if (fp == NULL) {
                    vxAddLogEntry((vx_reference)node, VX_FAILURE, "Failed to open file %s\n",filename);
                    return VX_FAILURE;
                }
                ext = strrchr(filename, '.');
                if (ext)
                {
                    vx_uint32 depth = 0;
                    if ((strcmp(ext, ".pgm") == 0 || strcmp(ext, ".PGM") == 0))
                    {
                        FGETS(tmp, fp); // PX
                        FGETS(tmp, fp); // comment
                        FGETS(tmp, fp); // W H
                        sscanf(tmp, "%u %u", &width, &height);
                        FGETS(tmp, fp); // BPP
                        sscanf(tmp, "%u", &depth);
                        if (UINT8_MAX == depth)
                            format = VX_DF_IMAGE_U8;
                        else if (INT16_MAX == depth)
                            format = VX_DF_IMAGE_S16;
                        else if (UINT16_MAX == depth)
                            format = VX_DF_IMAGE_U16;
                    }
                    else if (strcmp(ext, ".bw") == 0)
                    {
                        vx_char shortname[256] = {0};
                        vx_char fmt[5] = {0};
                        vx_int32 cbps = 0;
                        sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.bw", shortname, &width, &height, fmt, &cbps);
                        if (strcmp(fmt,"P400") == 0)
                        {
                            format = VX_DF_IMAGE_U8;
                        }
                    }
                    else if (strcmp(ext, ".yuv") == 0)
                    {
                        vx_char shortname[256] = {0};
                        vx_char fmt[5] = {0};
                        vx_int32 cbps = 0;
                        sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.bw", shortname, &width, &height, fmt, &cbps);
                        if (strcmp(fmt,"IYUV") == 0)
                        {
                            format = VX_DF_IMAGE_IYUV;
                        }
                        else if (strcmp(fmt,"UYVY") == 0)
                        {
                            format = VX_DF_IMAGE_UYVY;
                        }
                        else if (strcmp(fmt,"P444") == 0)
                        {
                            format = VX_DF_IMAGE_YUV4;
                        }
                        else if (strcmp(fmt, "YUY2") == 0)
                        {
                            format = VX_DF_IMAGE_YUYV;
                        }
                    }
                    else if (strcmp(ext, ".rgb") == 0)
                    {
                        vx_char shortname[256] = {0};
                        vx_char fmt[5] = {0};
                        vx_int32 cbps = 0;
                        sscanf(filename, "%256[a-zA-Z]_%ux%u_%4[A-Z0-9]_%db.rgb", shortname, &width, &height, fmt, &cbps);
                        if (strcmp(fmt,"I444") == 0)
                        {
                            format = VX_DF_IMAGE_RGB;
                        }
                    }
                }

                status = VX_SUCCESS;

                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxSetMetaFormatAttribute(meta, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));

                fclose(fp);
                vxCommitArrayRange(file, 0, 0, filename);
                vxReleaseArray(&file);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxAllPassInputValidator(vx_node node, vx_uint32 index)
{
    return VX_SUCCESS;
}

static vx_status VX_CALLBACK vxAllPassOutputValidator(vx_node node, vx_uint32 index, vx_meta_format meta)
{
    return VX_SUCCESS;
}

/*! \brief Declares the parameter types for \ref vxFWriteImageNode.
  * \ingroup group_debug_ext
  */
static vx_param_description_t fwriteimage_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
};

/*! \brief Declares the parameter types for \ref vxFWriteArrayNode.
 * \ingroup group_debug_ext
 */
static vx_param_description_t fwritearray_kernel_params[] = {
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
};

/*! \brief Declares the parameter types for \ref vxFReadImageNode.
  * \ingroup group_debug_ext
  */
static vx_param_description_t freadimage_kernel_params[] = {
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

/*! \brief Declares the parameter types for \ref vxFReadArrayNode.
 * \ingroup group_debug_ext
 */
static vx_param_description_t freadarray_kernel_params[] = {
    {VX_INPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_ARRAY, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t fwriteimage_kernel = {
    VX_KERNEL_DEBUG_FWRITE_IMAGE,
    "org.khronos.debug.fwrite_image",
    vxFWriteImageKernel,
    fwriteimage_kernel_params, dimof(fwriteimage_kernel_params),
    vxFWriteImageInputValidator,
    vxAllPassOutputValidator,
    NULL, NULL,
};

vx_kernel_description_t fwritearray_kernel = {
    VX_KERNEL_DEBUG_FWRITE_ARRAY,
    "org.khronos.debug.fwrite_array",
    vxFWriteArrayKernel,
    fwritearray_kernel_params, dimof(fwritearray_kernel_params),
    vxAllPassInputValidator,
    vxAllPassOutputValidator,
    NULL, NULL,
};

vx_kernel_description_t freadimage_kernel = {
    VX_KERNEL_DEBUG_FREAD_IMAGE,
    "org.khronos.debug.fread_image",
    vxFReadImageKernel,
    freadimage_kernel_params, dimof(freadimage_kernel_params),
    vxReadImageInputValidator,
    vxReadImageOutputValidator,
    NULL, NULL,
};

vx_kernel_description_t freadarray_kernel = {
    VX_KERNEL_DEBUG_FREAD_ARRAY,
    "org.khronos.debug.fread_array",
    vxFReadArrayKernel,
    freadarray_kernel_params, dimof(freadarray_kernel_params),
    vxAllPassInputValidator,
    vxAllPassOutputValidator,
    NULL, NULL,
};
