
TODO:
====

- Integrate CircleCI
- Get test output from build hooked-up
- Get artifacts from build added as release.
- Work out release procedure (e.g. branch/tag)

- Confirm output/operation of the following methods with further tests (see Completed below).
- Confirm output of all test methods are valid. BIG TASK TO CHECKOUT OUTPUT IMAGE IN ALL CASES

- Deal with method parameters for border modes/different image types/shift etc. (see Partial below).

- Currently suspended tests due to memory issues (sometimes work, sometimes not):
	- vxAccumulate
	- vxColorConvert
	- vxConvertDepth 
	- vxGaussianPyramid (weird one)

- Speed up composite kernels by graph functions (see Proxy below)
	- vxHarris
	- vxOptPyLK


- Look at Not Accelerated again...	

- Update to 1.1. imminent!



Kernel status
=============

Conformant
----------
Run against conformance tests.

Tested
------
Specific input/output and complete range of acceptable values at least checked. Belive conforms and is correct.


Completed
---------
Range of parameters as per spec (and input/output validators) all correctly handled. Believe ready to test.

(8)
* absdiff_kernel
* accumulate_kernel
* box3x3_kernel
* equalize_hist_kernel
* gaussian3x3_kernel
* magnitude_kernel
* mean_stddev_kernel
* phase_kernel


Partial Acceleration
--------------------
Certain parameter values/types not implemented (or impossible to do so).

(21)
* add_kernel (S16 support, truncate policy used)
* accumulate_square_kernel (handle shift)
* accumulate_weighted_kernel (handle scaling)
* canny_kernel (norm and gradient size parameters - easy to do.) 
* channelcombine_kernel (lots of other formats, not all may be possible...)
* channelextract_kernel (subsampling was in original, check)
* colorconvert_kernel (not stable)
* convolution_kernel (check offset correct for bordermode) 
* dilate3x3_kernel (bordermode handled)
* erode3x3_kernel (bordermode handled)
* histogram_kernel (only able to do 256 bins IMPOSSIBLE)
* multiply_kernel (handle scaling and policies)
* scale_image_kernel (handle different types of interpolation IMPOSSIBLE)
* sobel3x3_kernel (bordermode respected - output type)
* subtract_kernel (S16 support, handle policy)
* warp_affine_kernel (interpolation flag types IMPOSSILBE)
* threshold_kernel (range threshold still to do)
* minmaxloc_kernel (doesn't yet return array/count indexes of all max/min values')
* lut_kernel (what if LUT is not 256 values?)
* convertdepth_kernel (downsampling cliping policy needs to be respected)

Proxy Acceleration
------------------
Implemented as graphs - so may pick up speed via sum of parts!.

(4)
* harris_kernel (vxHarrisScoreNode, vxEuclideanNonMaxHarrisNode, vxImageListerNode)
* optpyrlk_kernel (lots of operations, strange combo of graph and custom LTTracker method).

* gaussian_pyramid_kernel (working)
* halfscale_gaussian_kernel (working)


Possible Acceleration
---------------------
Not using accelerate/vDSP library, but performing differently using other/compiler optimisation tricks.

(4)
* and_kernel
* xor_kernel
* or_kernel
* not_kernel

Not Accelerated
---------------
Not yet started implementation, but tricky (or even impossible) to create accelerated version.
FOR THE MOMENT ALL IMPLMENTED AS PER ORIGINAL C MODEL.

(5)
* fast9_kernel (lots of operations - have to double check the passing of boolean/scalar param)
* integral_image_kernel (actually quite tricky - 2D addition of top left of pixel)
* median3x3_kernel (hard to derive without sort) 
* remap_kernel (no equivalent lookup operation that quickly uses a map between pixels)
* warp_perspective_kernel (no such corresponding accelerate method)




