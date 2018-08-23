
#ifndef _OPENVX_INTERFACE_H_
#define _OPENVX_INTERFACE_H_

#include <VX/vx_helper.h>

// *******************************************************************
// TODO: CRUCIAL- list all kernels to be included in this module here!
// These are then listed under target_kernels in the interface.c file.
// *******************************************************************

// 41 version 1.0 OpenVX core kernels - checked/listed in order of spec.

extern vx_kernel_description_t absdiff_kernel;
extern vx_kernel_description_t accumulate_kernel;
extern vx_kernel_description_t accumulate_weighted_kernel;
extern vx_kernel_description_t accumulate_square_kernel;
extern vx_kernel_description_t add_kernel;
extern vx_kernel_description_t subtract_kernel;
extern vx_kernel_description_t and_kernel;
extern vx_kernel_description_t xor_kernel;
extern vx_kernel_description_t or_kernel;
extern vx_kernel_description_t not_kernel;
extern vx_kernel_description_t box3x3_kernel;
extern vx_kernel_description_t canny_kernel;
extern vx_kernel_description_t channelcombine_kernel;
extern vx_kernel_description_t channelextract_kernel;
extern vx_kernel_description_t colorconvert_kernel;
extern vx_kernel_description_t convertdepth_kernel;
extern vx_kernel_description_t convolution_kernel;
extern vx_kernel_description_t dilate3x3_kernel;
extern vx_kernel_description_t equalize_hist_kernel;
extern vx_kernel_description_t erode3x3_kernel;
extern vx_kernel_description_t fast9_kernel;
extern vx_kernel_description_t gaussian3x3_kernel;
extern vx_kernel_description_t harris_kernel;
extern vx_kernel_description_t histogram_kernel;
extern vx_kernel_description_t gaussian_pyramid_kernel;
extern vx_kernel_description_t integral_image_kernel;
extern vx_kernel_description_t magnitude_kernel;
extern vx_kernel_description_t mean_stddev_kernel;
extern vx_kernel_description_t median3x3_kernel;
extern vx_kernel_description_t minmaxloc_kernel;
extern vx_kernel_description_t optpyrlk_kernel;
extern vx_kernel_description_t phase_kernel;
extern vx_kernel_description_t multiply_kernel;
extern vx_kernel_description_t remap_kernel;
extern vx_kernel_description_t scale_image_kernel;
extern vx_kernel_description_t halfscale_gaussian_kernel;
extern vx_kernel_description_t sobel3x3_kernel;
extern vx_kernel_description_t lut_kernel;
extern vx_kernel_description_t threshold_kernel;
extern vx_kernel_description_t warp_affine_kernel;
extern vx_kernel_description_t warp_perspective_kernel;

// Extra modules copied here (originally in separate vx_extra_modules.h)

extern vx_kernel_description_t edge_trace_kernel;
extern vx_kernel_description_t euclidian_nonmax_harris_kernel;
extern vx_kernel_description_t harris_score_kernel;
extern vx_kernel_description_t laplacian3x3_kernel;
extern vx_kernel_description_t lister_kernel;
extern vx_kernel_description_t nonmax_kernel;
extern vx_kernel_description_t norm_kernel;
extern vx_kernel_description_t scharr3x3_kernel;
extern vx_kernel_description_t sobelMxN_kernel;

// Debug modules copied here (originally in separate vx_debug_modules.h)

extern vx_kernel_description_t fwriteimage_kernel;
extern vx_kernel_description_t freadimage_kernel;
extern vx_kernel_description_t fwritearray_kernel;
extern vx_kernel_description_t freadarray_kernel;
extern vx_kernel_description_t checkimage_kernel;
extern vx_kernel_description_t checkarray_kernel;
extern vx_kernel_description_t copyimage_kernel;
extern vx_kernel_description_t copyarray_kernel;
extern vx_kernel_description_t fillimage_kernel;
extern vx_kernel_description_t compareimage_kernel;

#endif

