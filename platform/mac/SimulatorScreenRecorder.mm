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
	SimulatorScreenRecorderErrorCleanupTimedOut
};

static const int64_t kCleanupTimeout = 5 * NSEC_PER_SEC;
static const int64_t kForcedCleanupTimeout = 3 * NSEC_PER_SEC;

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
	NSError *fTerminalError;
	SimulatorScreenRecorderState fState;
	NSUInteger fGeneration;
	BOOL fRecordingOutputIsAttached;
	BOOL fRecordingOutputFinished;
	BOOL fStreamStopRequested;
	BOOL fStreamStopped;
	BOOL fRemoveOutputFileWhenFinished;

	CGWindowID fWindowID;
	CGRect fWindowSourceRect;
	NSInteger fFramesPerSecond;
	double fResolutionScale;
	SimulatorScreenRecorderCaptureResolutionType fCaptureResolutionType;
	NSInteger fOutputWidth;
	NSInteger fOutputHeight;
	BOOL fIncludeAudio;
	BOOL fShowsCursor;
}

- (void)beginRecordingWithShareableContent:(SCShareableContent *)shareableContent
	error:(NSError *)error
	generation:(NSUInteger)generation;
- (void)beginStoppingWithError:(NSError *)error removeOutputFile:(BOOL)removeOutputFile;
- (void)requestStreamStop;
- (void)finishIfCaptureObjectsStopped;
- (void)scheduleCleanupTimeout;
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
		if (fRecordingOutput && fRecordingOutputIsAttached)
		{
			[fStream removeRecordingOutput:fRecordingOutput error:nil];
		}
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
	captureResolutionType:(SimulatorScreenRecorderCaptureResolutionType)captureResolutionType
	outputWidth:(NSInteger)outputWidth
	outputHeight:(NSInteger)outputHeight
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
	if ((0 == outputWidth) != (0 == outputHeight) ||
		(0 != outputWidth &&
			(outputWidth < 2 || outputWidth > 16384 || outputHeight < 2 || outputHeight > 16384 ||
			0 != outputWidth % 2 || 0 != outputHeight % 2)))
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument,
				@"Recording output width and height must both be even integers from 2 through 16384.");
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

	NSRect viewRectInWindow = [sourceView convertRect:[sourceView bounds] toView:nil];
	NSRect viewRectOnScreen = [window convertRectToScreen:viewRectInWindow];
	if (NSIsEmptyRect(viewRectOnScreen))
	{
		if (error)
		{
			*error = NewRecorderError(
				SimulatorScreenRecorderErrorInvalidArgument, @"The Simulator device view has no capturable area.");
		}
		return NO;
	}

	// AppKit screen coordinates have a bottom-left origin. ScreenCaptureKit's
	// window source rectangle has a top-left origin and is window-relative.
	NSRect windowFrame = [window frame];
	fWindowSourceRect = CGRectMake(
		NSMinX(viewRectOnScreen) - NSMinX(windowFrame),
		NSMaxY(windowFrame) - NSMaxY(viewRectOnScreen),
		NSWidth(viewRectOnScreen),
		NSHeight(viewRectOnScreen));
	fWindowID = (CGWindowID)[window windowNumber];
	fFramesPerSecond = framesPerSecond;
	fResolutionScale = resolutionScale;
	fCaptureResolutionType = captureResolutionType;
	fOutputWidth = outputWidth;
	fOutputHeight = outputHeight;
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

	SCContentFilter *filter = [[[SCContentFilter alloc]
		initWithDesktopIndependentWindow:capturedWindow] autorelease];
	// fWindowSourceRect is measured in window points. Use ScreenCaptureKit's
	// scale so transformed Simulator views retain the captured window's native pixels.
	CGFloat pointPixelScale = MAX(1.0, [filter pointPixelScale]);
	size_t outputWidth = fOutputWidth > 0 ? (size_t)fOutputWidth :
		EvenPixelDimension(CGRectGetWidth(fWindowSourceRect) * pointPixelScale * fResolutionScale);
	size_t outputHeight = fOutputHeight > 0 ? (size_t)fOutputHeight :
		EvenPixelDimension(CGRectGetHeight(fWindowSourceRect) * pointPixelScale * fResolutionScale);
	SCStreamConfiguration *streamConfiguration = [[[SCStreamConfiguration alloc] init] autorelease];
	[streamConfiguration setSourceRect:fWindowSourceRect];
	[streamConfiguration setDestinationRect:CGRectMake(0, 0, outputWidth, outputHeight)];
	[streamConfiguration setWidth:outputWidth];
	[streamConfiguration setHeight:outputHeight];
	[streamConfiguration setScalesToFit:YES];
	[streamConfiguration setPreservesAspectRatio:YES];
	// Allow display-synchronized frames to arrive slightly inside the exact
	// frame-rate boundary instead of having ScreenCaptureKit discard them.
	CMTime frameInterval = CMTimeMake(1, (int32_t)fFramesPerSecond);
	[streamConfiguration setMinimumFrameInterval:CMTimeMultiplyByFloat64(frameInterval, 0.9)];
	[streamConfiguration setQueueDepth:5];
	[streamConfiguration setPixelFormat:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange];
	[streamConfiguration setShowsCursor:fShowsCursor];
	[streamConfiguration setShowMouseClicks:NO];
	[streamConfiguration setIgnoreShadowsSingleWindow:YES];
	[streamConfiguration setShouldBeOpaque:YES];
	SCCaptureResolutionType captureResolutionType = SCCaptureResolutionAutomatic;
	if (SimulatorScreenRecorderCaptureResolutionBest == fCaptureResolutionType)
	{
		captureResolutionType = SCCaptureResolutionBest;
	}
	else if (SimulatorScreenRecorderCaptureResolutionNominal == fCaptureResolutionType)
	{
		captureResolutionType = SCCaptureResolutionNominal;
	}
	[streamConfiguration setCaptureResolution:captureResolutionType];
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
	fRecordingOutputIsAttached = YES;
	fRecordingOutputFinished = NO;
	fStreamStopRequested = NO;
	fStreamStopped = NO;

	[fStream startCaptureWithCompletionHandler:
		^(NSError *startError)
		{
			if (startError)
			{
				dispatch_async(dispatch_get_main_queue(),
					^{
						if (generation == fGeneration)
						{
							[self beginStoppingWithError:startError removeOutputFile:YES];
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

	if (!fStream)
	{
		// The shareable-content request has not finished. Invalidate its result
		// and report a clean, empty recording session.
		fGeneration++;
		[self finishWithPhase:@"ended" error:nil removeOutputFile:YES];
		return YES;
	}

	BOOL removeOutputFile = fState == SimulatorScreenRecorderStateStarting;
	[self beginStoppingWithError:nil removeOutputFile:removeOutputFile];
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
				fRecordingOutputFinished = YES;
				[self beginStoppingWithError:error removeOutputFile:YES];
			}
		});
}

- (void)recordingOutputDidFinishRecording:(SCRecordingOutput *)recordingOutput
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (recordingOutput == fRecordingOutput && fState != SimulatorScreenRecorderStateIdle)
			{
				fRecordingOutputFinished = YES;
				if (fState == SimulatorScreenRecorderStateStopping)
				{
					[self requestStreamStop];
					[self finishIfCaptureObjectsStopped];
				}
				else
				{
					[self beginStoppingWithError:NewRecorderError(
						SimulatorScreenRecorderErrorUnavailable,
						@"ScreenCaptureKit ended the recording unexpectedly.")
						removeOutputFile:YES];
				}
			}
		});
}

- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (stream == fStream && fState != SimulatorScreenRecorderStateIdle)
			{
				fStreamStopped = YES;
				NSError *streamError = error ?: NewRecorderError(
					SimulatorScreenRecorderErrorUnavailable,
					@"ScreenCaptureKit stopped the capture stream unexpectedly.");
				[self beginStoppingWithError:streamError removeOutputFile:YES];
			}
		});
}

- (void)beginStoppingWithError:(NSError *)error removeOutputFile:(BOOL)removeOutputFile
{
	if (fState == SimulatorScreenRecorderStateIdle)
	{
		return;
	}
	if (error && !fTerminalError)
	{
		fTerminalError = [error retain];
	}
	if (removeOutputFile)
	{
		fRemoveOutputFileWhenFinished = YES;
	}

	BOOL wasAlreadyStopping = fState == SimulatorScreenRecorderStateStopping;
	fState = SimulatorScreenRecorderStateStopping;
	if (!wasAlreadyStopping)
	{
		if (!fStream)
		{
			fStreamStopped = YES;
		}
		if (!fRecordingOutput)
		{
			fRecordingOutputFinished = YES;
			fRecordingOutputIsAttached = NO;
		}
		else if (fRecordingOutputIsAttached)
		{
			NSError *removeError = nil;
			if ([fStream removeRecordingOutput:fRecordingOutput error:&removeError])
			{
				fRecordingOutputIsAttached = NO;
			}
			else
			{
				if (!fTerminalError)
				{
					fTerminalError = [removeError retain];
				}
				[self requestStreamStop];
			}
		}
		[self scheduleCleanupTimeout];
	}

	if (fRecordingOutputFinished)
	{
		[self requestStreamStop];
	}
	[self finishIfCaptureObjectsStopped];
}

- (void)requestStreamStop
{
	if (fStreamStopped || fStreamStopRequested)
	{
		return;
	}
	if (!fStream)
	{
		fStreamStopped = YES;
		[self finishIfCaptureObjectsStopped];
		return;
	}

	fStreamStopRequested = YES;
	NSUInteger generation = fGeneration;
	[fStream stopCaptureWithCompletionHandler:
		^(NSError *stopError)
		{
			dispatch_async(dispatch_get_main_queue(),
				^{
					if (generation == fGeneration && fState == SimulatorScreenRecorderStateStopping)
					{
						fStreamStopped = YES;
						if (stopError && !fTerminalError)
						{
							fTerminalError = [stopError retain];
							fRemoveOutputFileWhenFinished = YES;
						}
						[self finishIfCaptureObjectsStopped];
					}
				});
		}];
}

- (void)finishIfCaptureObjectsStopped
{
	if (fState == SimulatorScreenRecorderStateStopping &&
		fRecordingOutputFinished && fStreamStopped)
	{
		NSString *phase = fTerminalError ? @"failed" : @"ended";
		[self finishWithPhase:phase error:fTerminalError removeOutputFile:fRemoveOutputFileWhenFinished];
	}
}

- (void)scheduleCleanupTimeout
{
	NSUInteger generation = fGeneration;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, kCleanupTimeout), dispatch_get_main_queue(),
		^{
			if (generation != fGeneration || fState != SimulatorScreenRecorderStateStopping)
			{
				return;
			}
			if (!fTerminalError)
			{
				fTerminalError = [NewRecorderError(
					SimulatorScreenRecorderErrorCleanupTimedOut,
					@"ScreenCaptureKit timed out while finalizing the recording.") retain];
			}
			fRemoveOutputFileWhenFinished = YES;
			[self requestStreamStop];

			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, kForcedCleanupTimeout), dispatch_get_main_queue(),
				^{
					if (generation == fGeneration && fState == SimulatorScreenRecorderStateStopping)
					{
						fRecordingOutputFinished = YES;
						fStreamStopped = YES;
						[self finishIfCaptureObjectsStopped];
					}
				});
		});
}

- (void)finishWithPhase:(NSString *)phase error:(NSError *)error removeOutputFile:(BOOL)removeOutputFile
{
	NSURL *completedOutputURL = [fOutputURL retain];
	NSError *completedError = [error retain];
	fState = SimulatorScreenRecorderStateIdle;
	[self releaseCaptureObjects];
	if (removeOutputFile && completedOutputURL)
	{
		[[NSFileManager defaultManager] removeItemAtURL:completedOutputURL error:nil];
	}

	if ([delegate respondsToSelector:@selector(screenRecorder:didChangePhase:outputURL:error:)])
	{
		[delegate screenRecorder:self didChangePhase:phase outputURL:completedOutputURL error:completedError];
	}
	[completedError release];
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
	[fTerminalError release];
	fTerminalError = nil;
	fRecordingOutputIsAttached = NO;
	fRecordingOutputFinished = NO;
	fStreamStopRequested = NO;
	fStreamStopped = NO;
	fRemoveOutputFileWhenFinished = NO;
}

@end

#pragma clang diagnostic pop
