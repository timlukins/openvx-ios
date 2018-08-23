/*
* Copyright (c) 2015 Machines With Vision
*/

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include<Accelerate/Accelerate.h>
#include <stdio.h>
#include <math.h>

static vx_status VX_CALLBACK vxHalfscaleGaussianKernel(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_FAILURE;
    if (num == 3)
    {
        vx_graph graph = vxGetChildGraphOfNode(node);

        status = vxProcessGraph(graph);
    }
    return status;
}

static const vx_uint32 gaussian5x5scale = 256;
static const vx_int16 gaussian5x5[5][5] =
{
    {1,  4,  6,  4, 1},
    {4, 16, 24, 16, 4},
    {6, 24, 36, 24, 6},
    {4, 16, 24, 16, 4},
    {1,  4,  6,  4, 1}
};

static vx_convolution vxCreateGaussian5x5Convolution(vx_context context)
{
    vx_convolution conv = vxCreateConvolution(context, 5, 5);
    vx_status status = vxReadConvolutionCoefficients(conv, NULL);
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }
    status = vxWriteConvolutionCoefficients(conv, (vx_int16 *)gaussian5x5);
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }
    
    vxSetConvolutionAttribute(conv, VX_CONVOLUTION_ATTRIBUTE_SCALE, (void *)&gaussian5x5scale, sizeof(vx_uint32));
    if (status != VX_SUCCESS)
    {
        vxReleaseConvolution(&conv);
        return NULL;
    }
    return conv;
}



static vx_status VX_CALLBACK vxHalfscaleGaussianInputValidator(vx_node node, vx_uint32 index)
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
                if (stype == VX_TYPE_INT32)
                {
                    vx_int32 ksize = 0;
                    vxReadScalarValue(scalar, &ksize);
                    if ((ksize == 3) || (ksize == 5))
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

static vx_status VX_CALLBACK vxHalfscaleGaussianOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *ptr)
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
                vx_uint32 w1 = 0, h1 = 0;
                vx_df_image f1 = VX_DF_IMAGE_VIRT;

                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_WIDTH, &w1, sizeof(w1));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_HEIGHT, &h1, sizeof(h1));
                vxQueryImage(src, VX_IMAGE_ATTRIBUTE_FORMAT, &f1, sizeof(f1));

                // fill in the meta data with the attributes so that the checker will pass
                ptr->type = VX_TYPE_IMAGE;
                ptr->dim.image.format = f1;
                ptr->dim.image.width = (w1 + 1) / 2;
                ptr->dim.image.height = (h1 + 1) / 2;
                status = VX_SUCCESS;
            }
            if (src) vxReleaseImage(&src);
            if (dst) vxReleaseImage(&dst);
            vxReleaseParameter(&src_param);
            vxReleaseParameter(&dst_param);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHalfscaleGaussianInitializer(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vx_image input = (vx_image)parameters[0];
        vx_image output = (vx_image)parameters[1];
        vx_int32 kernel_size = 3;
        vx_convolution convolution = 0;
        vx_context context = vxGetContext((vx_reference)node);
        vx_graph graph = vxCreateGraph(context);

        if (graph)
        {
            vx_uint32 i;
            vxReadScalarValue((vx_scalar)parameters[2], &kernel_size);
            if (kernel_size == 3 || kernel_size == 5)
            {
                if (kernel_size == 5)
                {
                    convolution = vxCreateGaussian5x5Convolution(context);
                }
                if (kernel_size == 3 || convolution)
                {
                    vx_image virt = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8);
                    vx_node nodes[] = {
                            kernel_size == 3 ? vxGaussian3x3Node(graph, input, virt) : vxConvolveNode(graph, input, convolution, virt),
                            vxScaleImageNode(graph, virt, output, VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR),
                    };

                    vx_border_mode_t borders;
                    vxQueryNode(node, VX_NODE_ATTRIBUTE_BORDER_MODE, &borders, sizeof(borders));
                    for (i = 0; i < dimof(nodes); i++) {
                        vxSetNodeAttribute(nodes[i], VX_NODE_ATTRIBUTE_BORDER_MODE, &borders, sizeof(borders));
                    }

                    status = VX_SUCCESS;
                    status |= vxAddParameterToGraphByIndex(graph, nodes[0], 0); // input image
                    status |= vxAddParameterToGraphByIndex(graph, nodes[1], 1); // output image
                    status |= vxAddParameterToGraphByIndex(graph, node, 2);     // gradient size - refer to self to quiet sub-graph validator
                    status |= vxVerifyGraph(graph);

                    // release our references, the graph will hold it's own
                    for (i = 0; i < dimof(nodes); i++) {
                        vxReleaseNode(&nodes[i]);
                    }
                    if (convolution) vxReleaseConvolution(&convolution);
                    vxReleaseImage(&virt);
                    status |= vxSetChildGraphOfNode(node, graph);
                }
            }
            vxReleaseGraph(&graph);
        }
    }
    return status;
}

static vx_status VX_CALLBACK vxHalfscaleGaussianDeinitializer(vx_node node, vx_reference *parameters, vx_uint32 num)
{
    vx_status status = VX_ERROR_INVALID_PARAMETERS;
    if (num == 3)
    {
        vxSetChildGraphOfNode(node, 0);
        status = VX_SUCCESS;
    }
    return status;
}

static vx_param_description_t halfscale_kernel_params[] = {
    {VX_INPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_OUTPUT, VX_TYPE_IMAGE, VX_PARAMETER_STATE_REQUIRED},
    {VX_INPUT, VX_TYPE_SCALAR, VX_PARAMETER_STATE_OPTIONAL},
};

vx_kernel_description_t halfscale_gaussian_kernel = {
    VX_KERNEL_HALFSCALE_GAUSSIAN,
    "com.machineswithvision.openvx.halfscale_gaussian",
    vxHalfscaleGaussianKernel,
    halfscale_kernel_params, dimof(halfscale_kernel_params),
    vxHalfscaleGaussianInputValidator,
    vxHalfscaleGaussianOutputValidator,
    vxHalfscaleGaussianInitializer,
    vxHalfscaleGaussianDeinitializer,
};

