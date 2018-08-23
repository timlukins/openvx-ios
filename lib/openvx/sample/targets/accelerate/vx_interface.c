/*
 * Copyright (c) 2015 Machines With Vision
 */

#include <VX/vx.h>
#include <VX/vx_helper.h>
#include <vx_internal.h>

#include <vx_interface.h>

// Much requried piece of info to name the module!

static const vx_char name[VX_MAX_TARGET_NAME] = "machineswithvision.openvx";

// Invalid kernel def!

static vx_status VX_CALLBACK vxInvalidKernel(vx_node node, vx_reference parameters[], vx_uint32 num)
{
    return VX_ERROR_NOT_SUPPORTED;
}

static vx_status VX_CALLBACK vxAllFailInputValidator(vx_node node, vx_uint32 index)
{
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_status VX_CALLBACK vxAllFailOutputValidator(vx_node node, vx_uint32 index, vx_meta_format_t *info)
{
    return VX_ERROR_INVALID_PARAMETERS;
}

static vx_param_description_t invalid_kernel_params[1];
static vx_kernel_description_t invalid_kernel = {
    VX_KERNEL_INVALID,
    "com.machineswithvision.openvx.invalid",
    vxInvalidKernel,
    invalid_kernel_params, 0,
    vxAllFailInputValidator,
    vxAllFailOutputValidator,
    NULL,
    NULL,
};


// *******************************************************************
// TODO: CRUCIAL- list all kernels to be included in this module here!
// *******************************************************************

static vx_kernel_description_t *target_kernels[] = {
    &invalid_kernel,
    &absdiff_kernel,
    &accumulate_kernel,
    &accumulate_weighted_kernel,
    &accumulate_square_kernel,
    &add_kernel,
    &subtract_kernel,
    &and_kernel,
    &xor_kernel,
    &or_kernel,
    &not_kernel,
    &box3x3_kernel,
    &canny_kernel,
    &channelcombine_kernel,
    &channelextract_kernel,
    &colorconvert_kernel,
    &convertdepth_kernel,
    &convolution_kernel,
    &dilate3x3_kernel,
    &equalize_hist_kernel,
    &erode3x3_kernel,
    &fast9_kernel,
    &gaussian3x3_kernel,
    &harris_kernel,
    &histogram_kernel,
    &gaussian_pyramid_kernel,
    &integral_image_kernel,
    &magnitude_kernel,
    &mean_stddev_kernel,
    &median3x3_kernel,
    &minmaxloc_kernel,
    &optpyrlk_kernel,
    &phase_kernel,
    &multiply_kernel,
    &remap_kernel,
    &scale_image_kernel,
    &halfscale_gaussian_kernel,
    &sobel3x3_kernel,
    &lut_kernel,
    &threshold_kernel,
    &warp_affine_kernel,
    &warp_perspective_kernel,
    // Extra kernels copied into this module (originally in vx_extras_module.c)
    &edge_trace_kernel,
    &euclidian_nonmax_harris_kernel,
    &harris_score_kernel,
    &laplacian3x3_kernel,
    &lister_kernel,
    &nonmax_kernel,
    &norm_kernel,
    &scharr3x3_kernel,
    &sobelMxN_kernel,
    // Debug kernels copied into this module (originally in vx_debug_module.c)
    &fwriteimage_kernel,
    &freadimage_kernel,
    &fwritearray_kernel,
    &freadarray_kernel,
    &checkimage_kernel,
    &checkarray_kernel,
    &copyimage_kernel,
    &copyarray_kernel,
    &fillimage_kernel,
    &compareimage_kernel,
};

static vx_uint32 num_target_kernels = dimof(target_kernels);

/******************************************************************************/
/* EXPORTED FUNCTIONS */
/******************************************************************************/

vx_status vxTargetInit(vx_target target)
{
    if (target)
    {
        strncpy(target->name, name, VX_MAX_TARGET_NAME);
        target->priority = VX_TARGET_PRIORITY_C_MODEL;
    }
    return vxInitializeTarget(target, target_kernels, num_target_kernels);
}

vx_status vxTargetDeinit(vx_target target)
{
    return vxDeinitializeTarget(target);;
}

vx_status vxTargetSupports(vx_target target,
                           vx_char targetName[VX_MAX_TARGET_NAME],
                           vx_char kernelName[VX_MAX_KERNEL_NAME],
#if defined(EXPERIMENTAL_USE_VARIANTS)
                           vx_char variantName[VX_MAX_VARIANT_NAME],
#endif
                           vx_uint32 *pIndex)
{
    vx_status status = VX_ERROR_NOT_SUPPORTED;
    if (strncmp(targetName, name, VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "default", VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "power", VX_MAX_TARGET_NAME) == 0 ||
        strncmp(targetName, "performance", VX_MAX_TARGET_NAME) == 0)
    {
        vx_uint32 k = 0u;
        for (k = 0u; k < target->num_kernels; k++)
        {
            vx_char targetKernelName[VX_MAX_KERNEL_NAME];
            vx_char *kernel;
#if defined(EXPERIMENTAL_USE_VARIANTS)
            vx_char *variant;
            vx_char def[8] = "default";
#endif

            strncpy(targetKernelName, target->kernels[k].name, VX_MAX_KERNEL_NAME);
            kernel = strtok(targetKernelName, ":");
#if defined(EXPERIMENTAL_USE_VARIANTS)
            variant = strtok(NULL, ":");

            if (variant == NULL)
                variant = def;
#endif
            if (strncmp(kernelName, kernel, VX_MAX_KERNEL_NAME) == 0
#if defined(EXPERIMENTAL_USE_VARIANTS)
                && strncmp(variantName, variant, VX_MAX_VARIANT_NAME) == 0
#endif
                )
            {
                status = VX_SUCCESS;
                if (pIndex) *pIndex = k;
                break;
            }
        }
    }
    return status;
}

vx_action vxTargetProcess(vx_target target, vx_node_t *nodes[], vx_size startIndex, vx_size numNodes)
{
    vx_action action = VX_ACTION_CONTINUE;
    vx_status status = VX_SUCCESS;
    vx_size n = 0;
    for (n = startIndex; (n < (startIndex + numNodes)) && (action == VX_ACTION_CONTINUE); n++)
    {
        VX_PRINT(VX_ZONE_GRAPH,"Executing Kernel %s:%d in Nodes[%u] on target %s\n",
            nodes[n]->kernel->name,
            nodes[n]->kernel->enumeration,
            n,
            nodes[n]->base.context->targets[nodes[n]->affinity].name);

        vxStartCapture(&nodes[n]->perf);
        status = nodes[n]->kernel->function((vx_node)nodes[n],
                                            (vx_reference *)nodes[n]->parameters,
                                            nodes[n]->kernel->signature.num_parameters);
        nodes[n]->executed = vx_true_e;
        nodes[n]->status = status;
        vxStopCapture(&nodes[n]->perf);

        VX_PRINT(VX_ZONE_GRAPH,"kernel %s returned %d\n", nodes[n]->kernel->name, status);

        if (status == VX_SUCCESS)
        {
            /* call the callback if it is attached */
            if (nodes[n]->callback)
            {
                action = nodes[n]->callback((vx_node)nodes[n]);
                VX_PRINT(VX_ZONE_GRAPH,"callback returned action %d\n", action);
            }
        }
        else
        {
            action = VX_ACTION_ABANDON;
            VX_PRINT(VX_ZONE_ERROR, "Abandoning Graph due to error (%d)!\n", status);
        }
    }
    return action;
}

vx_status vxTargetVerify(vx_target target, vx_node_t *node)
{
    vx_status status = VX_SUCCESS;
    return status;
}

vx_kernel vxTargetAddKernel(vx_target target,
                            vx_char name[VX_MAX_KERNEL_NAME],
                            vx_enum enumeration,
                            vx_kernel_f func_ptr,
                            vx_uint32 numParams,
                            vx_kernel_input_validate_f input,
                            vx_kernel_output_validate_f output,
                            vx_kernel_initialize_f initialize,
                            vx_kernel_deinitialize_f deinitialize)
{
    vx_uint32 k = 0u;
    vx_kernel_t *kernel = NULL;
    for (k = target->num_kernels; k < VX_INT_MAX_KERNELS; k++)
    {
        kernel = &(target->kernels[k]);
        if ((kernel->enabled == vx_false_e) &&
            (kernel->enumeration == VX_KERNEL_INVALID))
        {
            vxInitializeKernel(target->base.context,
                               kernel,
                               enumeration, func_ptr, name,
                               NULL, numParams,
                               input, output, initialize, deinitialize);
            VX_PRINT(VX_ZONE_KERNEL, "Reserving %s Kernel[%u] for %s\n", target->name, k, kernel->name);
            target->num_kernels++;
            break;
        }
        kernel = NULL;
    }
    return (vx_kernel)kernel;
}
