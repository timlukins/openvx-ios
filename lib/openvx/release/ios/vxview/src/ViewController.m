//
//  ViewController.m
//
//  Created by Tim Lukins on 01/05/2015.
//
//  Copyright (c) 2015 Machines with Vision. All rights reserved.
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
//

#import "ViewController.h"
#import <CoreVideo/CVOpenGLESTextureCache.h>
#import <Accelerate/Accelerate.h>
#import <OpenGLES/ES2/glext.h>
#import <TargetConditionals.h>
#import <OpenVX/vx_debug.h> // Included if you want to see VX_PRINT output

////////////////////////////////////////////////////////////////

// OpenGL shader uniform indexes.
enum
{
    UNIFORM_RGBA,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

// OpenGL shader attribute indexes.
enum
{
    ATTRIB_VERTEX,
    ATTRIB_TEXCOORD,
    ATTRIB_EDGE,
    NUM_ATTRIBUTES
};

// Edge struct that we use of rendering.
typedef struct _edge
{
    GLfloat x;
    GLfloat y;
} Edge;

// Allows us to hard-code limit on number of edges stored
//#define MAX_EDGES 100000
#define MAX_EDGES 307200 // =640x480

////////////////////////////////////////////////////////////////

// Private interface

@interface ViewController ()
{
    // AV..

    AVCaptureSession* _session;
    CGFloat _screenWidth;
    CGFloat _screenHeight;
    size_t _textureWidth;
    size_t _textureHeight;
    
    // Core image buffers for input and output...
    uint8_t* _baseAddress;
    uint8_t* _edgeAddress;

    // OpenVX...
    
    vx_context _vxContext;
    vx_graph _graph;
    
    // OpenGL edge buffer...
    Edge* _edges;
    GLuint _nEdges;

    // OpenGL...
    
    EAGLContext* _context;
    GLuint _edgeVBO;
    GLuint _videoProgram;
    GLuint _edgesProgram;
    CVOpenGLESTextureCacheRef _videoTextureCache;
    CVOpenGLESTextureRef _texture;
}

// Methods...

- (void)cleanUpTextures;
- (void)setupAVCapture;
- (void)tearDownAVCapture;

- (void)setupGLBuffers;
- (void)setupGL;
- (void)tearDownGL;

- (BOOL)loadShaders:(GLuint)prog prefix:(NSString *)prefix;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;

- (void)setupCanny:(vx_context)context width:(int)width height:(int)height;
- (void)doCanny;
- (void)releaseCanny;

@end

////////////////////////////////////////////////////////////////

// Implementation

@implementation ViewController

#pragma mark -

- (void)viewDidUnload
{
    [super viewDidUnload];
    
    [self tearDownAVCapture];
    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == _context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    vxReleaseContext(&_vxContext);
}

#pragma mark -

- (void)tearDownAVCapture
{
    [self cleanUpTextures];
    
    CFRelease(_videoTextureCache);
    
    [_session stopRunning];
}

#pragma mark -

- (void)cleanUpTextures
{
    if (_texture)
    {
        CFRelease(_texture);
        _texture = NULL;
    }
    
    // Periodic texture cache flush every frame
    CVOpenGLESTextureCacheFlush(_videoTextureCache, 0);
}

#pragma mark -

- (void)releaseCanny
{
    vxReleaseGraph(_graph); // Frees all resources associated
    
    free(_baseAddress);
    free(_edgeAddress);
    
    _graph = NULL;
}

#pragma mark -

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection
{
    CVImageBufferRef _pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    // Lock the pixel buffer
    if(kCVReturnSuccess != CVPixelBufferLockBaseAddress(_pixelBuffer,0)){
        NSLog(@"Unable to lock image buffer.");
    }

    // Record this data (and how to rotate it)...
    size_t width = CVPixelBufferGetWidth(_pixelBuffer);
    size_t height = CVPixelBufferGetHeight(_pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(_pixelBuffer);
    size_t bytesPerRowOut = 4*height*sizeof(unsigned char);
    
    // Have to rotate this here as pixelBuffer is always passed in *landscape* (even if filming in portrait mode)...
    @synchronized(self){
        vImage_Buffer ibuff = { CVPixelBufferGetBaseAddress(_pixelBuffer), height, width, bytesPerRow};
        vImage_Buffer obuff = { _baseAddress, width, height, bytesPerRowOut}; // Note flip height<->width AND write to _baseAddress
        uint8_t rotationConstant = 1; // 1=rotate 90deg counterclock 2=180 3=270
        uint8_t blackback[4] = { 1,0,0,0 };
        vImage_Error imgerr= vImageRotate90_ARGB8888 (&ibuff, &obuff, rotationConstant, blackback ,0);
        if (imgerr != kvImageNoError) NSLog(@"%ld", imgerr);
    }
    
    //Unlock the image buffer
    CVPixelBufferUnlockBaseAddress(_pixelBuffer,0);
    
    if (!_videoTextureCache)
    {
        NSLog(@"No video texture cache");
        return;
    }
    
    [self cleanUpTextures];
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(CVOpenGLESTextureGetTarget(_texture), CVOpenGLESTextureGetName(_texture));
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _textureWidth, _textureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, _baseAddress);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

#pragma mark -

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Create OpenGL context and set up view...
    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!_context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = _context;
    self.preferredFramesPerSecond = 60;
    
    _screenWidth = [UIScreen mainScreen].bounds.size.width;
    _screenHeight = [UIScreen mainScreen].bounds.size.height;
    view.contentScaleFactor = [UIScreen mainScreen].scale;

    [self setupGL];
    
    // Initialise this input buffer to store RGBA pixels from camera (session will request 640x480x4channel).
    _baseAddress = (uint8_t*)malloc(4*480*640*sizeof(uint8_t));
    // And set this output buffer for grayscale...
    _edgeAddress = (uint8_t*)malloc(sizeof(uint8_t)*480*640); // NB. single depth
    
    // Init OpenVX (will point the graph at these buffers)...
    _vxContext = nil;
    _vxContext = vxCreateContext();
    [self setupCannyGraph:_vxContext width:480 height:640];
    //vx_set_debug_zone(VX_ZONE_GRAPH); // Set this if you want debug output in the log
    // NOTE: there is seemingly a bit of an internal memory leak with debug output.

    // Bit of magic to allows us to test most of this in simulator...
#if !(TARGET_IPHONE_SIMULATOR)
    [self setupAVCapture]; // If on real device - will attempt to use camera.
#else
    [self setupStaticImage]; // Otherwise, will just load in test image for simulator.
#endif
    
    [self setupGLBuffers];
    
    // Make sure we allocate for edges as well...
    _edges = malloc(sizeof(Edge)*MAX_EDGES);
    _nEdges = 0;
    
}

#pragma mark -

- (void)setupStaticImage
{
    // Alternatively, for simulation - this will load a texture from file!
    NSError *error;
    glGetError(); // Clear existing errors...
    // notice flip for opengl render option
    NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:GLKTextureLoaderOriginBottomLeft];
    GLKTextureInfo *texture = [GLKTextureLoader textureWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"testimage" ofType:@"png"] options:options error:&error];
    if (error) NSLog(@"Loading texture from file error: %@", error);
    //else NSLog(@"Loaded texture of %d x %d\n", texture.width,texture.height);
    
    _textureWidth = texture.width;
    _textureHeight = texture.height;
    
    GLuint offscreen_framebuffer;
    glGenFramebuffers(1, &offscreen_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, offscreen_framebuffer);
    
    //Create texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture.target, texture.name);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Bind framebuffer to texture so we can use glReadPixels on it...
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.name, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"failed to make complete framebuffer object %x", status);
    }
    
    // Read bytes out of texture into _baseAddress...
    @synchronized(self){
        glReadPixels(0, 0, _textureWidth, _textureHeight, GL_BGRA, GL_UNSIGNED_BYTE, _baseAddress);
    }
}

#pragma mark -

- (void)setupAVCapture
{
    // Create CVOpenGLESTextureCacheRef for optimal CVImageBufferRef to GLES texture conversion.
#if COREVIDEO_USE_EAGLCONTEXT_CLASS_IN_API
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, _context, NULL, &_videoTextureCache);
#else
    CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, (__bridge void *)_context, NULL, &_videoTextureCache);
#endif
    if (err)
    {
        NSLog(@"Error at CVOpenGLESTextureCacheCreate %d", err);
        return;
    }
    
    // Setup session...
    _session = [[AVCaptureSession alloc] init];
    [_session beginConfiguration];
    [_session setSessionPreset:AVCaptureSessionPreset640x480]; // Fixed size here specified!
    
    // Record this same size here!
    _textureWidth = 480; // NB. note portrait orientation
    _textureHeight = 640;
    
    // Create video device and add to session as input...
    AVCaptureDevice * videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if(videoDevice == nil)
        assert(0);
    NSError *error;
    AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
    if(error)
        NSLog(@"ERROR: trying to open camera: %@", error);
    [_session addInput:input];
    
    // Set output device properties - capturing 32BGRA!
    
    AVCaptureVideoDataOutput *dataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [dataOutput setVideoSettings:[NSDictionary dictionaryWithObject:[NSNumber numberWithInt:kCVPixelFormatType_32BGRA]
                                                         forKey:(id)kCVPixelBufferPixelFormatTypeKey]];
    [dataOutput setAlwaysDiscardsLateVideoFrames:YES];
    [dataOutput setSampleBufferDelegate:self queue:dispatch_get_main_queue()];
    [_session addOutput:dataOutput];
    
    // Ready to go!
    [_session commitConfiguration];
    [_session startRunning];
    
}

#pragma mark -

- (void)setupGL
{
    [EAGLContext setCurrentContext:_context];
    
    // Create shader programs (one for rendering texture, the other for edges)...
    
    _videoProgram = glCreateProgram();
    [self loadShaders:_videoProgram prefix:@"VideoShader"];
    glBindAttribLocation(_videoProgram, ATTRIB_VERTEX, "position"); // Must be attached before linking!
    glBindAttribLocation(_videoProgram, ATTRIB_TEXCOORD, "texCoord");
    uniforms[UNIFORM_RGBA] = glGetUniformLocation(_videoProgram, "videoFrame");
    [self linkProgram:_videoProgram];
    glUniform1i(uniforms[UNIFORM_RGBA], 0); // Set to zero
    
    _edgesProgram = glCreateProgram();
    [self loadShaders:_edgesProgram prefix:@"EdgesShader"];
    glBindAttribLocation(_edgesProgram, ATTRIB_EDGE, "position");
    [self linkProgram:_edgesProgram];
    
    // Make sure this turned off so things will render as drawn...
    glDisable(GL_DEPTH_TEST);
    
    // And make sure we can blend our fragments...
    glEnable(GL_BLEND);
    
    // And we're doing textures...
    glEnable(GL_TEXTURE);
    
    // Clear colour - RED so we know something gone wrong...
    glClearColor(1.0, 0.0, 0.0, 0.0);
}

#pragma mark -

- (void)setupGLBuffers
{
    // Vertices_
    
    static const GLfloat squareVertices[] = {
        -1.0f, 1.0f, // Image origin is top left!
        -1.0f, -1.0f, // go anticlockwise
        1.0f,  1.0f, // But as a TRIANGLE_STRIP
        1.0f, -1.0f,
        
    };

    static const GLfloat textureVertices[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f,  1.0f,
        0.0f,  0.0f,
    };
    
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, 0, 0, textureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    
    glGenBuffers(1, &_edgeVBO);
    glBindBuffer(GL_ARRAY_BUFFER,_edgeVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_EDGES * sizeof(Edge), _edges, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_EDGE);
    glVertexAttribPointer(ATTRIB_EDGE, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

#pragma mark -

- (void)setupCannyGraph:(vx_context)context width:(int)width height:(int)height
{
    // Describe the input image data format.. // NOTE rgba = 4 depth
    vx_imagepatch_addressing_t in_addrs[] = {
        {width,height,sizeof(vx_uint8)*4,width * sizeof(vx_uint8)*4,VX_SCALE_UNITY,VX_SCALE_UNITY, 1, 1}
    };
    
    void* in_ptrs[] = { // RGB data is interleaved bytes...
       _baseAddress
    };
    
    // Create an input image mapped to host memory
    vx_image _inputimg = vxCreateImageFromHandle(context, VX_DF_IMAGE_RGB, in_addrs, in_ptrs, VX_IMPORT_TYPE_HOST);
    
    if (vxGetStatus((vx_reference)_inputimg) != VX_SUCCESS)
    {
        vxReleaseImage(&_inputimg);
    }
    
    // Create an output image as well from buffer...
    vx_imagepatch_addressing_t out_addrs[] = {
        {width, height, sizeof(vx_uint8), width*sizeof(vx_uint8), VX_SCALE_UNITY, VX_SCALE_UNITY, 1, 1}
    };
    
    void* out_ptrs[] = { // This is where we will write the edges to...
        _edgeAddress
    };
    
    vx_image _edgesimg = vxCreateImageFromHandle(context, VX_DF_IMAGE_U8, out_addrs, out_ptrs, VX_IMPORT_TYPE_HOST);
    //_edgesimg = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
    
    if (vxGetStatus((vx_reference)_edgesimg) != VX_SUCCESS)
    {
        vxReleaseImage(&_edgesimg);
    }
    
    // Create an OpenVX graph
    _graph = vxCreateGraph(context);
    if (!_graph) {
        NSLog(@"ERROR: failed to create graph!\n");
        return;
    }
    
    // And some virtual images to use.
    vx_image virts[] = {
        vxCreateVirtualImage(_graph,0,0,VX_DF_IMAGE_IYUV),
        vxCreateVirtualImage(_graph,0,0,VX_DF_IMAGE_U8),
    };
    
    if (_inputimg && _edgesimg) // Only proceed if images were created successfully
    {
        // Add a color convert node to the graph, reading from the input image
        if (!vxColorConvertNode(_graph, _inputimg, virts[0]))
        {
            NSLog(@"ERROR: failed to create color convert node!\n");
            [self releaseCanny];
        }
        
        // Add a channel convert node to the graph
        if (!vxChannelExtractNode(_graph, virts[0], VX_CHANNEL_Y,virts[1]))
        {
            NSLog(@"ERROR: failed to create channel extract node!\n");
            [self releaseCanny];
        }
        
        vx_threshold hyst = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
        vx_int32 lower = 50, upper = 100;
        vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_LOWER, &lower, sizeof(lower));
        vxSetThresholdAttribute(hyst, VX_THRESHOLD_ATTRIBUTE_THRESHOLD_UPPER, &upper, sizeof(upper));
        
        // Add a Canny node to the graph, linking the graysale and edges images
        if (!vxCannyEdgeDetectorNode(_graph, virts[1], hyst, 3, VX_NORM_L1, _edgesimg))
        {
            NSLog(@"ERROR: failed to create canny node!\n");
            [self releaseCanny];
        }
        
    }
    else
    {
        NSLog(@"ERROR: failed to create input/output images!\n");
        [self releaseCanny];
    }
    
}

#pragma mark -

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:_context];
    
    glDeleteBuffers(1, &_edgeVBO);
    
    if (_videoProgram) {
        glDeleteProgram(_videoProgram);
        _videoProgram = 0;
    }
    if (_edgesProgram) {
        glDeleteProgram(_edgesProgram);
        _edgesProgram = 0;
    }
}

#pragma mark - GLKViewDelegate

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(_videoProgram); // Use this to draw video
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glUseProgram(_edgesProgram); // Use this to draw edges
    
    glDrawArrays(GL_POINTS, 0, _nEdges);

}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    @synchronized(self){
        if (_baseAddress!=nil) [self doCanny];
        
        // Update buffer with these edges...
        glBufferData(GL_ARRAY_BUFFER, _nEdges * sizeof(Edge), _edges, GL_DYNAMIC_DRAW);
    }
}

#pragma mark - doCanny

- (void) doCanny
{
    vx_status status = VX_FAILURE;
    if (_graph) {
        
        // Verify the graph for safe execution
        if ((status=vxVerifyGraph(_graph)) != VX_SUCCESS) {
            NSLog(@"ERROR: failed to verify graph! vx_status: %i\n",status);
            vxVerifyGraph(_graph);
            return;
        }
        
        // Execute the graph;
        // Report if the graph failed to execute
        if ((status=vxProcessGraph(_graph)) != VX_SUCCESS) {
            NSLog(@"ERROR: failed to run graph! vx_status: %i\n",status);
            return;
        }

        // Convert to edges (really, would be better if canny graph returned these directly)...

        _nEdges = 0;
        
        uint8_t pixel;
        GLuint checked = 0;
        
        for (int y=0;y<640;y++) // Hardcoded dimensions here - look out!
        {
            for (int x=0;x<480;x++)
            {
                pixel = _edgeAddress[checked];
                checked++;
                if (pixel!=0) // I.e. is part of a detected edge!
                {
                    _edges[_nEdges].x = -((GLfloat)-1.0+ ((float)x/(float)480)*2.0); // image coords are neg-x
                    _edges[_nEdges].y = (GLfloat)-1.0+ ((float)y/(float)640)*2.0;
                    _nEdges++;
                    if (_nEdges==MAX_EDGES) // Jump out if hit maximum hard-coded limit!
                        goto esc;
                }
            }
        }
    esc:
        return;

    }
}


#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders:(GLuint)prog prefix:(NSString *)prefix;
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:prefix ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:prefix ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }
    
    // Attach vertex shader to program.
    glAttachShader(prog, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(prog, fragShader);
    
    return YES;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) {
        NSLog(@"Failed to load vertex shader");
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return NO;
    }
    
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    
    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    
    return YES;
}


@end
