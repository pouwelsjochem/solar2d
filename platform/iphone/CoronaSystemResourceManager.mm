//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "CoronaSystemResourceManager.h"

#ifdef Rtt_ORIENTATION
#import "CoronaOrientationObserver.h"
#endif

#ifdef Rtt_CORE_MOTION
#import <CoreMotion/CoreMotion.h>
#import "CoronaGyroscopeObserver.h"
#endif

#import <UIKit/UIDevice.h>

@protocol UIAccelerometerDelegate;


// Resource Keys
// ----------------------------------------------------------------------------
#pragma mark # Resource Keys

NSString * const CoronaOrientationResourceKey()
{
	return @"Orientation";
}

NSString * const CoronaAccelerometerResourceKey()
{
	return @"Accelerometer";
}

NSString * const CoronaGyroscopeResourceKey()
{
	return @"Gyroscope";
}


// CoronaSystemResourceManager()
// ----------------------------------------------------------------------------
#pragma mark # CoronaSystemResourceManager()

@interface CoronaSystemResourceManager()

@property (nonatomic, retain) NSMutableDictionary *observersByKey;

// - (BOOL)validateObserver:(id)observer forKey:(NSString *)key;

- (NSMutableSet *)observersForKey:(NSString *)key;
- (BOOL)hasObserversForKey:(NSString *)key;

//- (void)removeObserver:(id)observer;
- (void)removeObservers;
- (void)removeObserversForKey:(NSString *)key;

#ifdef Rtt_ORIENTATION
- (void)addObserverOrientation:(id <CoronaOrientationObserver>)observer;
- (void)removeObserverOrientation:(id <CoronaOrientationObserver>)observer;
#endif

#ifdef Rtt_ACCELEROMETER
- (void)addObserverAccelerometer:(id <UIAccelerometerDelegate>)observer;
- (void)removeObserverAccelerometer:(id <UIAccelerometerDelegate>)observer;
#endif

#ifdef Rtt_CORE_MOTION
- (void)addObserverGyroscope:(id )observer;
- (void)removeObserverGyroscope:(id)observer;
#endif

@end


// CoronaSystemResourceManager
// ----------------------------------------------------------------------------
#pragma mark # CoronaSystemResourceManager

@implementation CoronaSystemResourceManager

+ (instancetype)sharedInstance
{
	static id sInstance = nil;
	static dispatch_once_t sOnce = 0;
	
	dispatch_once(&sOnce, ^{
		sInstance = [[self alloc] init];
	});

	return sInstance;
}

+ (NSSet *)validKeys
{
	static NSSet *sKeys = nil;

	if ( nil == sKeys )
	{
		NSArray *keys = [NSArray arrayWithObjects:
			CoronaAccelerometerResourceKey(),
#ifdef Rtt_ORIENTATION
			CoronaOrientationResourceKey(),
#endif
#ifdef Rtt_CORE_MOTION
			CoronaGyroscopeResourceKey(),
#endif

			nil];
		sKeys = [[NSSet alloc] initWithArray:keys];
	}

	return sKeys;
}

- (instancetype)init
{
	self = [super init];
	if ( self )
	{
		_observersByKey = [NSMutableDictionary new];
	}
	return self;
}

- (void)dealloc
{
#ifdef Rtt_CORE_MOTION
	[_motionManager release];
#endif

	[self removeObservers];
	[_observersByKey release];

	[super dealloc];
}

#ifdef Rtt_CORE_MOTION
- (CMMotionManager *)motionManager
{
	if ( nil == _motionManager )
	{
		_motionManager = [[CMMotionManager alloc] init];
	}
	
	return _motionManager;
}
#endif

- (BOOL)resourceAvailableForKey:(NSString *)key
{
	BOOL result = [[[self class] validKeys] containsObject:key];
	
	if ( result )
	{
#ifdef Rtt_CORE_MOTION
		// Additional checks
		if ( [key isEqualToString:CoronaGyroscopeResourceKey()] )
		{
			result = [self motionManager].gyroAvailable;
		}
#endif
	}
	
	return result;
}

- (NSMutableSet *)observersForKey:(NSString *)key
{
	NSMutableSet *result = [self.observersByKey valueForKey:key];
	
	// Initialize set if none exists
	if ( ! result
			&& [[CoronaSystemResourceManager validKeys] containsObject:key] )
	{
		result = [NSMutableSet set];
		[self.observersByKey setValue:result forKey:key];
	}
	
	return result;
}

- (BOOL)hasObserversForKey:(NSString *)key
{
	return ( [[self observersForKey:key] count] > 0 );
}

- (void)addObserver:(id)observer forKey:(NSString *)key
{
	SEL sel = NSSelectorFromString( [NSString stringWithFormat:@"addObserver%@:", key] );
	if ( [self respondsToSelector:sel] )
	{
		[self performSelector:sel withObject:observer];

		// Update observer bookkeeping *after* system-specific operation
		[[self observersForKey:key] addObject:observer];
	}
}

- (void)removeObserver:(id)observer forKey:(NSString *)key
{
	SEL sel = NSSelectorFromString( [NSString stringWithFormat:@"removeObserver%@:", key] );
	if ( [self respondsToSelector:sel] )
	{
		// Update observer bookkeeping *before* system-specific operation
		[[self observersForKey:key] removeObject:observer];

		[self performSelector:sel withObject:observer];
	}
}

// Convenience function if you don't actually know what resource type
/*
- (void)removeObserver:(id)observer
{
	for ( NSString *key in [CoronaSystemResourceManager validKeys] )
	{
		[self removeObserver:observer forKey:key];
	}
}
*/

- (void)removeObservers
{
	NSSet *validKeys = [[self class] validKeys];
	for ( NSString *key in validKeys )
	{
		[self removeObserversForKey:key];
	}
}

- (void)removeObserversForKey:(NSString *)key
{
	SEL sel = NSSelectorFromString( [NSString stringWithFormat:@"removeObserver%@:", key] );
	if ( [self respondsToSelector:sel] )
	{
		NSMutableSet *observersSrc = [self observersForKey:key];

		// Update observer bookkeeping *before* system-specific operation
		NSSet *observersCopy = [[observersSrc copy] autorelease]; // Copy before removing
		[observersSrc removeAllObjects];

		for ( id observer in observersCopy )
		{
			[self performSelector:sel withObject:observer];
		}
	}
}

// -----------------------------------------------------------------------------

#ifdef Rtt_ORIENTATION

- (void)addObserverOrientation:(id <CoronaOrientationObserver>)observer
{
	if ( ! [self hasObserversForKey:CoronaOrientationResourceKey()] )
	{
		[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	}
}

- (void)removeObserverOrientation:(id <CoronaOrientationObserver>)observer
{
	if ( ! [self hasObserversForKey:CoronaOrientationResourceKey()] )
	{
		[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
	}
}

#endif // Rtt_ORIENTATION

// -----------------------------------------------------------------------------

#ifdef Rtt_ACCELEROMETER

- (void)addObserverAccelerometer:(id <UIAccelerometerDelegate>)observer
{
	;
}

- (void)removeObserverAccelerometer:(id <UIAccelerometerDelegate>)observer
{
	;
}

#endif // Rtt_ACCELEROMETER

// -----------------------------------------------------------------------------

#ifdef Rtt_CORE_MOTION

- (void)addObserverGyroscope:(id)observer
{
	NSString *key = CoronaGyroscopeResourceKey();
	if ( [self resourceAvailableForKey:key] )
	{
		if ( ! [self hasObserversForKey:key] )
		{
			// For games/best performance, Apple recommends polling periodically
			// instead of registering a block/callback handler
			[self.motionManager startGyroUpdates];
		}

		if ( [observer conformsToProtocol:@protocol(CoronaGyroscopeObserver)] )
		{
			[observer startPolling];
		}
	}
}

- (void)removeObserverGyroscope:(id)observer
{
	NSString *key = CoronaGyroscopeResourceKey();
	if ( [self resourceAvailableForKey:key] )
	{
		if ( ! [self hasObserversForKey:key] )
		{
			[self.motionManager stopGyroUpdates];
		}

		if ( [observer conformsToProtocol:@protocol(CoronaGyroscopeObserver)] )
		{
			[observer stopPolling];
		}
	}
}

#endif // Rtt_CORE_MOTION

// Accelerometer
// ----------------------------------------------------------------------------
#pragma mark # Accelerometer

// Gyroscope
// ----------------------------------------------------------------------------
#pragma mark # Gyroscope

@end

// ----------------------------------------------------------------------------
