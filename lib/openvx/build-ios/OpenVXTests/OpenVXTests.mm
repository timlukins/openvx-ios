//
//  OpenVXTests.m
//  OpenVXTests
//
//  Created by Tim Lukins on 28/06/2015.
//
//  Based on: https://github.com/mattstevens/xcode-googletest

/*
 * Copyright (c) 2013 Matthew Stevens
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>
#import <gtest/gtest.h>
#import <objc/runtime.h>

#include "vxtest.h"

using testing::TestCase;
using testing::TestInfo;
using testing::TestPartResult;
using testing::UnitTest;

static NSString * const GoogleTestDisabledPrefix = @"DISABLED_";

/**
 * Class prefix used for generated Objective-C class names.
 *
 * If a class name generated for a Google Test case conflicts with an existing
 * class the value of this variable can be changed to add a class prefix.
 */
static NSString * const GeneratedClassPrefix = @"";

/**
 * Map of test keys to Google Test filter strings.
 *
 * Some names allowed by Google Test would result in illegal Objective-C
 * identifiers and in such cases the generated class and method names are
 * adjusted to handle this. This map is used to obtain the original Google Test
 * filter string associated with a generated Objective-C test method.
 */
static NSDictionary *GoogleTestFilterMap;

/**
 * A Google Test listener that reports failures to XCTest.
 */
class XCTestListener : public testing::EmptyTestEventListener {
public:
    XCTestListener(XCTestCase *testCase) :
    _testCase(testCase) {}
    
    void OnTestPartResult(const TestPartResult& test_part_result) {
        if (test_part_result.passed())
            return;
        
        int lineNumber = test_part_result.line_number();
        const char *fileName = test_part_result.file_name();
        NSString *path = fileName ? [@(fileName) stringByStandardizingPath] : nil;
        NSString *description = @(test_part_result.message());
        [_testCase recordFailureWithDescription:description
                                         inFile:path
                                         atLine:(lineNumber >= 0 ? (NSUInteger)lineNumber : 0)
                                       expected:YES];
    }
    
private:
    XCTestCase *_testCase;
};

/**
 * Registers an XCTestCase subclass for each Google Test case.
 *
 * Generating these classes allows Google Test cases to be represented as peers
 * of standard XCTest suites and supports filtering of test runs to specific
 * Google Test cases or individual tests via Xcode.
 */
@interface GoogleTestLoader : NSObject
@end

/**
 * Base class for the generated classes for Google Test cases.
 */
@interface OpenVXTestCase : XCTestCase
@end

@implementation OpenVXTestCase

/**
 * Associates generated Google Test classes with the test bundle.
 *
 * This affects how the generated test cases are represented in reports. By
 * associating the generated classes with a test bundle the Google Test cases
 * appear to be part of the same test bundle that this source file is compiled
 * into. Without this association they appear to be part of a bundle
 * representing the directory of an internal Xcode tool that runs the tests.
 */
+ (NSBundle *)bundleForClass {
    return [NSBundle bundleForClass:[GoogleTestLoader class]];
}

/**
 * Implementation of +[XCTestCase testInvocations] that returns an array of test
 * invocations for each test method in the class.
 *
 * This differs from the standard implementation of testInvocations, which only
 * adds methods with a prefix of "test".
 */
+ (NSArray *)testInvocations {
    NSMutableArray *invocations = [NSMutableArray array];
    
    unsigned int methodCount = 0;
    Method *methods = class_copyMethodList([self class], &methodCount);
    
    for (unsigned int i = 0; i < methodCount; i++) {
        SEL sel = method_getName(methods[i]);
        NSMethodSignature *sig = [self instanceMethodSignatureForSelector:sel];
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:sig];
        [invocation setSelector:sel];
        [invocations addObject:invocation];
    }
    
    free(methods);
    
    return invocations;
}

@end

/**
 * Runs a single test.
 */
// TCL: this is added below to every instance of instantiated TestClass

static void RunTest(id self, SEL _cmd) {
 
    XCTestListener *listener = new XCTestListener(self);
    UnitTest *googleTest = UnitTest::GetInstance();
    googleTest->listeners().Append(listener);
    
    // TCL: this is where the problem with XML output is!
    // BY filtering individual tests we override the output file each time...
    
    NSString *testKey = [NSString stringWithFormat:@"%@.%@", [self class], NSStringFromSelector(_cmd)];
    NSString *testFilter = GoogleTestFilterMap[testKey];
    XCTAssertNotNil(testFilter, @"No test filter found for test %@", testKey);
    
    testing::GTEST_FLAG(filter) = [testFilter UTF8String];
    
    (void)RUN_ALL_TESTS();
    
    delete googleTest->listeners().Release(listener);
    
    int totalTestsRun = googleTest->successful_test_count() + googleTest->failed_test_count();
    XCTAssertEqual(totalTestsRun, 1, @"Expected to run a single test for filter \"%@\"", testFilter);
}

@implementation GoogleTestLoader

/**
 * Performs registration of classes for Google Test cases after our bundle has
 * finished loading.
 *
 * This registration needs to occur before XCTest queries the runtime for test
 * subclasses, but after C++ static initializers have run so that all Google
 * Test cases have been registered. This is accomplished by synchronously
 * observing the NSBundleDidLoadNotification for our own bundle.
 */
+ (void)load {
    NSBundle *bundle = [NSBundle bundleForClass:self];
    [[NSNotificationCenter defaultCenter] addObserverForName:NSBundleDidLoadNotification object:bundle queue:nil usingBlock:^(NSNotification *notification) {
        [self registerTestClasses];
    }];
}

+ (void)registerTestClasses {
    // Pass the command-line arguments to Google Test to support the --gtest options
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    
    int i = 0;
    int argc = (int)[arguments count];
    const char **argv = (const char **)calloc((unsigned int)argc + 1, sizeof(const char *));
    for (NSString *arg in arguments) {
        argv[i++] = [arg UTF8String];
    }
    
    testing::InitGoogleTest(&argc, (char **)argv);
    UnitTest *googleTest = UnitTest::GetInstance();
    // TCL Remove 2 lines below if want normal gtest reporter
    testing::TestEventListeners& listeners = googleTest->listeners();
    delete listeners.Release(listeners.default_result_printer());
    free(argv);
    
    BOOL runDisabledTests = testing::GTEST_FLAG(also_run_disabled_tests);
    NSMutableDictionary *testFilterMap = [NSMutableDictionary dictionary];
    NSCharacterSet *decimalDigitCharacterSet = [NSCharacterSet decimalDigitCharacterSet];
    
    for (int testCaseIndex = 0; testCaseIndex < googleTest->total_test_case_count(); testCaseIndex++) {
        const TestCase *testCase = googleTest->GetTestCase(testCaseIndex);
        NSString *testCaseName = @(testCase->name());
        
        // For typed tests '/' is used to separate the parts of the test case name.
        NSArray *testCaseNameComponents = [testCaseName componentsSeparatedByString:@"/"];
        
        if (runDisabledTests == NO) {
            BOOL testCaseDisabled = NO;
            
            for (NSString *component in testCaseNameComponents) {
                if ([component hasPrefix:GoogleTestDisabledPrefix]) {
                    testCaseDisabled = YES;
                    break;
                }
            }
            
            if (testCaseDisabled) {
                continue;
            }
        }
        
        // Join the test case name components with '_' rather than '/' to create
        // a valid class name.
        NSString *className = [GeneratedClassPrefix stringByAppendingString:[testCaseNameComponents componentsJoinedByString:@"_"]];
        //NSLog(@">>>>>>>>>>>>>>> %@",className); // Find all the test classes == Test Suite Name(s) X TEST(X,test_name)
        //  TCL: create a objc class of type GoogleTestCase - which is a XCTestCase with this name...
        // TCL: alternatively, just append all these methods to existing class
        Class testClass = [OpenVXTestCase class];//objc_allocateClassPair([OpenVXTestCase class], [className UTF8String], 0);
        // TCL: added only one instance of this listener!
        // NOTE: all this code will break if their is an unexpected other test case in the top loop!
        // TODO: fix
        //XCTestListener *listener = new XCTestListener(testClass);
        //googleTest->listeners().Append(listener);

        NSAssert1(testClass, @"Failed to register Google Test class \"%@\", this class may already exist. The value of GeneratedClassPrefix can be changed to avoid this.", className);
        BOOL hasMethods = NO;
        
        for (int testIndex = 0; testIndex < testCase->total_test_count(); testIndex++) {
            const TestInfo *testInfo = testCase->GetTestInfo(testIndex);
            NSString *testName = @(testInfo->name());
            if (runDisabledTests == NO && [testName hasPrefix:GoogleTestDisabledPrefix]) {
                continue;
            }
            
            // Google Test allows test names starting with a digit, prefix these with an
            // underscore to create a valid method name.
            NSString *methodName = testName;
            if ([methodName length] > 0 && [decimalDigitCharacterSet characterIsMember:[methodName characterAtIndex:0]]) {
                methodName = [@"_" stringByAppendingString:methodName];
            }
            
            // TCL can remove this if just want to RUN_ALL_TESTS - (but won't have report from XCTest)
            
            NSString *testKey = [NSString stringWithFormat:@"%@.%@", className, methodName];
            NSString *testFilter = [NSString stringWithFormat:@"%@.%@", testCaseName, testName];
            testFilterMap[testKey] = testFilter;
            
            SEL selector = sel_registerName([methodName UTF8String]);
            BOOL added = class_addMethod(testClass, selector, (IMP)RunTest, "v@:");
            NSAssert1(added, @"Failed to add Google Test method \"%@\", this method may already exist in the class.", methodName);
            hasMethods = YES;
        }
        
        /*
        if (hasMethods) {
            objc_registerClassPair(testClass);
        } else {
            objc_disposeClassPair(testClass);
        }
         */
    }
    
    GoogleTestFilterMap = testFilterMap;
    
    // TCL if just want to run all the tests
    
    //(void)RUN_ALL_TESTS();
    
}

@end

/***********************************************************************************/

/* Implementations of vxtest methods for iOS! */

vx_image vxTestGetImage(vx_context context, const char* filename)
{
    vx_image inimg;
    
    // This is definitely the test bundle, so use this!
    CFBundleRef inBundle  = CFBundleGetBundleWithIdentifier(CFSTR("com.machineswithvision.openvxtests"));
    
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

void vxTestLogStart()
{
    methodStart = [NSDate date];
}

void vxTestLogEnd(char* info)
{
    NSDate *methodFinish = [NSDate date];
    NSTimeInterval executionTime = [methodFinish timeIntervalSinceDate:methodStart];
    printf(">>> %s : %f seconds\n", info,executionTime);
}

