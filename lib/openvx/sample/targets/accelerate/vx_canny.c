/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include <math.h>
#include <Accelerate/Accelerate.h>

// TODO: different kernel size and thresholding scheme

// So, the original (c_model) version did this as a graph.
// It utilised a number of additional kernels from the extras lib.
// Here, we use the (dragonfly) original form of canny.

vx_status vxCanny(vx_image input, vx_image output,vx_threshold hyst,vx_scalar gradient_size, vx_scalar norm_type)
{
	//__builtin_trap();
    // Gubbings to convert vx data structures etc.
    void* I_ptr = NULL; // Pointers used in algorithm to input image and edgemap memory
    void* edgeMap_ptr = NULL;	
    // Assumes VX_DF_IMAGE_U8 (i.e. one channel 8bit) checked by input validator
    vx_status status;
    vx_rectangle_t rect;// = {0, 0, input->width, input->height};
    vx_imagepatch_addressing_t I_addr, edge_addr;
  	status = vxGetValidRegionImage(input, &rect);
    status = vxAccessImagePatch(input, &rect, 0, &I_addr, &I_ptr, VX_READ_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
    status = vxGetValidRegionImage(output, &rect);
    status = vxAccessImagePatch(output, &rect, 0, &edge_addr, &edgeMap_ptr, VX_WRITE_ONLY);
    if (status!=VX_SUCCESS)
        return VX_FAILURE;
  
    // Convert internally/explicitly

    vx_uint8* I = vxFormatImagePatchAddress1d(I_ptr,0,&I_addr);
    vx_uint8* edgeMap = vxFormatImagePatchAddress1d(edgeMap_ptr,0,&edge_addr);

    int width  = rect.end_x - rect.start_x;
    int height = rect.end_y - rect.start_y;
    
    vx_int32 th;
    vx_int32 tl;
    vxQueryThreshold(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_LOWER, &tl, sizeof(tl));
    vxQueryThreshold(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_UPPER, &th, sizeof(th));
    
    vx_uint32 grad;// Number representing size of sobel to use - 3, 5, or 7.
    vxReadScalarValue(gradient_size, &grad); // NOTE: you must be reading to the expected size of variable (otherwise memory issues!)
    vx_enum norm;// Norm calculation to use VX_NORM_L1 or VX_NORM_L2
    vxReadScalarValue(norm_type, &norm);
    
    // Original code here.... with TODO's
   
    // Pad out the input image with replicated border pixels
    int width_p = width+4;
    int height_p = height+4;
    
    float *im = (float*)malloc(width_p*height_p*sizeof(float));
    float *im_smooth = (float*)malloc(width_p*height_p*sizeof(float));
    float *im_gx = (float*)malloc(width_p*height_p*sizeof(float));
    float *im_gy = (float*)malloc(width_p*height_p*sizeof(float));
    
    float *im_mag = (float*)malloc(width*height*sizeof(float));
    int *im_dir = (int*)malloc(width*height*sizeof(int));
    float *im_nms = (float*)malloc(width*height*sizeof(float));
    int *ex=(int*)malloc(width*height*sizeof(int));
    int *ey=(int*)malloc(width*height*sizeof(int));
   
    // Convert the 8bit (and pad)
    for(int y_p=0;y_p<height_p;y_p++) {
        for(int x_p=0;x_p<width_p;x_p++) {
            int x = x_p-2;
            int y = y_p-2;
            if (x<0) x=0;
            if (y<0) y=0;
            if (x>width-1) x=width-1;
            if (y>height-1) y=height-1;
            int i_out=y_p*width_p+x_p;
            int i_in=y*width+x;
            //float val = (float)I[i_in];
            im[i_out]=(float)(I[i_in]&0xff);
        }
    }
    
    // Apply 5x5 gaussian smoothing
    float kernel_5x5[]={
        0.00291502,  0.01306423,  0.02153928,  0.01306423,  0.00291502,
        0.01306423,  0.05854983,  0.09653235,  0.05854983,  0.01306423,
        0.02153928,  0.09653235,  0.15915494,  0.09653235,  0.02153928,
        0.01306423,  0.05854983,  0.09653235,  0.05854983,  0.01306423,
        0.00291502,  0.01306423,  0.02153928,  0.01306423,  0.00291502
    };
    vDSP_f5x5(im, height_p, width_p, kernel_5x5, im_smooth);
    
    // TODO:  this should accept 3,5 & 7 kernels (see p.46 of openvx spec).
    
    // Now apply sobel edge detection
    float kernel_gx[]={
        1.0f, 0.0f, -1.0f,
        2.0f, 0.0f, -2.0f,
        1.0f, 0.0f, -1.0f
    };
    float kernel_gy[]={
        1.0f,  2.0f,  1.0f,
        0.0f,  0.0f,  0.0f,
        -1.0f, -2.0f, -1.0f
    };
    vDSP_f3x3(im_smooth, height_p, width_p, kernel_gx, im_gx);
    vDSP_f3x3(im_smooth, height_p, width_p, kernel_gy, im_gy);
    
    for(int y=1;y<height-1;y++) {
        for(int x=1;x<width-1;x++) {
            int direction=0;
            int i=y*width+x;
            int i_p=(y+2)*width_p+x+2;
            float gx=im_gx[i_p];
            float gy=im_gy[i_p];
            float mag=gx*gx+gy*gy; // TODO: THIS IS VX_NORM_L2 ??? SWITCH TYPE OF NORM
            if (mag>=tl*tl) {
                // Rotate by PI/8
                float xr=gx*0.92387953251128674+gy*0.38268343236508978;
                float yr=gy*0.92387953251128674-gx*0.38268343236508978;
                if (xr>=0) {
                    if (yr>=0) {
                        if (xr>yr)
                            direction=4;
                        else
                            direction=3;
                    } else {
                        if (xr>-yr)
                            direction=1;
                        else
                            direction=2;
                    }
                } else {
                    if (yr>=0) {
                        if (-xr>yr)
                            direction=1;
                        else
                            direction=2;
                    } else {
                        if (-xr>-yr)
                            direction=4;
                        else
                            direction=3;
                    }
                }
            } else {
                mag=0;
            }
            im_mag[i]=mag;
            im_dir[i]=direction;
        }
    }
    
    // Now apply non-maximum suppression
    for(int y=1;y<height-1;y++) {
        for(int x=1;x<width-1;x++) {
            int i=y*width+x;
            float mag=im_mag[i];
            float nms=0;
            if (mag>0) {
                int dir=im_dir[i];
                if (dir==1) {
                    if ((mag<im_mag[i+1]) || (mag<=im_mag[i-1]))
                        nms=0;
                    else
                        nms=mag;
                } else if (dir==2) {
                    if ((mag<im_mag[i-width+1]) || (mag<=im_mag[i+width-1]))
                        nms=0;
                    else
                        nms=mag;
                } else if (dir==3) {
                    if ((mag<im_mag[i+width]) || (mag<=im_mag[i-width]))
                        nms=0;
                    else
                        nms=mag;
                } else if (dir==4) {
                    if ((mag<im_mag[i+width+1]) || (mag<=im_mag[i-width-1]))
                        nms=0;
                    else
                        nms=mag;
                } else {
                    nms=0;
                }
            }
            im_nms[i]=nms;
        }
    }
    
    
    // Hysteresis thresholding
    int ox[]={1,1,0,-1,-1,-1, 0, 1};
    int oy[]={0,1,1, 1, 0,-1,-1,-1};
    int ec=0;
    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            int index=y*width+x;
            if (im_nms[index]>=th*th) {
                edgeMap[index]=255;
                ex[ec]=x;
                ey[ec]=y;
                ec++;
            } else {
                edgeMap[index]=0;
            }
        }
    }
    
    int i=0;
    while(i<ec) {
        int x=ex[i];
        int y=ey[i];
        for(int j=0;j<8;j++) {
            int index=(y+oy[j])*width+x+ox[j];
            if (index<width*height) { // Little bit of defensive programming
                if (edgeMap[index]==0) {
                    if (im_nms[index]>=tl*tl) {
                        edgeMap[index]=1;
                        ex[ec]=x+ox[j];
                        ey[ec]=y+oy[j];
                        ec++;
                    }
                }
            }
        }
        i++;
    }
 
    free(im);
    free(im_smooth);
    free(im_gx);
    free(im_gy);
    free(im_mag);
    free(im_dir);
    free(im_nms);
    free(ex);
    free(ey);
    //return ec;
		
		status |= vxCommitImagePatch(input, &rect, 0, &I_addr, I_ptr);
		status |= vxCommitImagePatch(output, &rect, 0, &edge_addr, edgeMap_ptr);
    return status; 
}


/*******************************************************************/

/* Again, original here would just start the graph. Instead we invoke the above function. */

static vx_status VX_CALLBACK vxCannyEdgeKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 5)
    {
        vx_image input = (vx_image)parameters[0];
        vx_threshold hyst = (vx_threshold)parameters[1];
        vx_scalar gradient_size = (vx_scalar)parameters[2];
        vx_scalar norm_type = (vx_scalar)parameters[3];
        vx_image output = (vx_image)parameters[4];
        
        status = vxCanny(input,output,hyst,gradient_size,norm_type);
    }

    return status;

}

static vx_status VX_CALLBACK vxCannyEdgeInputValidator(vx_node node, vx_uint32 index)
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
    else if (index == 1)
    {
        vx_threshold hyst = 0;
        vx_parameter param = vxGetParameterByIndex(node, index);

        vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &hyst, sizeof(hyst));
        if (hyst)
        {
            vx_enum type = 0;
            vxQueryThreshold(hyst, VX_THRESHOLD_ATTRIBUTE_TYPE, &type, sizeof(type));
            if (type == VX_THRESHOLD_TYPE_RANGE)
            {
                status = VX_SUCCESS;
            }
            vxReleaseThreshold(&hyst);
        }
        vxReleaseParameter(&param);
    }
    else if (index == 2)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar value = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &value, sizeof(value));
            if (value)
            {
                vx_enum stype = 0;
                vxQueryScalar(value, VX_SCALAR_ATTRIBUTE_TYPE, &stype, sizeof(stype));
                if (stype == VX_TYPE_INT32)
                {
                    vx_int32 gs = 0;
                    vxReadScalarValue(value, &gs);
                    if ((gs == 3) || (gs == 5) || (gs == 7))
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
                vxReleaseScalar(&value);
            }
            vxReleaseParameter(&param);
        }
    }
    else if (index == 3)
    {
        vx_parameter param = vxGetParameterByIndex(node, index);
        if (param)
        {
            vx_scalar value = 0;
            vxQueryParameter(param, VX_PARAMETER_ATTRIBUTE_REF, &value, sizeof(value));
            if (value)
            {
                vx_enum norm = 0;
                vxReadScalarValue(value, &norm);
                if ((norm == VX_NORM_L1) ||
                    (norm == VX_NORM_L2))
                {
                    status = VX_SUCCESS;
                }
                vxReleaseScalar(&value);
            }
            vxReleaseParameter(&param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxCannyEdgeOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (index == 4)
    {
        vx_parameter src_param = vxGetParameterByIndex(node, 0);
        if (src_param)
        {
            vx_image src = 0;
            vxQueryParameter(src_param, VX_PARAMETER_ATTRIBUTE_REF, &src, sizeof(src));
            if (src)
            {
                vx_uint32 width = 0, height = 0;
                vx_df_image format = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &width, sizeof(height));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &height, sizeof(height));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &format, sizeof(format));
                /* fill in the meta data with the attributes so that the checker will pass */
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = format;
                ptr->dim.image.width = width;
                ptr->dim.image.height = height;
                status = VX_SUCCESS;
                vxReleaseImage(&src);
            }
            vxReleaseParameter(&src_param);
        }
    }
    return status;
}

static vx_param_description_t canny_kernel_params[] = {
    {VX_INPUT,  VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT,  VX_TYPE_THRESHOLD, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT,  VX_TYPE_SCALAR, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
};

vx_kernel_description_t canny_kernel = {
    VX_KERNEL_CANNY_EDGE_DETECTOR,
    "com.machineswithvision.openvx.canny_edge_detector",
    vxCannyEdgeKernel,
    canny_kernel_params, dimof(canny_kernel_params),
    vxCannyEdgeInputValidator,
    vxCannyEdgeOutputValidator,
    //vxCannyEdgeInitializer, // Original form setup a graph...
    //vxCannyEdgeDeinitializer, // ... and took it down again.
};
