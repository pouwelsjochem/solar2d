//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_PlatformSimulator_H__
#define _Rtt_PlatformSimulator_H__

#include "Rtt_MPlatform.h"
#include "Rtt_MPlatformDevice.h"
#include "Rtt_String.h"
#include "Rtt_TargetDevice.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class MPlatform;
class PlatformPlayer;

// ----------------------------------------------------------------------------

struct SimulatorOptions
{
	bool connectToDebugger;
	bool isInteractive;
};

// Base class
// 
// A PlatformSimulator instance owns a platform-specific Player instance.
// 
// Each platform should have its own subclass.
// 
// The MPlatform is owned by this class --- it must outlive the player instance
// b/c the player instance owns the Runtime.
class PlatformSimulator
{
	Rtt_CLASS_NO_COPIES( PlatformSimulator )

	public:
		typedef PlatformSimulator Self;

		typedef void(*PlatformFinalizer)(const MPlatform*);

	public:
		template< typename T >
		static void Finalizer( const MPlatform* p )
		{
			delete static_cast< T* >( const_cast< MPlatform* >( p ) );
		}

	public:
		typedef enum _PropertyMask
		{
			kUninitializedMask = 0x0,

			kIsAppStartedMask = 0x0001,

			// Event Masks
			kAccelerometerEventMask = 0x0100,
			kMultitouchEventMask		= 0x0200,
			kGyroscopeEventMask		= 0x0400,
			kMouseEventMask	= 0x0800,
		}
		PropertyMask;

	public:
		struct Config
		{
			Config( Rtt_Allocator & allocator );
			~Config();

            bool configLoaded;
			TargetDevice::Platform platform;
			float deviceWidth;
			float deviceHeight;
			float safeScreenInsetTop;
			float safeScreenInsetLeft;
			float safeScreenInsetBottom;
			float safeScreenInsetRight;
			float safeLandscapeScreenInsetTop;
			float safeLandscapeScreenInsetLeft;
			float safeLandscapeScreenInsetBottom;
			float safeLandscapeScreenInsetRight;
			String osName;
			String displayManufacturer;
			String displayName;
			bool supportsExitRequests;
			bool supportsInputDevices;
			bool supportsKeyEvents;
			bool supportsKeyEventsFromKeyboard;
			bool supportsBackKey;
			bool supportsMouse;
			bool supportsMultipleAlerts;
			bool isAlertButtonOrderRightToLeft;
			bool hasAccelerometer;
			bool hasGyroscope;
			String windowTitleBarName;
			float defaultFontSize;
		};

	public:
		PlatformSimulator( PlatformFinalizer finalizer );
		virtual ~PlatformSimulator();

	public:
		virtual const char *GetPlatformName() const;
		virtual const char *GetPlatform() const;

	protected:
		void Initialize( MPlatform* platform, MCallback *viewCallback = NULL );

		// build.settings
		void LoadBuildSettings( const MPlatform& platform );

	public:
		static const char kPlatformKeyNameIPhone[];
		static const char kPlatformKeyNameAndroid[];
		static const char kPlatformKeyNameMac[];
	
		// Device configuration --- not to be confused with config.lua
		static void LoadConfig(const char deviceConfigFile[], Config& rConfig);
//TODO: Deprecate this function. MPlatform object is never used here and it is expensive to create one when not needed.
		static void LoadConfig(const char deviceConfigFile[], const MPlatform& platform, Config& rConfig);
		static void ValidateSettings( const MPlatform& platform );

//		virtual PlatformStringRef CreatePlatformString( const char* src ) const = 0;
//		virtual void ReleasePlatformString( PlatformStringRef str ) const = 0;


	public:
		Rtt_INLINE bool IsProperty( U32 mask ) const { return (fProperties & mask) != 0; }
		Rtt_INLINE void ToggleProperty( U32 mask ) { fProperties ^= mask; }
		void SetProperty( U32 mask, bool value );

	public:
		void Start( const SimulatorOptions& options );

	public:
		void BeginNotifications( MPlatformDevice::EventType type ) const;
		void EndNotifications( MPlatformDevice::EventType type ) const;

		virtual const char *GetOSName() const { return "simulator"; }

	public:
		const PlatformPlayer* GetPlayer() const { return fPlayer; }
		PlatformPlayer* GetPlayer() { return fPlayer; }

	public:
		virtual void WillSuspend();
		virtual void DidSuspend();
		virtual void WillResume();
		virtual void DidResume();
		void ToggleSuspendResume(bool sendApplicationEvents = true);

	private:
		void DidStart(); // Called first time DidResume() is invoked
		void WillExit(); // Called by the dtor

	private:
		MPlatform* fPlatform; // Platform must outlive Player
		PlatformPlayer* fPlayer;
		PlatformFinalizer fPlatformFinalizer;
		mutable U32 fProperties;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_PlatformSimulator_H__
