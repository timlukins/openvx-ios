//
//  OpenVX.m
//  OpenVX
//
//  Created by Tim Lukins on 29/10/2015.
//
//
// THESE ARE THE OPENVX PERFORMANCE TESTS MOVED REMOVED FORM THE UNIT TESTS


#import <Foundation/Foundation.h>
#import <OpenVX/vx.h>
#import <OpenVX/vxu.h>
#import <OpenVX/vx_debug.h>

#define ORIGINAL_MANDRILL_512_512_1U8				"original_mandrill_512_512_1U8"
#define COLOUR_MANDRILL_512_512_RGB                 "colour_mandrill_512_512_RGB"

/***********************************************************************************/

/* Implementations of vxtest methods for iOS! */

vx_image vxTestGetImage(vx_context context, const char* filename)
{
    vx_image inimg;
    
    // This is definitely the test bundle, so use this!
    CFBundleRef inBundle  = CFBundleGetBundleWithIdentifier(CFSTR("com.machineswithvision.VxTest"));
    
    // Get path in file in bundle. BY DEFAULT RESOURCES ARE JUST COPIED TO THE TOP!
    CFStringRef filestring = CFStringCreateWithBytes(NULL, (const unsigned char*)filename, strlen(filename)*sizeof(char),kCFStringEncodingUTF8,false);
    
    //TODO - revamp this!
    if (CFStringCompare(filestring, CFSTR(ORIGINAL_MANDRILL_512_512_1U8), 0) == kCFCompareEqualTo)
    {
        CFURLRef fileUrl = CFBundleCopyResourceURL(inBundle, filestring, CFSTR("pgm"), NULL);
        CFStringRef filePath = CFURLCopyFileSystemPath(fileUrl, kCFURLPOSIXPathStyle);
        
        // There's probably a better way to load images natively with iOS but, lets try and keep it simple...
        CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
        const char *path = CFStringGetCStringPtr(filePath, encodingMethod);
        
        // Right, now let's load it...
        int width;
        int height;
        FILE *in = fopen(path, "r");
        fscanf(in, "%*[^\n]\n%d %d\n%*[^\n]\n",&width, &height);
        vx_uint8* bytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);
        for(int y=0; y<height; y++)
        for(int x=0; x<width; x++)
        fscanf(in, "%c",&bytes[(y*width)+x]);
        
        // Describe the data
        vx_imagepatch_addressing_t addrs[] = {
            {
                width,
                height,
                sizeof(vx_uint8),
                width * sizeof(vx_uint8),
                VX_SCALE_UNITY,
                VX_SCALE_UNITY,
                1,
                1
            }
        };
        
        void* indata[] = { bytes };
        
        inimg = vxCreateImageFromHandle(
                                        context,
                                        VX_DF_IMAGE_U8,
                                        addrs,
                                        indata,
                                        VX_IMPORT_TYPE_HOST);
        
        if (vxGetStatus((vx_reference)inimg) != VX_SUCCESS)
        {
            printf("Failed to create vx_image.\n");
        }
        
    }
    else if (CFStringCompare(filestring, CFSTR(COLOUR_MANDRILL_512_512_RGB), 0) == kCFCompareEqualTo)
    {
        CFURLRef fileUrl = CFBundleCopyResourceURL(inBundle, filestring, CFSTR("ppm"), NULL);
        CFStringRef filePath = CFURLCopyFileSystemPath(fileUrl, kCFURLPOSIXPathStyle);
        
        // There's probably a better way to load images natively with iOS but, lets try and keep it simple...
        CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
        const char *path = CFStringGetCStringPtr(filePath, encodingMethod);
        
        // Right, now let's load it...
        int width;
        int height;
        int depth = 3;
        FILE *in = fopen(path, "r");
        fscanf(in, "%*[^\n]\n%d %d\n%*[^\n]\n",&width, &height);
        vx_uint8* bytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height*depth); // Interleaved bytes
        //vx_uint8* rbytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);
        //vx_uint8* gbytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);
        //vx_uint8* bbytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);
        for(int y=0; y<height; y++)
        for(int x=0; x<width; x++)
        for(int d=0; d<depth; d++)
        fscanf(in, "%c",&bytes[(y*width)+x+d]);
        //fscanf(in, "%c%c%c",
        //   &rbytes[(y*width)+x],
        //   &gbytes[(y*width)+x],
        //   &bbytes[(y*width)+x]);
        
        // Describe the data
        vx_imagepatch_addressing_t addrs[] = {
            {
                width,
                height,
                sizeof(vx_uint8),
                width * sizeof(vx_uint8),
                VX_SCALE_UNITY,
                VX_SCALE_UNITY,
                1,
                1
            }
        };
        
        void* indata[] = {bytes};//{ rbytes,gbytes,bbytes };
        
        inimg = vxCreateImageFromHandle(
                                        context,
                                        VX_DF_IMAGE_RGB,
                                        addrs,
                                        indata,
                                        VX_IMPORT_TYPE_HOST);
        
        if (vxGetStatus((vx_reference)inimg) != VX_SUCCESS)
        {
            printf("Failed to create vx_image.\n");
        }
        
    }
    // TODO: else if
    //vx_image inimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8); // Just create blank image
    else
    {
        printf("Unknown type of test data...\n");
    }
    
    return inimg;
    
}


// Performance logging methods..

NSDate *methodStart; // Global used...

void vxTestStart()
{
    methodStart = [NSDate date];
}

NSTimeInterval vxTestEnd()
{
    NSDate *methodFinish = [NSDate date];
    NSTimeInterval executionTime = [methodFinish timeIntervalSinceDate:methodStart];
    return executionTime;
}

//////////////////////////////////////////////////////////////////////////////


NSTimeInterval test_vxAbsDiff(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vxAbsDiffNode(graph, input1, input2, outimg);
    
    vx_status status = vxVerifyGraph(graph);
    
    vxTestStart();
    for (int x=0;x<runfor;x++)
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();
    
    vxReleaseImage(&outimg);
    vxReleaseGraph(&graph);
    
    return runtime;
}


NSTimeInterval test_vxCanny(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image edges = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vx_threshold hyst = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
    vx_int32 lower = 50, upper = 100;
    vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_LOWER, &lower, sizeof(lower));
    vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_UPPER, &upper, sizeof(upper));
    
    vxCannyEdgeDetectorNode(graph, input, hyst, 3, VX_NORM_L1, edges);
    
    vx_status status = vxVerifyGraph(graph);

    
    vxTestStart();
    for (int x=0;x<runfor;x++) // 100 = RUNXTIMES macro instead?
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();
    
    vxReleaseImage(&edges);
    vxReleaseGraph(&graph);
    vxReleaseContext(&context);

    return runtime;
}



NSTimeInterval test_vxDilate3x3(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vxDilate3x3Node(graph, input, outimg);
    
    vx_status status = vxVerifyGraph(graph);
    
    vxTestStart();
    for (int x=0;x<runfor;x++) // 100 = RUNXTIMES macro instead?
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();
    
    vxReleaseImage(&outimg);
    vxReleaseGraph(&graph);
    vxReleaseContext(&context);
    
    return runtime;
}



NSTimeInterval test_vxEqualizeHist(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image outimg = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vxEqualizeHistNode(graph, input, outimg);
    
    vx_status status = vxVerifyGraph(graph);
    
    vxTestStart();
    for (int x=0;x<runfor;x++) // 100 = RUNXTIMES macro instead?
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();
    
    vxReleaseImage(&outimg);
    vxReleaseGraph(&graph);
    vxReleaseContext(&context);
    
    return runtime;
}



NSTimeInterval test_vxMeanStdDev(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_float32 mean;
    vx_float32 stddev; // A bit weird not scalars...
    
    vx_scalar mean_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &mean);
    vx_scalar stddev_scalar = vxCreateScalar(context, VX_TYPE_FLOAT32, &stddev);
    
    vxMeanStdDevNode(graph, input, mean_scalar, stddev_scalar);
    
    vx_status status = vxVerifyGraph(graph);
    
    vxTestStart();
    for (int x=0;x<runfor;x++) // 100 = RUNXTIMES macro instead?
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();
    
    vxReleaseGraph(&graph);
    vxReleaseContext(&context);
    
    return runtime;
}

NSTimeInterval test_vxOr(int runfor)
{
    vx_context context = vxCreateContext();
    
    vx_graph graph = vxCreateGraph(context);
    
    vx_image input1 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    vx_image input2 = vxTestGetImage(context,ORIGINAL_MANDRILL_512_512_1U8);
    
    vx_image result = vxCreateImage(context, 512, 512, VX_DF_IMAGE_U8);
    
    vxOrNode(graph, input1, input2, result);
    
    vx_status status = vxVerifyGraph(graph);
    
    vxTestStart();
    for (int x=0;x<runfor;x++) // 100 = RUNXTIMES macro instead?
    status = vxProcessGraph(graph);
    NSTimeInterval runtime = vxTestEnd();

    vxReleaseImage(&result);
    vxReleaseGraph(&graph);
    
    return runtime;
}