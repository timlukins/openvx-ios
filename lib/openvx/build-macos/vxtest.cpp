#include "vxtest.h"
#include "gtest/gtest.h"

/***********************************************************/

/* Implementations of vxtest methods for *nix (MacOS & Linux) here! */

vx_image vxTestGetImage(vx_context context, const char* filename)
{
    char path[256]; // Where to load it from
    vx_image inimg; // Resulting input image
    
    if (strcmp(filename,ORIGINAL_MANDRILL_512_512_1U8)==0)
    {
        int len= snprintf(path, sizeof(path), "../../test/data/%s.pgm",filename);
        //printf("Loading from: %s\n",path);
        
        /* Image data. */
        int width;
        int height;
        FILE *in = fopen(path, "r");
        fscanf(in, "%*[^\n]\n%d %d\n%*[^\n]\n",&width, &height);
        vx_uint8* bytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);
        for(int y=0; y<height; y++)
            for(int x=0; x<width; x++)	
                fscanf(in, "%c",&bytes[(y*width)+x]);

        /* Sort out memory addressing for image data. */
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
        
        /* And wrap the data. */	
        void* indata[] = { bytes };

        /* Image to process. */
        inimg = vxCreateImageFromHandle(
                context,
                VX_DF_IMAGE_U8,// VX_DF_IMAGE_RGB,
                addrs,
                indata,
                VX_IMPORT_TYPE_HOST);

        if (vxGetStatus((vx_reference)inimg) != VX_SUCCESS)
        {
            printf("Failed to create vx_image.\n");
        }
    }
    else if (strcmp(filename,COLOUR_MANDRILL_512_512_RGB)==0)
    {
        int len= snprintf(path, sizeof(path), "../../test/data/%s.ppm",filename);
        //printf("Loading from: %s\n",path);
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
                static_cast<vx_uint32>(width),
                static_cast<vx_uint32>(height),
                sizeof(vx_uint8),
                static_cast<vx_int32>(width * sizeof(vx_uint8)),
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

	return inimg;
}

void vxTestSaveImage(vx_image outimg, const char* filename)
{
	vx_uint8* outbytes;
	vx_imagepatch_addressing_t out_addr;
	vx_rectangle_t rect;// = {0, 0,width, height};
	vx_status status = vxGetValidRegionImage(outimg, &rect);
	if (status!=VX_SUCCESS) printf("Errk!!\n");
	
	void* base_ptr = NULL;	
	status |= vxGetValidRegionImage(outimg, &rect);	
	//printf("Processed image dimensions: %d %d -> %d %d\n",rect.start_x, rect.start_y,rect.end_x,rect.end_y);	
	
	status |= vxAccessImagePatch(outimg, &rect, 0, &out_addr, &base_ptr, VX_READ_ONLY); // Note single plane = 0 index
	outbytes = (vx_uint8*)vxFormatImagePatchAddress1d(base_ptr,0,&out_addr); // access address of first pixel
	//status |= vxCommitImagePatch(outimg, &rect, 0, &out_addr, base_ptr);

	int width = rect.end_x - rect.start_x;
	FILE *out = fopen(filename, "w");
	fprintf(out, "P5\n%d %d\n255\n", rect.end_x-rect.start_x, rect.end_y-rect.start_y);
	for(int y=rect.start_y; y<rect.end_y; y++)
	{
		for(int x=rect.start_x; x<rect.end_x; x++)
		{
			fprintf(out, "%c", outbytes[(y*width)+x]);
		}
	}
}

/*
// Performance test methods...

// TODO!
void vxTestLogStart() {}
void vxTestLogEnd(char* info) {} 
*/
/***********************************************************/
/*
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/
