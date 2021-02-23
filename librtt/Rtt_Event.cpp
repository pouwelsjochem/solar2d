//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_Event.h"

#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_Display.h"
#include "Display/Rtt_DisplayObject.h"
#include "Display/Rtt_StageObject.h"
#include "Input/Rtt_PlatformInputAxis.h"
#include "Input/Rtt_PlatformInputDevice.h"
#include "Rtt_Lua.h"
#include "Display/Rtt_BitmapMask.h"
#include "Rtt_HitTestObject.h"
#include "Rtt_LuaContext.h"
#include "Rtt_LuaProxy.h"
#include "Rtt_LuaResource.h"
#include "Rtt_Runtime.h"
#include "Display/Rtt_SpriteObject.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

const char MEvent::kNameKey[] = "name";
const char MEvent::kProviderKey[] = "provider";
const char MEvent::kPhaseKey[] = "phase";
const char MEvent::kTypeKey[] = "type";
const char MEvent::kResponseKey[] = "response";
const char MEvent::kIsErrorKey[] = "isError";
const char MEvent::kErrorCodeKey[] = "errorCode";
const char MEvent::kDataKey[] = "data";

// Deprecated: We should favor kResponseKey in favor of this
// since we can use kIsErrorKey to tell us how to interpret the response
static const char kErrorMessageKey[] = "errorMessage";

// ----------------------------------------------------------------------------

VirtualEvent::~VirtualEvent()
{
}

int
VirtualEvent::PrepareDispatch( lua_State *L ) const
{
	Lua::PushRuntime( L );
	lua_getfield( L, -1, "dispatchEvent" );
	lua_insert( L, -2 ); // swap stack positions for "Runtime" and "dispatchEvent"
	return 1 + Push( L );
}

void
VirtualEvent::Dispatch( lua_State *L, Runtime& ) const
{
	// Invoke Lua code: "Runtime:dispatchEvent( eventKey )"
	int nargs = PrepareDispatch( L );
	LuaContext::DoCall( L, nargs, 0 );
}

int
VirtualEvent::Push( lua_State *L ) const
{
	Lua::NewEvent( L, Name() ); Rtt_ASSERT( lua_istable( L, -1 ) );
	return 1;
}

// ----------------------------------------------------------------------------

ErrorEvent::ErrorEvent()
:	fErrorMsg( NULL ),
	fErrorCode( 0 )
{
}

int
ErrorEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		if ( fErrorMsg )
		{
			lua_pushstring( L, fErrorMsg );
			lua_setfield( L, -2, kErrorMessageKey  );

			lua_pushinteger( L, fErrorCode );
			lua_setfield( L, -2, kErrorCodeKey );
		}
	}
	return 1;
}

void
ErrorEvent::SetError( const char *errorMsg, int errorCode )
{
	fErrorMsg = errorMsg;
	fErrorCode = errorCode;
}

// ----------------------------------------------------------------------------

const FrameEvent&
FrameEvent::Constant()
{
	static const FrameEvent kEvent;
	return kEvent;
}

FrameEvent::FrameEvent()
{
}

const char*
FrameEvent::Name() const
{
	static const char kName[] = "enterFrame";
	return kName;
}

int
FrameEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Runtime *runtime = LuaContext::GetRuntime( L );
		lua_pushnumber( L, runtime->GetFrame() );
		lua_setfield( L, -2, "frame" );
		lua_pushnumber( L, runtime->GetElapsedMS() );
		lua_setfield( L, -2, "time" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const RenderEvent&
RenderEvent::Constant()
{
	static const RenderEvent kEvent;
	return kEvent;
}

RenderEvent::RenderEvent()
{
}

const char*
RenderEvent::Name() const
{
	static const char kName[] = "lateUpdate";
	return kName;
}

int
RenderEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Runtime *runtime = LuaContext::GetRuntime( L );
		lua_pushnumber( L, runtime->GetFrame() );
		lua_setfield( L, -2, "frame" );
		lua_pushnumber( L, runtime->GetElapsedMS() );
		lua_setfield( L, -2, "time" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char*
SystemEvent::StringForType( Type type )
{
	const char* result = NULL;
	static const char kOnAppExitString[] = "applicationExit";
	static const char kOnAppStartString[] = "applicationStart";
	static const char kOnAppSuspendString[] = "applicationSuspend";
	static const char kOnAppResumeString[] = "applicationResume";
	static const char kOnAppOpenString[] = "applicationOpen";

	switch( type )
	{
		case kOnAppExit:
			result = kOnAppExitString;
			break;
		case kOnAppStart:
			result = kOnAppStartString;
			break;
		case kOnAppSuspend:
			result = kOnAppSuspendString;
			break;
		case kOnAppResume:
			result = kOnAppResumeString;
			break;
		case kOnAppOpen:
			result = kOnAppOpenString;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

SystemEvent::SystemEvent( Type type )
:	fType( type )
{
}

const char*
SystemEvent::Name() const
{
	static const char kName[] = "system";
	return kName;
}

int
SystemEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		const char* value = StringForType( (Type)fType );
		if ( value )
		{
			lua_pushstring( L, value );
			lua_setfield( L, -2, kTypeKey );
		}
	}

	return 1;
}
	
// ----------------------------------------------------------------------------
	
SystemOpenEvent::SystemOpenEvent( const char *url )
:	Super( kOnAppOpen ),
	fUrl( url ),
	fCommandLineDirectoryPath( NULL ),
	fCommandLineArgumentArray( NULL ),
	fCommandLineArgumentCount( 0 )
{
}

void
SystemOpenEvent::SetCommandLineArgs(int argumentCount, const char **arguments)
{
	fCommandLineArgumentCount = argumentCount;
	fCommandLineArgumentArray = arguments;
}

void
SystemOpenEvent::SetCommandLineDir(const char *directoryPath)
{
	fCommandLineDirectoryPath = directoryPath;
}

int
SystemOpenEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		if ( fUrl )
		{
			lua_pushstring( L, fUrl );
			lua_setfield( L, -2, "url" );
		}

		lua_createtable( L, fCommandLineArgumentCount > 0 ? fCommandLineArgumentCount : 0, 0 );
		if ( fCommandLineArgumentArray && ( fCommandLineArgumentCount > 0 ) )
		{
			for ( int index = 0; index < fCommandLineArgumentCount; index++ )
			{
				const char* stringPointer = fCommandLineArgumentArray[ index ];
				lua_pushstring( L, stringPointer ? stringPointer : "" );
				lua_rawseti( L, -2, index + 1 );
			}
		}
		lua_setfield( L, -2, "commandLineArgs" );

		if ( fCommandLineDirectoryPath )
		{
			lua_pushstring( L, fCommandLineDirectoryPath );
			lua_setfield( L, -2, "commandLineDir" );
		}
	}

	return 1;
}

// ----------------------------------------------------------------------------

/// Creates a new event indicating that the main app window has been resized.
ResizeEvent::ResizeEvent()
{
}

const char*
ResizeEvent::Name() const
{
	static const char kName[] = "resize";
	return kName;
}

int
ResizeEvent::Push( lua_State *L ) const
{
	return Super::Push( L );
}

// ----------------------------------------------------------------------------

/// Creates a new event data object that stores a single accelerometer measurement.
/// @param gravity Pointer to an array of 3 elements storing smoothed acceleration data.
/// @param instant Pointer to an array of 3 elements storing acceleration deltas based on smoother/gravity data.
/// @param raw Pointer to an array of 3 elements storing raw acceleration data.
/// @param isShake Set true if the device was shaken.
/// @param deltaTime Number of seconds since the last measurement.
AccelerometerEvent::AccelerometerEvent(
	double* gravity, double* instant, double* raw, bool isShake, double deltaTime, PlatformInputDevice *device )
:	fGravityAccel( gravity ),
	fInstantAccel( instant ),
	fRawAccel( raw ),
	fIsShake( isShake ),
	fDeltaTime( deltaTime ),
	fDevice( device )
{
}

const char*
AccelerometerEvent::Name() const
{
	static const char kName[] = "accelerometer";
	return kName;
}

static void
PushAccelerationComponents( lua_State *L, double* accel, const char* labels[] )
{
	lua_pushnumber( L, accel[0] );
	lua_setfield( L, -2, labels[0] );
	lua_pushnumber( L, accel[1] );
	lua_setfield( L, -2, labels[1] );
	lua_pushnumber( L, accel[2] );
	lua_setfield( L, -2, labels[2] );
}

int
AccelerometerEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		const char *gravityLabels[] = { "xGravity", "yGravity", "zGravity" };
		PushAccelerationComponents( L, fGravityAccel, gravityLabels );

		const char *instantLabels[] = { "xInstant", "yInstant", "zInstant" };
		PushAccelerationComponents( L, fInstantAccel, instantLabels );

		const char *rawLabels[] = { "xRaw", "yRaw", "zRaw" };
		PushAccelerationComponents( L, fRawAccel, rawLabels );

		lua_pushboolean( L, fIsShake );
		lua_setfield( L, -2, "isShake" );
		
		lua_pushnumber( L, fDeltaTime );
		lua_setfield( L, -2, "deltaTime" );
		
		if (fDevice)
		{
			fDevice->PushTo( L );
			lua_setfield( L, -2, "device" );
		}

	}

	return 1;
}

// ----------------------------------------------------------------------------

/// Creates a new event data object that stores a single gyroscope measurement.
/// @param xRotation Rate of rotation around the x-axis in radians per second.
/// @param yRotation Rate of rotation around the y-axis in radians per second.
/// @param zRotation Rate of rotation around the z-axis in radians per second.
/// @param deltaTime Number of seconds since the last measurement.
GyroscopeEvent::GyroscopeEvent( double xRotation, double yRotation, double zRotation, double deltaTime, PlatformInputDevice *device )
:	fXRotation( xRotation ),
	fYRotation( yRotation ),
	fZRotation( zRotation ),
	fDeltaTime( deltaTime ),
	fDevice(device)
{
}

const char*
GyroscopeEvent::Name() const
{
	static const char kName[] = "gyroscope";
	return kName;
}

int
GyroscopeEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushnumber( L, fXRotation );
		lua_setfield( L, -2, "xRotation" );
		lua_pushnumber( L, fYRotation );
		lua_setfield( L, -2, "yRotation" );
		lua_pushnumber( L, fZRotation );
		lua_setfield( L, -2, "zRotation" );
		lua_pushnumber( L, fDeltaTime );
		lua_setfield( L, -2, "deltaTime" );
		if (fDevice)
		{
			fDevice->PushTo( L );
			lua_setfield( L, -2, "device" );
		}
	}
	
	return 1;
}

// ----------------------------------------------------------------------------

MemoryWarningEvent::MemoryWarningEvent( )
{
}

const char*
MemoryWarningEvent::Name() const
{
	static const char kName[] = "memoryWarning";
	return kName;
}

int
MemoryWarningEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		// No fields currently in memory warning events	
	}
	return 1;
}

// ----------------------------------------------------------------------------

const char*
NotificationEvent::StringForType( Type type )
{
	static const char kLocalString[] = "local";
	static const char kRemoteString[] = "remote";
	static const char kRemoteRegistrationString[] = "remoteRegistration";

	const char *result = kLocalString;

	switch ( type )
	{
		case kRemote:
			result = kRemoteString;
			break;
		case kRemoteRegistration:
			result = kRemoteRegistrationString;
		default:
			break;
	}

	return result;
}

const char*
NotificationEvent::StringForApplicationState( ApplicationState state )
{
	static const char kBackgroundString[] = "background";
	static const char kActiveString[] = "active";
	static const char kInactiveString[] = "inactive";

	const char *result = kBackgroundString;
	switch ( state )
	{
		case kActive:
			result = kActiveString;
			break;
		case kInactive:
			result = kInactiveString;
			break;
		default:
			break;
	}

	return result;
}

NotificationEvent::NotificationEvent( Type t, ApplicationState state )
:	fType( t ),
	fApplicationState( state )
{
}

const char*
NotificationEvent::Name() const
{
	static const char kName[] = "notification";
	return kName;
}

int
NotificationEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushstring( L, StringForType( (Type)fType ) );
		lua_setfield( L, -2, kTypeKey );

		lua_pushstring( L, StringForApplicationState( (ApplicationState)fApplicationState ) );
		lua_setfield( L, -2, "applicationState" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

InputDeviceStatusEvent::InputDeviceStatusEvent(
	PlatformInputDevice *device, bool hasConnectionStateChanged, bool wasReconfigured)
:	fDevice( device ),
	fHasConnectionStateChanged( hasConnectionStateChanged ),
	fWasReconfigured( wasReconfigured )
{
}

const char*
InputDeviceStatusEvent::Name() const
{
	static const char kName[] = "inputDeviceStatus";
	return kName;
}

int
InputDeviceStatusEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		if (fDevice)
		{
			fDevice->PushTo( L );
			lua_setfield( L, -2, "device" );
		}

		lua_pushboolean( L, fHasConnectionStateChanged );
		lua_setfield( L, -2, "connectionStateChanged" );

		lua_pushboolean( L, fWasReconfigured );
		lua_setfield( L, -2, "reconfigured" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char*
KeyEvent::StringForPhase( Phase type )
{
	const char* result = NULL;
	static const char kUnknownString[] = "unknown";
	static const char kDownString[] = "down";
	static const char kUpString[] = "up";

	switch( type )
	{
		case kDown:
			result = kDownString;
			break;
		case kUp:
			result = kUpString;
			break;
		default:
			result = kUnknownString;
			break;
	}

	return result;
}

KeyEvent::KeyEvent(
	PlatformInputDevice *device, Phase phase, const char *keyName, S32 nativeKeyCode,
	bool isShiftDown, bool isAltDown, bool isCtrlDown, bool isCommandDown)
:	fDevice( device ),
	fPhase( phase ),
	fKeyName( keyName ),
	fNativeKeyCode( nativeKeyCode ),
	fIsShiftDown( isShiftDown ),
	fIsAltDown( isAltDown ),
	fIsCtrlDown( isCtrlDown ),
    fIsCommandDown( isCommandDown )
{
}

const char*
KeyEvent::Name() const
{
	static const char kName[] = "key";
	return kName;
}

int
KeyEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		if (fDevice)
		{
			fDevice->PushTo( L );
			lua_setfield( L, -2, "device" );
		}

		if (fDevice)
		{
			lua_pushstring( L, fDevice->GetDescriptor().GetInvariantName() );
			lua_pushstring( L, ": " );
			lua_pushstring( L, fKeyName );
			lua_concat( L, 3 );
		}
		else
		{
			lua_pushstring( L, fKeyName );
		}
		lua_setfield( L, -2, "descriptor" );

		const char* value = StringForPhase( (Phase)fPhase );
		lua_pushstring( L, value );
		lua_setfield( L, -2, kPhaseKey );

		lua_pushstring( L, fKeyName );
		lua_setfield( L, -2, "keyName" );

		lua_pushinteger( L, fNativeKeyCode );
		lua_setfield( L, -2, "nativeKeyCode" );

		lua_pushboolean( L, fIsShiftDown );
		lua_setfield( L, -2, "isShiftDown" );

		lua_pushboolean( L, fIsAltDown );
		lua_setfield( L, -2, "isAltDown" );
        
		lua_pushboolean( L, fIsCtrlDown );
		lua_setfield( L, -2, "isCtrlDown" );
        
		lua_pushboolean( L, fIsCommandDown );
		lua_setfield( L, -2, "isCommandDown" );
	}

	return 1;
}

void
KeyEvent::Dispatch( lua_State *L, Runtime& ) const
{
	// Invoke Lua code: "Runtime:dispatchEvent( eventKey )"
	int nargs = PrepareDispatch( L );
	LuaContext::DoCall( L, nargs, 1 );

	fResult = lua_toboolean( L, -1 ); // fetch result 
	lua_pop( L, 1 ); // pop result off stack
}

// ----------------------------------------------------------------------------

CharacterEvent::CharacterEvent(PlatformInputDevice *device, const char *character)
:	fDevice( device ),
fCharacter( character )
{
}

const char*
CharacterEvent::Name() const
{
	static const char kName[] = "character";
	return kName;
}

int
CharacterEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		if (fDevice)
		{
			fDevice->PushTo( L );
			lua_setfield( L, -2, "device" );
		}
		
		lua_pushstring( L, fCharacter );
		lua_setfield( L, -2, "character" );
	}
	
	return 1;
}

void
CharacterEvent::Dispatch( lua_State *L, Runtime& ) const
{
	// Invoke Lua code: "Runtime:dispatchEvent( eventKey )"
	int nargs = PrepareDispatch( L );
	LuaContext::DoCall( L, nargs, 1 );
	
	fResult = lua_toboolean( L, -1 ); // fetch result
	lua_pop( L, 1 ); // pop result off stack
}

// ----------------------------------------------------------------------------

AxisEvent::AxisEvent(PlatformInputDevice *devicePointer, PlatformInputAxis *axisPointer, Rtt_Real rawValue)
:	fDevicePointer( devicePointer ),
	fAxisPointer( axisPointer ),
	fRawValue( rawValue )
{
}

const char*
AxisEvent::Name() const
{
	static const char kName[] = "axis";
	return kName;
}

int
AxisEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		// Push the device userdata to the Lua event table.
		if (fDevicePointer)
		{
			fDevicePointer->PushTo( L );
			lua_setfield( L, -2, "device" );
		}

		// Push the axis information table to the Lua event table.
		if (fAxisPointer)
		{
			fAxisPointer->PushTo( L );
			lua_setfield( L, -2, "axis" );
		}

		// Push the raw axis value to the Lua event table.
		lua_pushnumber( L, Rtt_RealToFloat( fRawValue ) );
		lua_setfield( L, -2, "rawValue" );

		// Push a normalized axis value to the Lua event table.
		if (fAxisPointer)
		{
			Rtt_Real normalizedValue = fAxisPointer->GetNormalizedValue(fRawValue);
			lua_pushnumber( L, Rtt_RealToFloat( normalizedValue ) );
			lua_setfield( L, -2, "normalizedValue" );
		}
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char ColorSampleEvent::kName[] = "colorSample";

ColorSampleEvent::ColorSampleEvent( float x, float y, RGBA &color )
: fX( x )
, fY( y )
, fColor( color )
{
}

const char*
ColorSampleEvent::Name() const
{
	return Self::kName;
}

int
ColorSampleEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushnumber( L, fX );
		lua_setfield( L, -2, "x" );

		lua_pushnumber( L, fY );
		lua_setfield( L, -2, "y" );

		lua_pushnumber( L, ( (float)fColor.r / 255.0f ) );
		lua_setfield( L, -2, "r" );

		lua_pushnumber( L, ( (float)fColor.g / 255.0f ) );
		lua_setfield( L, -2, "g" );

		lua_pushnumber( L, ( (float)fColor.b / 255.0f ) );
		lua_setfield( L, -2, "b" );

		lua_pushnumber( L, ( (float)fColor.a / 255.0f ) );
		lua_setfield( L, -2, "a" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

HitEvent::HitEvent( Real xScreen, Real yScreen )
:	fXContent( xScreen ),
	fYContent( yScreen ),
	fXScreen( xScreen ),
	fYScreen( yScreen ),
	fTime( -1. ),
	fId( NULL )
{
}

const char*
HitEvent::Name() const
{
	static const char kName[] = "touch";
	return kName;
}

int
HitEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		const char kXKey[] = "x";
		const char kYKey[] = "y";

		lua_pushnumber( L, Rtt_RealToFloat( fXContent ) );
		lua_setfield( L, -2, kXKey );
		lua_pushnumber( L, Rtt_RealToFloat( fYContent ) );
		lua_setfield( L, -2, kYKey );

		if ( fTime < 0. )
		{
			fTime = LuaContext::GetRuntime( L )->GetElapsedMS();
		}

		lua_pushnumber( L, fTime );
		lua_setfield( L, -2, "time" );

//		fHitTarget->GetProxy( L )->PushTable( L );
//		lua_setfield( L, -2, "target" );
	}

	return 1;
}

void
HitEvent::InvalidateTime()
{
	fTime = -1.;
}

enum HitTestProperties
{
	kHitTestHandled = 0x1,
	kHitTestDidHit = 0x2
};

static bool
TestMask( Rtt_Allocator *allocator, DisplayObject& child, const Matrix& srcToDstSpace, Real dstX, Real dstY )
{
	Rtt_ASSERT( allocator );

	bool result = false;

	const BitmapMask *mask = child.GetMask();
	Rtt_ASSERT( NULL != mask );

	// Map dstX, dstY to src (mask's, not child's) coordinates
  	Matrix maskToDstSpace( srcToDstSpace );
  	maskToDstSpace.Concat( mask->GetTransform().GetMatrix( NULL ) );
  	Matrix inverse;
  	Matrix::Invert( maskToDstSpace, inverse );

  	Vertex2 p = { dstX, dstY };
  	inverse.Apply( p );

  	PlatformBitmap *bitmap = mask->GetPaint()->GetBitmap();
	if ( Rtt_VERIFY( bitmap ) )
	{
		Real w = bitmap->Width();
		Real h = bitmap->Height();
		
		Real halfW = Rtt_RealDiv2( w );
		Real halfH = Rtt_RealDiv2( h );

		// Map to coordinates where top-left is origin, instead of center.
		Real x = p.x + halfW;
		Real y = p.y + halfH;

		// Useful for debugging
		#if 0
			Rtt_TRACE( ( "TestMask: touch(%d,%d) half(%d,%d) mask(%d,%d)\n",
				Rtt_RealToInt( p.x ), Rtt_RealToInt( p.y ),
				Rtt_RealToInt( halfW ), Rtt_RealToInt( halfH ), 
				Rtt_RealToInt( x ), Rtt_RealToInt( y ) ) );
		#endif

		result = mask->HitTest( allocator, Rtt_RealToInt( x ), Rtt_RealToInt( y ) );
	}

	return result;
}

void
HitEvent::Test( HitTestObject& hitParent, const Matrix& srcToDstSpace ) const
{
	Rtt_ASSERT( hitParent.Target().AsGroupObject() );

	// We need to create a snapshot of the sub-tree of all possible objects 
	// that *might* get dispatched an event.  The model is that the event is 
	// relevant to all objects in the display hierarchy that were present
	// at the time of the event.  Because listeners can remove old objects and
	// insert new ones, this snapshot allows us to dispatch events correctly.
	Matrix xform( srcToDstSpace );
	GroupObject& object = static_cast< GroupObject& >( hitParent.Target() );
	xform.Concat( object.GetMatrix() ); // Object's transform gets applied first

	const StageObject *stage = object.GetStage(); Rtt_ASSERT( stage );

	const Display& display = stage->GetDisplay();
	Rtt_Allocator *allocator = display.GetRuntime().GetAllocator();

	Real x = fXContent;
	Real y = fYContent;

	for ( S32 i = 0, iMax = object.NumChildren(); i < iMax; i++ )
	{
		DisplayObject& child = object.ChildAt( i );

		// Only add visible/hitTestable objects
		// and in the multitouch case, do not have per object focus id set
		// since we dispatch focused events outside of hit testing.
		if ( child.ShouldHitTest() && ! child.GetFocusId() )
		{
			GroupObject* childAsGroup = child.AsGroupObject();
			if ( ! childAsGroup )
			{
//				Rtt_ASSERT( child.IsStageBoundsValid() || ! child.CanCull() );

				// Only test if object is actually on-screen
				// Test bounding box before doing more expensive testing
				if ( ! child.IsOffScreen() && child.StageBounds().HitTest( fXContent, fYContent ) )
				{
					Rtt_ASSERT( child.IsStageBoundsValid() );
					child.Prepare( display );

					// TODO: Should we only do SetForceDraw() if the object is hidden?
					// Ensure Draw() is not a no-op for hidden objects
					// as defined by DisplayObject::IsNotHidden()
					bool oldValue = child.IsForceDraw();
					child.SetForceDraw( true );

					bool didHit = child.HitTest( x, y );

					child.SetForceDraw( oldValue );

					// Only do deeper testing if a mask exists and the "isHitTestMasked" property is true
					if ( didHit && child.IsHitTestMasked() && child.GetMask() )
					{
						Matrix childToDst( xform );
						childToDst.Concat( child.GetMatrix() );
						didHit = TestMask( allocator, child, childToDst, x, y );
					}

					if ( didHit )
					{
						// Only if we hit, do we add child to the snapshot
						HitTestObject* hitChild = Rtt_NEW( object.Allocator(), HitTestObject( child, & hitParent ) );
						hitParent.Prepend( hitChild );
					}
				}
			}
			else
			{
				// By default, we hit test children, but if the group has hit test masking on,
				// then we hit test the group's clipped bounding box before we attempt to
				// hit test the group's children.
				bool hitTestChildren = child.HitTest( x, y );
				if( hitTestChildren && child.IsHitTestMasked() )
				{
					// By default, stage bounds of composite objects are not built.
					child.BuildStageBounds();
					hitTestChildren = child.StageBounds().HitTest( x, y );

					// Only do deeper testing if a mask exists and the "isHitTestMasked" property is true
					if ( hitTestChildren && child.GetMask() )
					{
						Matrix childToDst( xform );
						childToDst.Concat( child.GetMatrix() );

						hitTestChildren = TestMask( allocator, child, childToDst, x, y );
					}
				}

				if ( hitTestChildren )
				{
					HitTestObject* hitGroup = Rtt_NEW( object.Allocator(), HitTestObject( child, & hitParent ) );

					// Recursively call on children
					Test( * hitGroup, xform );
					if ( hitGroup->NumChildren() > 0 )
					{
						// Only groups that contain children that were hit are added to the snapshot
						hitParent.Prepend( hitGroup );
					}
					else
					{
						Rtt_DELETE( hitGroup );
					}
				}
			}
		}
	}
}

bool
HitEvent::DispatchFocused( lua_State *L, Runtime& runtime, StageObject& stage, DisplayObject *focus ) const
{
	Rtt_ASSERT( focus );

	bool handled = false;

	ScreenToContent( runtime.GetDisplay(), fXScreen, fYScreen, fXContent, fYContent );

	// If we have focus, then dispatch only to that object or its ancestors
	Rtt_ASSERT( focus->GetStage() == & stage );

	// Append focus and its ancestors into a list. We'll send the event
	// through this snapshot of the hierarchy.
	PtrArrayDisplayObject hitList( runtime.Allocator() );
	for ( DisplayObject* object = focus;
		  NULL != object;
		  object = object->GetParent() )
	{
		hitList.Append( object );
		object->SetUsedByHitTest( true );
	}

	DisplayObject::ListenerMask mask = (DisplayObject::ListenerMask)GetListenerMask();

	// Now, dispatch event starting at focus
	for ( int i = 0, iMax = hitList.Length();
		  i < iMax;
		  i++ )
	{
		DisplayObject* object = hitList[i];
		if ( ! handled )
		{
			if ( object->HasListener( mask ) )
			{
				handled = object->DispatchEvent( L, * this );
			}
		}
		object->SetUsedByHitTest( false );
	}

	// Prevent the objects in hitList from being deleted by hitList's dtor
	hitList.Remove( 0, hitList.Length(), false );

	return handled;
}

void
HitEvent::ScreenToContent( const Display& display, Real xScreen, Real yScreen, Real& outXContent, Real& outYContent )
{
	// Scale point: map screen coord to content coord
	outXContent = Rtt_RealMul( xScreen - display.GetXScreenOffset(), display.GetScreenToContentScale() );
	outYContent = Rtt_RealMul( yScreen - display.GetYScreenOffset(), display.GetScreenToContentScale() );
}

void
HitEvent::Dispatch( lua_State *L, Runtime& runtime ) const
{
	Super::Dispatch( L, runtime ); // Send to global Runtime

	// TODO: If receiver is called more than once, we need to restore fX, fY
	// or alternatively set some flag so that we know that it's been mapped
	Display& display = runtime.GetDisplay();

	ScreenToContent( display, fXScreen, fYScreen, fXContent, fYContent );

	StageObject& stage = * display.GetStage();
	DisplayObject* focus = stage.GetFocus();
	if ( focus )
	{
		DispatchFocused( L, runtime, stage, focus );
	}
	else
	{
		Matrix identity;
		stage.UpdateTransform( identity );
		HitTestObject root( stage, NULL );
		Test( root, identity ); // Generates subtree snapshot
		DispatchEvent( L, root ); // Dispatches to that subtree
	}

	// Cleanup: Move objects from the snapshot orphanage to the real orphanage
	// so they can be properly removed at the end of the event dispatch scope.
	GroupObject& hitTestOrphanage = * display.HitTestOrphanage();
	GroupObject& orphanage = * display.Orphanage();
	for ( int i = hitTestOrphanage.NumChildren(); --i >= 0; )
	{
		DisplayObject& child = hitTestOrphanage.ChildAt( i );
		orphanage.Insert( -1, & child, false );
	}
	Rtt_ASSERT( 0 == hitTestOrphanage.NumChildren() );
}

U32
HitEvent::GetListenerMask() const
{
	return DisplayObject::kUnknownListener;
}

bool
HitEvent::DispatchEvent( lua_State *L, HitTestObject& parent ) const
{
	// We want to do post-order tree walk. The walk traverses the tree in a
	// depth-first manner. This ensures that the frontmost, "deepest" child is 
	// dispatched the event first. If not handled (i.e. no listener or the 
	// listener returns false) then the event is sent to a sibling that's behind
	// and then to their ancestor.

	// NOTE: All objects in this tree were successfully hit-tested!

	bool handled = false;
	const HitEvent& e = *this;

	DisplayObject::ListenerMask mask = (DisplayObject::ListenerMask)GetListenerMask();

	// Traverse children in order --- they were added via Prepend, so the
	// sibling ordering is reverse of the display hierarchy's. The visibly-
	// frontmost objects are at the beginning of the child list
	for ( HitTestObject* o = parent.Child(); o && ! handled; o = o->Sibling() )
	{
		DisplayObject& target = o->Target();

		// Either target is not a group, or if it is, then o has at least one child.
		// We should never have added a HitTestObject whose target is a GroupObject
		// unless that HitTestObject had child hit objects.
		Rtt_ASSERT( ! target.AsGroupObject() || o->NumChildren() > 0 );

		if ( o->NumChildren() == 0 )
		{
			if ( target.HasListener( mask ) )
			{
				handled = target.DispatchEvent( L, e );
			}
		}
		else
		{
			// Recurse into o
			handled = DispatchEvent( L, *o );
		}
	}

	// Post-order tree walk, so only dispatch to the parent *after* the children.
	if ( ! handled )
	{
		handled = parent.Target().DispatchEvent( L, e );
	}

	return handled;
}

// ----------------------------------------------------------------------------

const char*
TouchEvent::StringForPhase( Phase phase )
{
	const char* result = NULL;
	static const char kBeganString[] = "began";
	static const char kMovedString[] = "moved";
	static const char kEndedString[] = "ended";
	static const char kCancelledString[] = "cancelled";

	switch( phase )
	{
		case kBegan:
			result = kBeganString;
			break;
		case kMoved:
			result = kMovedString;
			break;
		case kEnded:
			result = kEndedString;
			break;
		case kCancelled:
			result = kCancelledString;
			break;
		default:
			break;
	}

	return result;
}


TouchEvent::Phase
TouchEvent::phaseForType( int touchType )
{
	Rtt::TouchEvent::Phase result;
	
	switch( touchType )
	{
	case 0:
		result = Rtt::TouchEvent::kBegan;
		break;
	case 1:
		result = Rtt::TouchEvent::kMoved;
		break;
	case 2:
		result = Rtt::TouchEvent::kEnded;
		break;
	case 3:
	default:
		result = Rtt::TouchEvent::kCancelled;
		break;
	}
	
	return result;
}

const char TouchEvent::kName[] = "touch";

TouchEvent::TouchEvent()
:	Super( Rtt_REAL_0, Rtt_REAL_0 ),
	fPhase( kBegan ),
	fProperties( 0 ),
	fXStartScreen( Rtt_REAL_0 ),
	fYStartScreen( Rtt_REAL_0 ),
	fXStartContent( Rtt_REAL_0 ),
	fYStartContent( Rtt_REAL_0 ),
	fDeltaX( Rtt_REAL_0 ),
	fDeltaY( Rtt_REAL_0 )
{
}

TouchEvent::TouchEvent( Real x, Real y, Real xStartScreen, Real yStartScreen, Phase phase )
:	Super( x, y ),
	fPhase( phase ),
	fProperties( 0 ),
	fXStartScreen( xStartScreen ),
	fYStartScreen( yStartScreen ),
	fXStartContent( xStartScreen ),
	fYStartContent( yStartScreen ),
	fDeltaX( x - xStartScreen ),
	fDeltaY( y - yStartScreen )
{
}

const char*
TouchEvent::Name() const
{
	return kName;
}

int
TouchEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		const char kXStartKey[] = "xStart";
		const char kYStartKey[] = "yStart";
		const char kXDeltaKey[] = "xDelta";
		const char kYDeltaKey[] = "yDelta";

		lua_pushstring( L, StringForPhase( (Phase)fPhase ) );
		lua_setfield( L, -2, kPhaseKey );

		lua_pushnumber( L, Rtt_RealToFloat( fXStartContent ) );
		lua_setfield( L, -2, kXStartKey );
		lua_pushnumber( L, Rtt_RealToFloat( fYStartContent ) );
		lua_setfield( L, -2, kYStartKey );
		lua_pushinteger( L, Rtt_RealToInt( fDeltaX) );
		lua_setfield( L, -2, kXDeltaKey );
		lua_pushinteger( L, Rtt_RealToInt( fDeltaY ));
		lua_setfield( L, -2, kYDeltaKey );

		if ( GetId() )
		{
			lua_pushlightuserdata( L, const_cast< void * >( GetId() ) );
			lua_setfield( L, -2, "id" );
		}
	}

	return 1;
}

void
TouchEvent::Dispatch( lua_State *L, Runtime& runtime ) const
{
	ScreenToContent( runtime.GetDisplay(), fXStartScreen, fYStartScreen, fXStartContent, fYStartContent );

	Super::Dispatch( L, runtime );
}

bool
TouchEvent::DispatchFocused( lua_State *L, Runtime& runtime, StageObject& stage, DisplayObject *focus ) const
{
	ScreenToContent( runtime.GetDisplay(), fXStartScreen, fYStartScreen, fXStartContent, fYStartContent );

	return Super::DispatchFocused( L, runtime, stage, focus );
}

U32
TouchEvent::GetListenerMask() const
{
	return DisplayObject::kTouchListener;
}

void
TouchEvent::SetProperty( U16 mask, bool value )
{
	const U16 p = fProperties;
	fProperties = ( value ? p | mask : p & ~mask );
}

// ----------------------------------------------------------------------------

MultitouchEvent::MultitouchEvent( TouchEvent *touches, int numTouches )
:	Super(),
	fTouches( touches ),
	fNumTouches( numTouches )
{
	Rtt_ASSERT( numTouches > 0 );
}

const char*
MultitouchEvent::Name() const
{
	return TouchEvent::kName;
}

void
MultitouchEvent::Dispatch( lua_State *L, Runtime& runtime ) const
{
	TouchEvent::Phase phase = fTouches[0].GetPhase(); // all fTouches should be in the same phase
	bool shouldCleanup = ( TouchEvent::kEnded == phase || TouchEvent::kCancelled == phase );

	Display& display = runtime.GetDisplay();
	StageObject& stage = * display.GetStage();

	// Iterate through the touches, dispatching individually
	for ( int i = 0, iMax = fNumTouches; i < iMax; i++ )
	{
		const TouchEvent& e = fTouches[i];
		DisplayObject *object = stage.GetFocus( e.GetId() );
		if ( object )
		{
			// Dispatch focused per-object touch events
			e.DispatchFocused( L, runtime, stage, object );

			// Cleanup: remove per-object focus at the end of the touch (e.g. phase is "ended" or "cancelled")
			if ( shouldCleanup )
			{
				stage.SetFocus( object, NULL );
			}
		}
		else
		{
			// Dispatch touch event normally
			e.Dispatch( L, runtime );
		}
	}
}
	
// ----------------------------------------------------------------------------

const char RelativeTouchEvent::kName[] = "relativeTouch";

RelativeTouchEvent::RelativeTouchEvent( Real x, Real y, Phase phase )
:	TouchEvent( x, y, Rtt_REAL_0, Rtt_REAL_0, phase )
{
}

const char*
RelativeTouchEvent::Name() const
{
	return RelativeTouchEvent::kName;
}

// TouchEvents try to adjust based on content scaling and screen origins. For relative touch events, we
// don't care about the screen, only the relationship between the initial touch and follow-up movements.
// We also do not want hit testing. So, we bypass ScreenToContent from TouchEvent and content/focus tests
// from HitEvent.
void
RelativeTouchEvent::Dispatch( lua_State *L, Runtime& runtime ) const
{
	VirtualEvent::Dispatch( L, runtime );
}

// ----------------------------------------------------------------------------

DragEvent::DragEvent( Real x, Real y, Real dstX, Real dstY )
:	Super( dstX, dstY ),
	fDeltaX( dstX - x ),
	fDeltaY( dstY - y )
{
}

const char*
DragEvent::Name() const
{
	static const char kName[] = "drag";

	return kName;
}

int
DragEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		const char kXDeltaKey[] = "xDelta";
		const char kYDeltaKey[] = "yDelta";

		lua_pushinteger( L, Rtt_RealToInt( fDeltaX ) );
		lua_setfield( L, -2, kXDeltaKey );
		lua_pushinteger( L, Rtt_RealToInt( fDeltaY ) );
		lua_setfield( L, -2, kYDeltaKey );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char MouseEvent::kName[] = "mouse";

const char*
MouseEvent::StringForMouseEventType( MouseEventType eventType )
{
    const char* result = NULL;
    static const char kGenericString[] = "generic";
    static const char kUpString[] = "up";
    static const char kDownString[] = "down";
    static const char kDragString[] = "drag";
    static const char kMoveString[] = "move";
    static const char kScrollString[] = "scroll";
  
    switch( eventType )
    {
        case kGeneric:
            result = kGenericString;
            break;
        case kUp:
            result = kUpString;
            break;
        case kDown:
            result = kDownString;
            break;
        case kDrag:
            result = kDragString;
            break;
        case kMove:
            result = kMoveString;
            break;
        case kScroll:
            result = kScrollString;
            break;
        default:
            break;
    }
    
    return result;
}

    
MouseEvent::MouseEvent(MouseEventType eventType, Real x, Real y, Real scrollX, Real scrollY, int clickCount,
                       bool isPrimaryButtonDown, bool isSecondaryButtonDown, bool isMiddleButtonDown,
                       bool isShiftDown, bool isAltDown, bool isCtrlDown, bool isCommandDown)
:	fEventType(eventType),
    Super( x, y ),
    fScrollX(scrollX),
    fScrollY(scrollY),
    fClickCount(clickCount),
	fIsPrimaryButtonDown( isPrimaryButtonDown ),
	fIsSecondaryButtonDown( isSecondaryButtonDown ),
	fIsMiddleButtonDown( isMiddleButtonDown ),
    fIsShiftDown( isShiftDown ),
    fIsAltDown( isAltDown ),
    fIsCtrlDown( isCtrlDown ),
    fIsCommandDown( isCommandDown )
{
}

const char*
MouseEvent::Name() const
{
	return kName;
}

int
MouseEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		lua_pushnumber( L, fScrollX );
		lua_setfield( L, -2, "scrollX" );
        
		lua_pushnumber( L, fScrollY );
		lua_setfield( L, -2, "scrollY" );
        
		lua_pushinteger( L, fClickCount );
		lua_setfield( L, -2, "clickCount" );
        
		lua_pushboolean( L, fIsPrimaryButtonDown ? 1 : 0 );
		lua_setfield( L, -2, "isPrimaryButtonDown" );
        
		lua_pushboolean( L, fIsSecondaryButtonDown ? 1 : 0 );
		lua_setfield( L, -2, "isSecondaryButtonDown" );
		
		lua_pushboolean( L, fIsMiddleButtonDown ? 1 : 0 );
		lua_setfield( L, -2, "isMiddleButtonDown" );
        
		lua_pushboolean( L, fIsShiftDown );
		lua_setfield( L, -2, "isShiftDown" );
        
		lua_pushboolean( L, fIsAltDown );
		lua_setfield( L, -2, "isAltDown" );
        
		lua_pushboolean( L, fIsCtrlDown );
		lua_setfield( L, -2, "isCtrlDown" );
        
		lua_pushboolean( L, fIsCommandDown );
		lua_setfield( L, -2, "isCommandDown" );
        
		lua_pushstring( L, StringForMouseEventType(fEventType) );
		lua_setfield( L, -2, "type" );
	}

	return 1;
}

U32
MouseEvent::GetListenerMask() const
{
	return DisplayObject::kMouseListener;
}

// ----------------------------------------------------------------------------
/*
void
VirtualLocalEvent::Dispatch( lua_State *L, Runtime& runtime ) const
{
	// Should never call this version for local events
	Rtt_ASSERT_NOT_REACHED();
}
*/
// ----------------------------------------------------------------------------

const char FBConnectBaseEvent::kName[] = "fbconnect";

const char*
FBConnectBaseEvent::StringForType( Type t )
{
	const char* result = NULL;
	static const char kSessionString[] = "session";
	static const char kRequestString[] = "request";
	static const char kDialogString[] = "dialog";

	switch( t )
	{
		case kSession:
			result = kSessionString;
			break;
		case kRequest:
			result = kRequestString;
			break;
		case kDialog:
			result = kDialogString;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

FBConnectBaseEvent::FBConnectBaseEvent( Type t )
:	fResponse( NULL ),
	fIsError( false ),
	fType( t )
{
}

FBConnectBaseEvent::FBConnectBaseEvent( Type t, const char *response, bool isError )
:	fResponse( response ),
	fIsError( isError ),
	fType( t )
{
}

const char*
FBConnectBaseEvent::Name() const
{
	return Self::kName;
}

int
FBConnectBaseEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		const char *value = StringForType( (Type)fType ); Rtt_ASSERT( value );
		lua_pushstring( L, value );
		lua_setfield( L, -2, kTypeKey );

		lua_pushboolean( L, fIsError );
		lua_setfield( L, -2, kIsErrorKey );

		const char *message = fResponse ? fResponse : "";
		lua_pushstring( L, message );
		lua_setfield( L, -2, kResponseKey );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char*
FBConnectSessionEvent::StringForPhase( Phase phase )
{
	const char* result = NULL;
	static const char kLoginString[] = "login";
	static const char kLoginFailedString[] = "loginFailed";
	static const char kLoginCancelledString[] = "loginCancelled";
	static const char kLogoutString[] = "logout";

	switch( phase )
	{
		case kLogin:
			result = kLoginString;
			break;
		case kLoginFailed:
			result = kLoginFailedString;
			break;
		case kLoginCancelled:
			result = kLoginCancelledString;
			break;
		case kLogout:
			result = kLogoutString;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

FBConnectSessionEvent::FBConnectSessionEvent( const char *token, time_t tokenExpiration )
:	Super( Super::kSession ),
	fPhase( kLogin ),
	fToken( token ),
	fTokenExpiration( tokenExpiration )
{
}

FBConnectSessionEvent::FBConnectSessionEvent( Phase phase )
:	Super( Super::kSession ),
	fPhase( phase ),
	fToken( NULL ),
	fTokenExpiration( 0 )
{
}

FBConnectSessionEvent::FBConnectSessionEvent( Phase phase, const char errorMsg[] )
:	Super( Super::kSession, errorMsg, true ),
	fPhase( phase ),
	fToken( NULL ),
	fTokenExpiration( 0 )
{
}

int
FBConnectSessionEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		const char *value = StringForPhase( (Phase)fPhase ); Rtt_ASSERT( value );
		lua_pushstring( L, value );
		lua_setfield( L, -2, kPhaseKey );

		if ( fToken )
		{
			Rtt_ASSERT( kLogin == fPhase );
			lua_pushstring( L, fToken );
			lua_setfield( L, -2, "token" );

			lua_pushnumber( L, fTokenExpiration );
			lua_setfield( L, -2, "expiration" );
		}
	}

	return 1;
}

// ----------------------------------------------------------------------------

FBConnectRequestEvent::FBConnectRequestEvent( const char *response, bool isError )
:	Super( Super::kRequest, response, isError ),
	fDidComplete( false )
{
}

FBConnectRequestEvent::FBConnectRequestEvent( const char *response, bool isError, bool didComplete )
:	Super( Super::kDialog, response, isError ),
	fDidComplete( didComplete )
{
}

int
FBConnectRequestEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		lua_pushboolean( L, fDidComplete );
		lua_setfield( L, -2, "didComplete" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char CompletionEvent::kName[] = "completion";

const char*
CompletionEvent::Name() const
{
	return Self::kName;
}

// ----------------------------------------------------------------------------
	
const char GameNetworkEvent::kName[] = "gameNetwork";

const char*
GameNetworkEvent::Name() const
{
	return Self::kName;
}

// ----------------------------------------------------------------------------

const char UrlRequestEvent::kName[] = "urlRequest";

const char*
UrlRequestEvent::StringForType( Type type )
{
	const char *result = NULL;

	static const char kLinkString[] = "link";
	static const char kFormString[] = "form";
	static const char kHistoryString[] = "history";
	static const char kReloadString[] = "reload";
	static const char kFormResubmittedString[] = "formResubmit";
	static const char kOtherString[] = "other";
	static const char kLoadedString[] = "loaded";

	switch ( type )
	{
		case kLink:
			result = kLinkString;
			break;
		case kForm:
			result = kFormString;
			break;
		case kHistory:
			result = kHistoryString;
			break;
		case kReload:
			result = kReloadString;
			break;
		case kFormResubmitted:
			result = kFormResubmittedString;
			break;
		case kOther:
			result = kOtherString;
			break;
		case kLoaded:
			result = kLoadedString;
			break;
		default:
			break;
	}

	return result;
}

UrlRequestEvent::UrlRequestEvent( const char *url, Type type )
:	fUrl( url ),
	fType( type ),
	fErrorMsg( NULL ),
	fErrorCode( 0 )
{
	Rtt_ASSERT( url );
}

UrlRequestEvent::UrlRequestEvent( const char *url, const char *errorMsg, S32 errorCode )
:	fUrl( url ),
	fType( kUnknown ),
	fErrorMsg( errorMsg ),
	fErrorCode( errorCode )
{
	Rtt_ASSERT( url );
}

const char*
UrlRequestEvent::Name() const
{
	return Self::kName;
}

int
UrlRequestEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushstring( L, fUrl );
		lua_setfield( L, -2, "url"  );

		const char *typeValue = StringForType( fType );
		if ( typeValue )
		{
			lua_pushstring( L, typeValue );
			lua_setfield( L, -2, kTypeKey  );
		}

		if ( fErrorMsg )
		{
			lua_pushstring( L, fErrorMsg );
			lua_setfield( L, -2, kErrorMessageKey );
			lua_pushinteger( L, fErrorCode );
			lua_setfield( L, -2, kErrorCodeKey );
		}
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char*
SpriteEvent::StringForPhase( Phase phase )
{
	static const char kBeganString[] = "began";
	static const char kNextString[] = "next";
	static const char kLoopString[] = "loop";
	static const char kEndedString[] = "ended";

	const char *result = NULL;

	switch( phase )
	{
		case kBegan:
			result = kBeganString;
			break;
		case kNext:
			result = kNextString;
			break;
		case kLoop:
			result = kLoopString;
			break;
		case kEnded:
			result = kEndedString;
			break;
		default:
			Rtt_ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

const char
SpriteEvent::kName[] = "sprite";

SpriteEvent::SpriteEvent( const SpriteObject& target, Phase phase, const char * sequenceName, int loopIndex, int effectiveFrameIndex, int frameIndex, int frameIndexInSheet )
:	fTarget( target ),
	fPhase( phase ),
	fSequenceName(sequenceName),
	fLoopIndex(loopIndex),
	fEffectiveFrameIndex(effectiveFrameIndex),
	fFrameIndex(frameIndex),
	fFrameIndexInSheet(frameIndexInSheet)
{
}

const char*
SpriteEvent::Name() const
{
	return kName;
}

int
SpriteEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		lua_pushstring( L, StringForPhase( (Phase)fPhase ) );
		lua_setfield( L, -2, kPhaseKey );
		
		lua_pushstring( L, fSequenceName );
		lua_setfield( L, -2, "sequence" );
		
		lua_pushinteger( L, fLoopIndex );
		lua_setfield( L, -2, "loopIndex" );
		
		lua_pushinteger( L, fEffectiveFrameIndex );
		lua_setfield( L, -2, "frame" );
		
		lua_pushinteger( L, fFrameIndexInSheet );
		lua_setfield( L, -2, "frameInSheet" );
		
		lua_pushinteger( L, fFrameIndex );
		lua_setfield( L, -2, "frameInSequence" );
		
		fTarget.GetProxy()->PushTable( L );
		lua_setfield( L, -2, "target" );
	}

	return 1;
}

// ----------------------------------------------------------------------------

const char NetworkRequestEvent::kName[] = "networkRequest";

const char*
NetworkRequestEvent::Name() const
{
	return Self::kName;
}

NetworkRequestEvent::NetworkRequestEvent( const char *url, const char *responseString, int statusCode, const char *rawHeader )
:	fUrl( url ),
	fResponseString( responseString ),
	fStatusCode( statusCode ),
	fRawHeader( rawHeader ),
	fIsError( statusCode <= 0 )
{
}

NetworkRequestEvent::NetworkRequestEvent( const char *url, const char *errorMessage )
:	fUrl( url ),
	fResponseString( errorMessage ),
	fStatusCode( -1 ),
	fRawHeader( NULL ),
	fIsError( true )
{
}

int
NetworkRequestEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		Rtt_ASSERT( lua_istable( L, -1 ) );

		lua_pushstring( L, fUrl );
		lua_setfield( L, -2, "url"  );

		lua_pushstring( L, fResponseString ? fResponseString : "" );
		lua_setfield( L, -2, kResponseKey );

		if ( fStatusCode > 0 )
		{
			lua_pushinteger( L, fStatusCode );
			lua_setfield( L, -2, "status"  );
		}

		if ( fRawHeader )
		{
			lua_pushstring( L, fRawHeader );
			lua_setfield( L, -2, "header" );
		}
		
		lua_pushboolean( L, fIsError );
		lua_setfield( L, -2, kIsErrorKey );
	}

	return 1;
}

void
NetworkRequestEvent::DispatchEvent( const LuaResource& resource )
{
	int nargs = resource.PushListenerAndEvent( * this );
	if ( nargs > 0 )
	{
		lua_State *L = resource.L(); Rtt_ASSERT( L );

		// Runtime can be NULL
		Runtime *runtime = LuaContext::GetRuntime( L );
		const MPlatform& platform = LuaContext::GetPlatform( L );

		if ( runtime ) { platform.BeginRuntime( * runtime ); }

		(void) Rtt_VERIFY( 0 == LuaContext::DoCall( L, nargs, 0 ) );

		if ( runtime ) { platform.EndRuntime( * runtime ); }
	}
}

// ----------------------------------------------------------------------------

const char AdsRequestEvent::kName[] = "adsRequest";

AdsRequestEvent::AdsRequestEvent( const char *provider, bool isError )
:	fProvider( provider ),
	fIsError( isError )
{
}

const char*
AdsRequestEvent::Name() const
{
	return Self::kName;
}

int
AdsRequestEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushstring( L, fProvider );
		lua_setfield( L, -2, kProviderKey );

		lua_pushboolean( L, fIsError );
		lua_setfield( L, -2, kIsErrorKey );
	}
	
	return 1;
}

// ----------------------------------------------------------------------------

const char PopupClosedEvent::kName[] = "popupClosed";

PopupClosedEvent::PopupClosedEvent( const char *popupName, bool wasCanceled )
:	fPopupName( popupName ),
	fWasCanceled( wasCanceled )
{
}

const char*
PopupClosedEvent::Name() const
{
	return Self::kName;
}

int
PopupClosedEvent::Push( lua_State *L ) const
{
	if ( Rtt_VERIFY( Super::Push( L ) ) )
	{
		lua_pushstring( L, fPopupName );
		lua_setfield( L, -2, kTypeKey );
		
		lua_pushboolean( L, fWasCanceled ? 1 : 0 );
		lua_setfield( L, -2, "cancelled" );
	}
	
	return 1;
}

// ----------------------------------------------------------------------------

const char FinalizeEvent::kName[] = "finalize";

FinalizeEvent::FinalizeEvent()
{
}

const char*
FinalizeEvent::Name() const
{
	return Self::kName;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

