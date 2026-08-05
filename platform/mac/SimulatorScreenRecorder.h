//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@solar2d.com
//
//////////////////////////////////////////////////////////////////////////////

#import <AppKit/AppKit.h>

// ----------------------------------------------------------------------------

typedef NS_ENUM(NSInteger, SimulatorScreenRecorderState)
{
	SimulatorScreenRecorderStateIdle = 0,
	SimulatorScreenRecorderStateStarting,
	SimulatorScreenRecorderStateRecording,
	SimulatorScreenRecorderStateStopping
};

@class SimulatorScreenRecorder;

@protocol SimulatorScreenRecorderDelegate <NSObject>

- (void)screenRecorder:(SimulatorScreenRecorder *)recorder
	didChangePhase:(NSString *)phase
	outputURL:(NSURL *)outputURL
	error:(NSError *)error;

@end

// Records only the Simulator's device view. ScreenCaptureKit performs capture
// and scaling outside of Solar2D's render loop, and SCRecordingOutput handles
// encoding and file finalization.
@interface SimulatorScreenRecorder : NSObject

@property(nonatomic, assign) id<SimulatorScreenRecorderDelegate> delegate;
@property(nonatomic, readonly) SimulatorScreenRecorderState state;
@property(nonatomic, readonly, copy) NSURL *outputURL;

- (id)initWithDelegate:(id<SimulatorScreenRecorderDelegate>)delegate;

- (BOOL)startRecordingWindow:(NSWindow *)window
	sourceView:(NSView *)sourceView
	outputURL:(NSURL *)outputURL
	framesPerSecond:(NSInteger)framesPerSecond
	resolutionScale:(double)resolutionScale
	includeAudio:(BOOL)includeAudio
	showsCursor:(BOOL)showsCursor
	error:(NSError **)error;

- (BOOL)stopRecording:(NSError **)error;

@end
