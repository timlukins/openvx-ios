//
//  ViewController.h
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

#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>

// Notice that for the iOS, the framework name "OpenVX" is referenced!
#import <OpenVX/vx.h>
#import <OpenVX/vxu.h>

// Public interface

@interface ViewController : GLKViewController <AVCaptureVideoDataOutputSampleBufferDelegate>

@end