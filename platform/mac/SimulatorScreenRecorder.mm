//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@solar2d.com
//
//////////////////////////////////////////////////////////////////////////////

#import "SimulatorScreenRecorder.h"

#import <AVFoundation/AVFoundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

// ----------------------------------------------------------------------------

// This class refuses initialization before macOS 15. AppDelegate also guards
// construction, so every ScreenCaptureKit call below is behind that runtime
// availability invariant without spreading checks through each callback.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

static NSString *const kSimulatorScreenRecorderErrorDomain = @"com.solar2d.simulator.screen-recorder";

typedef NS_ENUM(NSInteger, SimulatorScreenRecorderError)
{
	SimulatorScreenRecorderErrorInvalidArgument = 1,
	SimulatorScreenRecorderErrorBusy,
	SimulatorScreenRecorderErrorUnavailable,
	SimulatorScreenRecorderErrorWindowNotFound,
	SimulatorScreenRecorderErrorDisplayNotFound
};

static NSError *
NewRecorderError(SimulatorScreenRecorderError code, NSString *message)
{
	NSDictionary *userInfo = [NSDictionary dictionaryWithObject:message forKey:NSLocalizedDescriptionKey];
	return [NSError errorWithDomain:kSimulatorScreenRecorderErrorDomain code:code userInfo:userInfo];
}

static size_t
EvenPixelDimension(CGFloat value)
{
	size_t result = (size_t)MAX(2.0, round(value));
	return result + (result & 1);
}

@interface SimulatorScreenRecorder () <SCRecordingOutputDelegate, SCStreamDelegate>
{
	SCStream *fStream;
	SCRecordingOutput *fRecordingOutput;
	NSURL *fOutputURL;
	SimulatorScreenRecorderState fState;
	NSUInteger fGeneration;

	CGWindowID fWindowID;
	CGDirectDisplayID fDisplayID;
	CGRect fSourceRect;
	NSInteger fFramesPerSecond;
	double fResolutionScale;
	BOOL fIncludeAudio;
	BOOL fShowsCursor;
}

- (void)beginRecordingWithShareableContent:(SCShareableContent *)shareableContent
	error:(NSError *)error
	generation:(NSUInteger)generation;
- (void)finishWithPhase:(NSString *)phase error:(NSError *)error removeOutputFile:(BOOL)removeOutputFile;
- (void)releaseCaptureObjects;

@end

@implementation SimulatorScreenRecorder

@synthesize delegate;

- (id)initWithDelegate:(id<SimulatorScreenRecorderDelegate>)value
{
	self = [super init];
	if (self)
	{
		if (@available(macOS 15.0, *))
		{
			delegate = value;
			fState = SimulatorScreenRecorderStateIdle;
			fGeneration = 0;
		}
		else
		{
			[self release];
			self = nil;
		}
	}
	return self;
}

- (void)dealloc
{
	delegate = nil;
	if (fStream)
	{
		[fStream stopCaptureWithCompletionHandler:nil];
	}
	[self releaseCaptureObjects];
	[super dealloc];
}

- (SimulatorScreenRecorderState)state
{
	return fState;
}

- (NSURL *)outputURL
{
	return [[fOutputURL copy] autorelease];
}

- (BOOL)startRecordingWindow:(NSWindow *)window
	sourceView:(NSView *)sourceView
	outputURL:(NSURL *)outputURL
	framesPerSecond:(NSInteger)framesPerSecond
	resolutionScale:(double)resolutionScale
	includeAudio:(BOOL)includeAudio
	showsCursor:(BOOL)showsCursor
	error:(NSError **)error
{
	if (fState != SimulatorScreenRecorderStateIdle)
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorBusy, @"A Simulator screen recording is already active.");
		}
		return NO;
	}
	if (!window || !sourceView || [sourceView window] != window)
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"The Simulator device view is not available.");
		}
		return NO;
	}
	if (!outputURL || ![outputURL isFileURL])
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"The recording path must be a local file URL.");
		}
		return NO;
	}
	if (framesPerSecond < 1 || framesPerSecond > 240)
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"Recording fps must be between 1 and 240.");
		}
		return NO;
	}
	if (!(resolutionScale > 0.0 && resolutionScale <= 1.0))
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"Recording resolution scale must be greater than 0 and no greater than 1.");
		}
		return NO;
	}

	NSScreen *screen = [window screen];
	NSNumber *screenNumber = [[screen deviceDescription] objectForKey:@"NSScreenNumber"];
	if (!screen || !screenNumber)
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorDisplayNotFound, @"The Simulator window is not on a capturable display.");
		}
		return NO;
	}

	NSRect viewRectInWindow = [sourceView convertRect:[sourceView bounds] toView:nil];
	NSRect viewRectOnScreen = [window convertRectToScreen:viewRectInWindow];
	NSRect screenFrame = [screen frame];
	NSRect clippedRect = NSIntersectionRect(viewRectOnScreen, screenFrame);
	if (NSIsEmptyRect(clippedRect))
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"The Simulator device view is outside the display bounds.");
		}
		return NO;
	}

	// AppKit screen coordinates have a bottom-left origin. ScreenCaptureKit's
	// display source rectangle has a top-left origin and is display-relative.
	fSourceRect = CGRectMake(
		NSMinX(clippedRect) - NSMinX(screenFrame),
		NSMaxY(screenFrame) - NSMaxY(clippedRect),
		NSWidth(clippedRect),
		NSHeight(clippedRect));
	fWindowID = (CGWindowID)[window windowNumber];
	fDisplayID = (CGDirectDisplayID)[screenNumber unsignedIntValue];
	fFramesPerSecond = framesPerSecond;
	fResolutionScale = resolutionScale;
	fIncludeAudio = includeAudio;
	fShowsCursor = showsCursor;
	fOutputURL = [outputURL copy];
	fState = SimulatorScreenRecorderStateStarting;
	fGeneration++;

	NSUInteger generation = fGeneration;
	[SCShareableContent getCurrentProcessShareableContentWithCompletionHandler:
		^(SCShareableContent *shareableContent, NSError *shareableContentError)
		{
			dispatch_async(dispatch_get_main_queue(),
				^{
					[self beginRecordingWithShareableContent:shareableContent
						error:shareableContentError
						generation:generation];
				});
		}];
	return YES;
}

- (void)beginRecordingWithShareableContent:(SCShareableContent *)shareableContent
	error:(NSError *)error
	generation:(NSUInteger)generation
{
	if (generation != fGeneration || fState == SimulatorScreenRecorderStateIdle)
	{
		return;
	}
	if (error || !shareableContent)
	{
		[self finishWithPhase:@"failed" error:error ?: NewRecorderError(
			SimulatorScreenRecorderErrorUnavailable, @"ScreenCaptureKit could not enumerate the Simulator window.")
			removeOutputFile:YES];
		return;
	}

	SCWindow *capturedWindow = nil;
	for (SCWindow *window in [shareableContent windows])
	{
		if ([window windowID] == fWindowID)
		{
			capturedWindow = window;
			break;
		}
	}
	if (!capturedWindow)
	{
		[self finishWithPhase:@"failed" error:NewRecorderError(
			SimulatorScreenRecorderErrorWindowNotFound, @"ScreenCaptureKit could not find the Simulator window.")
			removeOutputFile:YES];
		return;
	}

	SCDisplay *capturedDisplay = nil;
	for (SCDisplay *display in [shareableContent displays])
	{
		if ([display displayID] == fDisplayID)
		{
			capturedDisplay = display;
			break;
		}
	}
	if (!capturedDisplay)
	{
		[self finishWithPhase:@"failed" error:NewRecorderError(
			SimulatorScreenRecorderErrorDisplayNotFound, @"ScreenCaptureKit could not find the Simulator display.")
			removeOutputFile:YES];
		return;
	}

	SCContentFilter *filter = [[[SCContentFilter alloc]
		initWithDisplay:capturedDisplay includingWindows:[NSArray arrayWithObject:capturedWindow]] autorelease];
	// fSourceRect is measured in screen points. Use ScreenCaptureKit's scale so
	// transformed Simulator views retain the captured display's native pixels.
	CGFloat pointPixelScale = MAX(1.0, [filter pointPixelScale]);
	size_t outputWidth = EvenPixelDimension(CGRectGetWidth(fSourceRect) * pointPixelScale * fResolutionScale);
	size_t outputHeight = EvenPixelDimension(CGRectGetHeight(fSourceRect) * pointPixelScale * fResolutionScale);
	SCStreamConfiguration *streamConfiguration = [[[SCStreamConfiguration alloc] init] autorelease];
	[streamConfiguration setSourceRect:fSourceRect];
	[streamConfiguration setDestinationRect:CGRectMake(0, 0, outputWidth, outputHeight)];
	[streamConfiguration setWidth:outputWidth];
	[streamConfiguration setHeight:outputHeight];
	[streamConfiguration setScalesToFit:YES];
	[streamConfiguration setPreservesAspectRatio:YES];
	[streamConfiguration setMinimumFrameInterval:CMTimeMake(1, (int32_t)fFramesPerSecond)];
	[streamConfiguration setQueueDepth:5];
	[streamConfiguration setShowsCursor:fShowsCursor];
	[streamConfiguration setShowMouseClicks:NO];
	[streamConfiguration setIgnoreShadowsDisplay:YES];
	[streamConfiguration setShouldBeOpaque:YES];
	[streamConfiguration setCaptureResolution:SCCaptureResolutionBest];
	[streamConfiguration setCapturesAudio:fIncludeAudio];
	[streamConfiguration setExcludesCurrentProcessAudio:!fIncludeAudio];
	[streamConfiguration setSampleRate:48000];
	[streamConfiguration setChannelCount:2];
	[streamConfiguration setStreamName:@"Solar2D Simulator Screen Recording"];

	SCRecordingOutputConfiguration *recordingConfiguration =
		[[[SCRecordingOutputConfiguration alloc] init] autorelease];
	[recordingConfiguration setOutputURL:fOutputURL];
	[recordingConfiguration setOutputFileType:AVFileTypeMPEG4];
	[recordingConfiguration setVideoCodecType:AVVideoCodecTypeH264];

	fRecordingOutput = [[SCRecordingOutput alloc]
		initWithConfiguration:recordingConfiguration delegate:self];
	fStream = [[SCStream alloc] initWithFilter:filter configuration:streamConfiguration delegate:self];

	NSError *addOutputError = nil;
	if (![fStream addRecordingOutput:fRecordingOutput error:&addOutputError])
	{
		[self finishWithPhase:@"failed" error:addOutputError removeOutputFile:YES];
		return;
	}

	[fStream startCaptureWithCompletionHandler:
		^(NSError *startError)
		{
			if (startError)
			{
				dispatch_async(dispatch_get_main_queue(),
					^{
						if (generation == fGeneration)
						{
							[self finishWithPhase:@"failed" error:startError removeOutputFile:YES];
						}
					});
			}
		}];
}

- (BOOL)stopRecording:(NSError **)error
{
	if (fState == SimulatorScreenRecorderStateIdle)
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"No Simulator screen recording is active.");
		}
		return NO;
	}
	if (fState == SimulatorScreenRecorderStateStopping)
	{
		return YES;
	}

	fState = SimulatorScreenRecorderStateStopping;
	if (!fStream)
	{
		// The shareable-content request has not finished. Invalidate its result
		// and report a clean, empty recording session.
		fGeneration++;
		[self finishWithPhase:@"ended" error:nil removeOutputFile:YES];
		return YES;
	}

	NSUInteger generation = fGeneration;
	[fStream stopCaptureWithCompletionHandler:
		^(NSError *stopError)
		{
			if (stopError)
			{
				dispatch_async(dispatch_get_main_queue(),
					^{
						if (generation == fGeneration)
						{
							[self finishWithPhase:@"failed" error:stopError removeOutputFile:YES];
						}
					});
			}
		}];
	return YES;
}

- (void)recordingOutputDidStartRecording:(SCRecordingOutput *)recordingOutput
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (recordingOutput == fRecordingOutput && fState == SimulatorScreenRecorderStateStarting)
			{
				fState = SimulatorScreenRecorderStateRecording;
				if ([delegate respondsToSelector:@selector(screenRecorder:didChangePhase:outputURL:error:)])
				{
					[delegate screenRecorder:self didChangePhase:@"started" outputURL:fOutputURL error:nil];
				}
			}
		});
}

- (void)recordingOutput:(SCRecordingOutput *)recordingOutput didFailWithError:(NSError *)error
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (recordingOutput == fRecordingOutput && fState != SimulatorScreenRecorderStateIdle)
			{
				[self finishWithPhase:@"failed" error:error removeOutputFile:YES];
			}
		});
}

- (void)recordingOutputDidFinishRecording:(SCRecordingOutput *)recordingOutput
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (recordingOutput == fRecordingOutput && fState != SimulatorScreenRecorderStateIdle)
			{
				[self finishWithPhase:@"ended" error:nil removeOutputFile:NO];
			}
		});
}

- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (stream == fStream && fState != SimulatorScreenRecorderStateIdle)
			{
				[self finishWithPhase:@"failed" error:error removeOutputFile:YES];
			}
		});
}

- (void)finishWithPhase:(NSString *)phase error:(NSError *)error removeOutputFile:(BOOL)removeOutputFile
{
	NSURL *completedOutputURL = [fOutputURL retain];
	if (error && fStream)
	{
		[fStream stopCaptureWithCompletionHandler:nil];
	}
	if (removeOutputFile && completedOutputURL)
	{
		[[NSFileManager defaultManager] removeItemAtURL:completedOutputURL error:nil];
	}
	fState = SimulatorScreenRecorderStateIdle;
	[self releaseCaptureObjects];

	if ([delegate respondsToSelector:@selector(screenRecorder:didChangePhase:outputURL:error:)])
	{
		[delegate screenRecorder:self didChangePhase:phase outputURL:completedOutputURL error:error];
	}
	[completedOutputURL release];
}

- (void)releaseCaptureObjects
{
	[fStream release];
	fStream = nil;
	[fRecordingOutput release];
	fRecordingOutput = nil;
	[fOutputURL release];
	fOutputURL = nil;
}

@end

#pragma clang diagnostic pop
