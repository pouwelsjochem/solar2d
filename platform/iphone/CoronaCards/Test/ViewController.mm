//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#import "CoronaCards/Test/ViewController.h"
#import <Foundation/Foundation.h>

@interface ViewController ()
{
}
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

	CoronaView *view = (CoronaView *)self.view;
	
	NSDictionary *params = nil;

	[view runWithPath:nil parameters:params];
}

@end
