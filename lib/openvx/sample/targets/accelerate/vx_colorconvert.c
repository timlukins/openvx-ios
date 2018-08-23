/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxColorConvert(vx_image src, vx_image dst)
{

	// NOTE: again, in sample implementation this is a function defined
	// in the ./kernels/c_model/c_convertcolor.c - just defining it here for now...
	//return vxConvertColor(src, dst);
	vx_imagepatch_addressing_t src_addr[4], dst_addr[4];
	void *src_base[4] = {NULL};
	void *dst_base[4] = {NULL};
	vx_uint32 p;
	vx_df_image src_format, dst_format;
	vx_size src_planes, dst_planes;
	vx_enum src_space;
	vx_rectangle_t rect;

	vx_status status = VX_SUCCESS;
	status |= vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &src_format, sizeof(src_format));
	status |= vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_FORMAT, &dst_format, sizeof(dst_format));
	status |= vxQueryImage(src, VX_IMAGE_ATTRIBUTE_PLANES, &src_planes, sizeof(src_planes));
	status |= vxQueryImage(dst, VX_IMAGE_ATTRIBUTE_PLANES, &dst_planes, sizeof(dst_planes));
	status |= vxQueryImage(src, VX_IMAGE_ATTRIBUTE_SPACE, &src_space, sizeof(src_space));
	status = vxGetValidRegionImage(src, &rect);
	for (p = 0; p < src_planes; p++)
	{
		status |= vxAccessImagePatch(src, &rect, p, &src_addr[p], &src_base[p], VX_READ_ONLY);
		vxPrintImageAddressing(&src_addr[p]);
	}
	for (p = 0; p < dst_planes; p++)
	{
		status |= vxAccessImagePatch(dst, &rect, p, &dst_addr[p], &dst_base[p], VX_WRITE_ONLY);
		vxPrintImageAddressing(&dst_addr[p]);
	}
	if (status != VX_SUCCESS)
	{
		VX_PRINT(VX_ZONE_ERROR, "Failed to setup images in Color Convert!\n");
	}

	///////////////////////////////////////////

	// Only one of these implemented at the moment - to support our vxview example!

	if (src_format == VX_DF_IMAGE_RGB)// || (src_format == VX_DF_IMAGE_RGBX))
	{
		if (dst_format == VX_DF_IMAGE_IYUV)
		{
			if (src_planes==1 && dst_planes==3) // According to OpenVX spec RGB is single plane INTERLEAVED whereas IYUV is 3 plane
			{

				vImage_Buffer input = {
					.data = src_base[0],
					.height = src->height,
					.width = src->width, 
					.rowBytes = sizeof(vx_uint8)*src->width 
				};

				vImage_Buffer argb8888; // A temporary alpha 
				vImageBuffer_Init(&argb8888, dst->height, dst->width, 8*sizeof(vx_uint8)*4,kvImageNoFlags); // Allocated  here!

				vImage_Buffer destYp = {
					.data = dst_base[0],
					.height = dst->height,
					.width = dst->width, 
					.rowBytes = sizeof(vx_uint8)*dst->width 
				};

				vImage_Buffer destCb = {
					.data = dst_base[1],
					.height = dst->height,
					.width = dst->width, 
					.rowBytes = sizeof(vx_uint8)*dst->width 
				};

				vImage_Buffer destCr = {
					.data = dst_base[2],
					.height = dst->height,
					.width = dst->width, 
					.rowBytes = sizeof(vx_uint8)*dst->width 
				};

				/*
					 vImage_Buffer input; // The rgb interleaved image (no alpha)
					 vImage_Buffer argb8888; // A temporary alpha 
					 vImage_Buffer destYp;
					 vImage_Buffer destCb;
					 vImage_Buffer destCr;

				//vImageBuffer_Init(&input, src->height, src->width, 8*sizeof(vx_uint8)*3,kvImageNoAllocate);
				vImageBuffer_Init(&input, src->height, src->width, 8*sizeof(vx_uint8)*3,kvImageNoFlags);
				//posix_memalign(&input.data, alignment, input.height * input.rowBytes ); 	
				//input.data = src_base[0];  // all in that first channel
				memcpy(input.data,src_base[0],src->height*src->width*sizeof(vx_uint8));	

				vImageBuffer_Init(&argb8888, dst->height, dst->width, 8*sizeof(vx_uint8)*4,kvImageNoFlags); // Allocated 

				//vImageBuffer_Init(&destYp, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoAllocate);
				vImageBuffer_Init(&destYp, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoFlags);
				//vImageBuffer_Init(&destCb, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoAllocate);
				vImageBuffer_Init(&destCb, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoFlags);
				//vImageBuffer_Init(&destCr, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoAllocate);
				vImageBuffer_Init(&destCr, dst->height, dst->width, 8*sizeof(vx_uint8),kvImageNoFlags);
				//destYp.data = dst_base[0]; // Separate channels mapped
				//destCb.data = dst_base[1]; 
				//destCr.data = dst_base[2]; 
				*/

				vImage_Error result; 

				result = vImageConvert_RGB888toARGB8888(&input, NULL, 0, &argb8888, false, kvImageNoFlags);
				if(result != kvImageNoError)
				{
					VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to colour convert planar rgb to argb8888\n");
					return VX_FAILURE;
				}

				/// TODO: This set up of conversion structures/matrices should only be done once!
				vImage_Flags flags = kvImageNoFlags;
				vImage_YpCbCrPixelRange pixelRange;
				vImage_ARGBToYpCbCr info;

				vImage_ARGBToYpCbCrMatrix matrix;

				matrix.R_Yp          =  0.2989f;
				matrix.G_Yp          =  0.5866f;
				matrix.B_Yp          =  0.1144f;
				matrix.R_Cb          = -0.1688f;
				matrix.G_Cb          = -0.3312f;
				matrix.B_Cb_R_Cr     =  0.5f;
				matrix.G_Cr          = -0.4183f;
				matrix.B_Cr          = -0.0816f;

				pixelRange.Yp_bias         =   16;     // encoding for Y' = 0.0
				pixelRange.CbCr_bias       =  128;     // encoding for CbCr = 0.0
				pixelRange.YpRangeMax      =  235;     // encoding for Y'= 1.0
				pixelRange.CbCrRangeMax    =  240;     // encoding for CbCr = 0.5
				pixelRange.YpMax           =  255;     // a clamping limit above which the value is not allowed to go. 255 is fastest. Use pixelRange.YpRangeMax if you don't want Y' > 1.
				pixelRange.YpMin           =    0;     // a clamping limit below which the value is not allowed to go. 0 is fastest. Use pixelRange.Yp_bias if you don't want Y' < 0.
				pixelRange.CbCrMax         =  255;     // a clamping limit above which the value is not allowed to go. 255 is fastest.  Use pixelRange.CbCrRangeMax, if you don't want CbCr > 0.5
				pixelRange.CbCrMin         =    0;     // a clamping limit above which the value is not allowed to go. 0 is fastest.  Use (2*pixelRange.CbCr_bias - pixelRange.CbCrRangeMax), if you don't want CbCr < -0.5

				// THIS IS WHERE IT ALL FAILS FOR SOME REASON...
				
				result = vImageConvert_ARGBToYpCbCr_GenerateConversion(&matrix, &pixelRange, &info, kvImageARGB8888, kvImage420Yp8_Cb8_Cr8, flags);
				///
				result = vImageConvert_ARGB8888To420Yp8_Cb8_Cr8(&argb8888,&destYp,&destCb,&destCr,&info,NULL,kvImageNoFlags);
				if(result != kvImageNoError)
				{
				VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failled to colour convert argb8888 to YCbCr\n");
				return VX_FAILURE;
				}
					
				// Write it out to output vx_images	
				/*
					 memcpy(destYp.data,dst_base[0],dst->height*dst->width*sizeof(vx_uint8));	
					 memcpy(destCb.data,dst_base[1],dst->height*dst->width*sizeof(vx_uint8));	
					 memcpy(destCr.data,dst_base[2],dst->height*dst->width*sizeof(vx_uint8));	

					 free(input.data);
					 free(destYp.data);
					 free(destCb.data);
					 free(destCr.data);
					 */
				free(argb8888.data);
			}	
		}
	}

	///////////////////////////////////////////

	status = VX_SUCCESS;
	for (p = 0; p < src_planes; p++)
	{
		status |= vxCommitImagePatch(src, NULL, p, &src_addr[p], src_base[p]);
	}
	for (p = 0; p < dst_planes; p++)
	{
		status |= vxCommitImagePatch(dst, &rect, p, &dst_addr[p], dst_base[p]);
	}
	if (status != VX_SUCCESS)
	{
		VX_PRINT(VX_ZONE_ERROR, "Failed to set image patches on source or destination\n");
	}

	return status;
}

static vx_status VX_CALLBACK vxColorConvertKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 2)
    {
        vx_image src = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        
        //return VX_SUCCESS;
        status = vxColorConvert(src,dst);
    }

    return status;
}


static vx_status VX_CALLBACK vxColorConvertInputValidator(vx_node node, vx_uint32 index)
{
    vx_status status = VX_SUCCESS;
    if (index == 0)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0);
        if (param)
        {
            vx_image image = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &image, sizeof(image));
            if (image)
            {
                vx_df_image format = 0;
                vx_uint32 width = 0, height = 0;

                vxQueryImage(image, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                vxQueryImage(image, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(image, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                // check to make sure the input format is supported.
                switch (format)
                {
                    case VX_DF_IMAGE_RGB:  /* 8:8:8 interleaved */
                    case VX_DF_IMAGE_RGBX: /* 8:8:8:8 interleaved */
                    case VX_DF_IMAGE_NV12: /* 4:2:0 co-planar*/
                    case VX_DF_IMAGE_NV21: /* 4:2:0 co-planar*/
                    case VX_DF_IMAGE_IYUV: /* 4:2:0 planar */
                        if (height & 1)
                        {
                            status = VX_ERROR_INVALID_DIMENSION;
                            break;
                        }
                        /* no break */
                    case VX_DF_IMAGE_YUYV: /* 4:2:2 interleaved */
                    case VX_DF_IMAGE_UYVY: /* 4:2:2 interleaved */
                        if (width & 1)
                        {
                            status = VX_ERROR_INVALID_DIMENSION;
                        }
                        break;
                    default:
                        status = VX_ERROR_INVALID_FORMAT;
                        break;
                }
                vxReleaseImage(&image);
            }
            else
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
            vxReleaseParameter(&param);
        }
        else
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }
    return status;
}

static vx_df_image color_combos[][2] = {
        /* {src, dst} */
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_RGB, VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_RGBX,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_NV21},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_NV12,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_NV21,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_UYVY,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_YUV4},
        {VX_DF_IMAGE_YUYV,VX_DF_IMAGE_IYUV},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_RGB},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_RGBX},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_NV12},
        {VX_DF_IMAGE_IYUV,VX_DF_IMAGE_YUV4},
};

static vx_status VX_CALLBACK vxColorConvertOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param0 = vxGetParameterByIndex(node, 0);
        vx_parameter param1 = vxGetParameterByIndex(node, 1);
        if (param0 && param1)
        {
            vx_image output = 0, input = 0;
            vxQueryParameter(param0, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            vxQueryParameter(param1, VX_PARAMETER_ATTRIBUTE_REF, &output, sizeof(output));
            if (input && output)
            {
                vx_df_image src = VX_DF_IMAGE_VIRT;
                vx_df_image dst = VX_DF_IMAGE_VIRT;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_FORMAT, &src, sizeof(src));
                vxQueryImage(output, VX_IMAGE_ATTRIBUTE_FORMAT, &dst, sizeof(dst));
                if (dst != VX_DF_IMAGE_VIRT) /* can't be a unspecified format */
                {
                    vx_uint32 i = 0;
                    for (i = 0; i < dimof(color_combos); i++)
                    {
                        if ((color_combos[i][0] == src) &&
                            (color_combos[i][1] == dst))
                        {
                            ptr->type = VX_TYPE_IMAGE;
                            ptr->dim.image.format = dst;
                            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &ptr->dim.image.width, sizeof(ptr->dim.image.width));
                            vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &ptr->dim.image.height, sizeof(ptr->dim.image.height));
                            status = VX_SUCCESS;
                            break;
                        }
                    }
                }
                vxReleaseImage(&input);
                vxReleaseImage(&output);
            }
            vxReleaseParameter(&param0);
            vxReleaseParameter(&param1);
        }
    }
    VX_PRINT(VX_ZONE_API, "%s:%u returned %d\n", __FUNCTION__, index, status);
    return status;
}


/*! \brief Declares the parameter types for \ref vxuColorConvert.
 * \ingroup group_implementation
 */
static vx_param_description_t color_convert_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

/*! \brief The exported kernel table entry */
vx_kernel_description_t colorconvert_kernel = {
    VX_KERNEL_COLOR_CONVERT,
    "com.machineswithvision.openvx.color_convert",
    vxColorConvertKernel,
    color_convert_kernel_params, dimof(color_convert_kernel_params),
    vxColorConvertInputValidator,
    vxColorConvertOutputValidator,
    NULL,
    NULL,
};

