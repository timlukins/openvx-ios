/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>

vx_status vxBox3x3(vx_image src, vx_image dst, vx_border_mode_t *bordermode)
{
		// Input should only have already been confirmed as a single channel of U8 

		vx_imagepatch_addressing_t src_addr, dst_addr;
		void *src_base = NULL;
		void *dst_base = NULL;

        vx_rectangle_t rect = {0, 0, src->width, src->height};
				
		vx_status status = VX_SUCCESS;
		status = vxGetValidRegionImage(src, &rect);
		status |= vxAccessImagePatch(src, &rect, 0, &src_addr, &src_base, VX_READ_ONLY); // Note single plane = 0 index
        //vxPrintImageAddressing(&src_addr);
		
        status = vxGetValidRegionImage(dst, &rect);
		status |= vxAccessImagePatch(dst, &rect, 0, &dst_addr, &dst_base, VX_WRITE_ONLY);
		//vxPrintImageAddressing(&dst_addr);
		
		if (status != VX_SUCCESS)
		{
				VX_PRINT(VX_ZONE_ERROR, "Failed to setup images in Box 3x3!\n");
		}

		vImage_Buffer input = {
			.data = src_base,
			.height = src->height,
			.width = src->width, 
			.rowBytes = sizeof(vx_uint8)*src->width 
		};
		
		vImage_Buffer output = {
			.data = dst_base,
			.height = dst->height,
			.width = dst->width, 
			.rowBytes = sizeof(vx_uint8)*dst->width 
		};

		vImage_Error result;
    
        result = vImageBoxConvolve_Planar8 (&input, &output, NULL, 0, 0, 3, 3, 0, kvImageBackgroundColorFill);
    
		if(result != kvImageNoError)
		{
			VX_PRINT(VX_ZONE_ERROR, "Accelerate: Failed to do image convolve on planar 8\n");
			return VX_FAILURE;
		}
	
		status |= vxCommitImagePatch(src, &rect, 0, &src_addr, src_base);
		status |= vxCommitImagePatch(dst, &rect, 0, &dst_addr, dst_base);

	return status;
}

static vx_status VX_CALLBACK vxBox3x3Kernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 2)
    {
        vx_border_mode_t bordermode;
        vx_image src  = (vx_image)parameters[0];
        vx_image dst = (vx_image)parameters[1];
        status = vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &bordermode, sizeof(bordermode));
        if (status == VX_SUCCESS)
        {
            status = vxBox3x3(src, dst, &bordermode);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxBoxFilterInputValidator(vx_node node, vx_uint32 index)
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
    return status;
}

static vx_status VX_CALLBACK vxBoxFilterOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 1)
    {
        vx_parameter param = vxGetParameterByIndex(node, 0); // we reference the input image 
        if (param)
        {
            vx_image input = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &input, sizeof(input));
            if (input)
            {
                vx_uint32 width = 0, height = 0;
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(width));
                vxQueryImage(input, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = VX_DF_IMAGE_U8;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&input);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_param_description_t box_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t box3x3_kernel = {
    VX_KERNEL_BOX_3x3,
    "com.machineswithvision.openvx.box3x3",
    vxBox3x3Kernel,
    box_kernel_params, dimof(box_kernel_params),
    vxBoxFilterInputValidator,
    vxBoxFilterOutputValidator,
};

