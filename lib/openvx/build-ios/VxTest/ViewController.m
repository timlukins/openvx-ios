//
//  ViewController.m
//  VxTest
//
//  Created by Tim Lukins on 23/10/2015.
//
//

#import "ViewController.h"

#include "OpenVXTests.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (IBAction)handleRunClick:(id)sender {
    

    // Basically run the current (hardwired) tests for 100 iterations.
    
    int iters = 100;
    NSTimeInterval time;
    
    NSMutableString *str = [@"Starting...\n" mutableCopy];
    self.testResults.text = str;
    
    [str appendString: [NSString stringWithFormat:@"Ran test_vxAbsDiff in %fsec\n",test_vxAbsDiff(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]]; // Hacky way to let main loop post UI update

    [str appendString: [NSString stringWithFormat:@"Ran test_vxCanny in %fsec\n",test_vxCanny(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
    
    [str appendString: [NSString stringWithFormat:@"Ran test_vxADilate3x3 in %fsec\n",test_vxDilate3x3(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];

    [str appendString: [NSString stringWithFormat:@"Ran test_vxEqualizeHist in %fsec\n",test_vxEqualizeHist(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
    
    [str appendString: [NSString stringWithFormat:@"Ran test_vxMeanStdDev in %fsec\n",test_vxMeanStdDev(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
    
    [str appendString: [NSString stringWithFormat:@"Ran test_vxOr in %fsec\n",test_vxOr(iters)]];
    self.testResults.text = str;
    [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.5]];
    
    [str appendString: @"...finished!"];
    self.testResults.text = str;
}


@end
