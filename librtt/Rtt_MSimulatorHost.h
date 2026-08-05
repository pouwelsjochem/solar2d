//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MSimulatorHost_H__
#define _Rtt_MSimulatorHost_H__

#include <string>
#include <vector>

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class MSimulatorHost
{
	public:
		struct SafeAreaInsets
		{
			SafeAreaInsets()
			:	top( 0 ),
				left( 0 ),
				bottom( 0 ),
				right( 0 )
			{
			}

			int top;
			int left;
			int bottom;
			int right;
		};

		struct Device
		{
			Device()
			:	width( 0 ),
				height( 0 ),
				isCustom( false ),
				isProject( false ),
				hasRoundedCorners( false ),
				roundedCorners( false ),
				isCurrent( false )
			{
			}

			std::string id;
			std::string name;
			std::string category;
			int width;
			int height;
			bool isCustom;
			bool isProject;
			bool hasRoundedCorners;
			bool roundedCorners;
			bool isCurrent;
			SafeAreaInsets safeAreaInsets;
		};

		struct Window
		{
			Window()
			:	x( 0.0 ),
				y( 0.0 ),
				width( 0.0 ),
				height( 0.0 ),
				backingScale( 1.0 ),
				isFullscreen( false )
			{
			}

			double x;
			double y;
			double width;
			double height;
			double backingScale;
			bool isFullscreen;
		};

		struct State
		{
			State()
			:	isSuspended( false ),
				safeAreaGuidesVisible( false ),
				isRelaunchPending( false ),
				relaunchCount( 0 )
			{
			}

			Device device;
			bool isSuspended;
			bool safeAreaGuidesVisible;
			bool isRelaunchPending;
			long relaunchCount;
			Window window;
		};

		struct Configuration
		{
			typedef enum _DeviceSelection
			{
				kKeepCurrentDevice = 0,
				kNamedDevice,
				kCustomDevice
			}
			DeviceSelection;

			Configuration()
			:	deviceSelection( kKeepCurrentDevice ),
				width( 0 ),
				height( 0 ),
				hasRoundedCorners( false ),
				roundedCorners( false ),
				temporary( false )
			{
			}

			DeviceSelection deviceSelection;
			std::string deviceId;
			int width;
			int height;
			SafeAreaInsets safeAreaInsets;
			bool hasRoundedCorners;
			bool roundedCorners;
			bool temporary;
		};

		typedef enum _ConfigureResult
		{
			kConfigureFailed = 0,
			kConfigureApplied,
			kConfigureAlreadyActive
		}
		ConfigureResult;

		struct Input
		{
			typedef enum _Type
			{
				kBackInput = 0,
				kKeyInput,
				kTextInput,
				kTouchInput,
				kMouseInput,
				kControllerInput
			}
			Type;

			typedef enum _ControllerAction
			{
				kNoControllerAction = 0,
				kConnectController,
				kDisconnectController,
				kButtonController,
				kAxisController
			}
			ControllerAction;

			typedef enum _Phase
			{
				kNoPhase = 0,
				kDownPhase,
				kUpPhase,
				kPressedPhase,
				kBeganPhase,
				kMovedPhase,
				kEndedPhase,
				kCancelledPhase,
				kDragPhase,
				kMovePhase,
				kExitPhase,
				kScrollPhase
			}
			Phase;

			Input()
			:	type( kBackInput ),
				controllerAction( kNoControllerAction ),
				phase( kNoPhase ),
				controllerId( "default" ),
				hasControllerProfile( false ),
				hasControllerPlayerNumber( false ),
				controllerPlayerNumber( 0 ),
				hasQwertyKeyName( false ),
				hasNativeKeyCode( false ),
				nativeKeyCode( 0 ),
				x( 0.0 ),
				y( 0.0 ),
				xStart( 0.0 ),
				yStart( 0.0 ),
				scrollX( 0.0 ),
				scrollY( 0.0 ),
				axisValue( 0.0 ),
				clickCount( 0 ),
				isShiftDown( false ),
				isAltDown( false ),
				isCtrlDown( false ),
				isCommandDown( false ),
				isPrimaryButtonDown( false ),
				isSecondaryButtonDown( false ),
				isMiddleButtonDown( false )
			{
			}

			Type type;
			ControllerAction controllerAction;
			Phase phase;
			std::string controllerId;
			std::string controllerProfile;
			bool hasControllerProfile;
			bool hasControllerPlayerNumber;
			int controllerPlayerNumber;
			std::string keyName;
			std::string text;
			std::string axisName;
			bool hasQwertyKeyName;
			std::string qwertyKeyName;
			bool hasNativeKeyCode;
			int nativeKeyCode;
			double x;
			double y;
			double xStart;
			double yStart;
			double scrollX;
			double scrollY;
			double axisValue;
			int clickCount;
			bool isShiftDown;
			bool isAltDown;
			bool isCtrlDown;
			bool isCommandDown;
			bool isPrimaryButtonDown;
			bool isSecondaryButtonDown;
			bool isMiddleButtonDown;
		};

		struct Event
		{
			typedef enum _Type
			{
				kMemoryWarningEvent = 0,
				kBackgroundEvent,
				kAccelerometerEvent,
				kGyroscopeEvent
			}
			Type;

			Event()
			:	type( kMemoryWarningEvent ),
				duration( 0.0 ),
				deltaTime( 0.0 ),
				isShake( false ),
				xGravity( 0.0 ),
				yGravity( 0.0 ),
				zGravity( 0.0 ),
				xInstant( 0.0 ),
				yInstant( 0.0 ),
				zInstant( 0.0 ),
				xRaw( 0.0 ),
				yRaw( 0.0 ),
				zRaw( 0.0 ),
				xRotation( 0.0 ),
				yRotation( 0.0 ),
				zRotation( 0.0 )
			{
			}

			Type type;
			double duration;
			double deltaTime;
			bool isShake;
			double xGravity;
			double yGravity;
			double zGravity;
			double xInstant;
			double yInstant;
			double zInstant;
			double xRaw;
			double yRaw;
			double zRaw;
			double xRotation;
			double yRotation;
			double zRotation;
		};

		struct ScreenRecordingOptions
		{
			ScreenRecordingOptions()
			: framesPerSecond( 60 ),
			  resolutionScale( 1.0 ),
			  includeAudio( true ),
			  showsCursor( false ),
			  overwrite( false )
			{
			}

			std::string path;
			int framesPerSecond;
			double resolutionScale;
			bool includeAudio;
			bool showsCursor;
			bool overwrite;
		};

		typedef enum _ScreenRecordingState
		{
			kScreenRecordingUnavailable = 0,
			kScreenRecordingIdle,
			kScreenRecordingStarting,
			kScreenRecordingRecording,
			kScreenRecordingStopping
		}
		ScreenRecordingState;

	public:
		virtual bool GetCurrentDevice( Device& result ) const = 0;
		virtual bool GetState( State& result ) const = 0;
		virtual bool GetDevices( std::vector< Device >& result ) const = 0;
		virtual ConfigureResult ConfigureAndRelaunch( const Configuration& configuration, bool onlyIfNeeded ) const = 0;
		virtual bool Relaunch() const = 0;
		virtual bool SetSafeAreaGuidesVisible( bool visible ) const = 0;
		virtual bool SetFullscreen( bool fullscreen ) const = 0;
		virtual bool SendInput( const Input& input ) const = 0;
		virtual bool Simulate( const Event& event ) const = 0;
		virtual bool Quit( int exitCode ) const = 0;
		virtual bool StartScreenRecording( const ScreenRecordingOptions& options, std::string& error ) const
		{
			(void)options;
			error = "screen recording is not supported by this Simulator";
			return false;
		}
		virtual bool StopScreenRecording( std::string& error ) const
		{
			error = "screen recording is not supported by this Simulator";
			return false;
		}
		virtual ScreenRecordingState GetScreenRecordingState() const
		{
			return kScreenRecordingUnavailable;
		}
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MSimulatorHost_H__
