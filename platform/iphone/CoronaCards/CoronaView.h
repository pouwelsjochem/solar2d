//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifdef Rtt_MetalANGLE
#import <MetalANGLE/MGLKit.h>
#else
#import <GLKit/GLKit.h>
#endif

// ----------------------------------------------------------------------------
#ifdef Rtt_MetalANGLE
@interface CoronaView : MGLKView
#else
@interface CoronaView : GLKView
#endif

- (NSInteger)run;
- (NSInteger)runWithPath:(NSString*)path parameters:(NSDictionary *)params;
- (void)suspend;
- (void)resume;

- (id)sendEvent:(NSDictionary *)event;

@end

// ----------------------------------------------------------------------------

