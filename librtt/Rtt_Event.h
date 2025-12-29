//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Event_H__
#define _Rtt_Event_H__

// Events are either dispatched to their corresponding target or broadcast to
// the Runtime instance (the Lua table, not the C++ object).
// 
// Events with targets can be considered "local" (or per-object) events because
// they are dispatched to the listeners belonging to the "deepest" target before 
// they emanate outward to parent objects (like the wake of a pebble dropping in
// a pond). These targets are subclasses of "EventListener" allowing listeners
// to be registered directly with the target.
// 
// Events without targets can be considered "global" b/c they are broadcast to
// the registered listeners of the global "Runtime" object. In this case, they 
// are broadcast immediately to all registered listeners of "Runtime".
// 
// For local events, the dispatch sequence follows a chain of responders, first
// being dispatched to the "deepest" target. If unhandled, it continues to
// propagate to parent targets until handled. If no target responds,
// the event is dispatched to the global (Lua-side) "Runtime" instance, the 
// same as if it were "global".
// 
// A note on "handled". This is really a misnomer, b/c it really means 
// "terminate propagation". This is useful for listeners wishing to process 
// an event and then passing on the event to parent objects.
// ----------------------------------------------------------------------------

#include "Core/Rtt_Types.h"
#include "Core/Rtt_Real.h"
#include "Renderer/Rtt_RenderTypes.h"

#include <time.h>

struct lua_State;

namespace Rtt
{

class Display;
class HitTestObject;
class StageObject;
class GroupObject;
class DisplayObject;
class SpriteObject;
class Matrix;
class PlatformInputAxis;
class PlatformInputDevice;
class Runtime;
class LuaResource;
class UserdataWrapper;
struct RGBA;

// ============================================================================

class MEvent
{
	public:
		static const char kNameKey[];
		static const char kProviderKey[];
		static const char kPhaseKey[];
		static const char kTypeKey[];
		static const char kResponseKey[];
		static const char kIsErrorKey[];
		static const char kErrorCodeKey[];
		static const char kDataKey[];

	public:
		virtual const char* Name() const = 0;
		virtual int Push( lua_State *L ) const = 0;
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const = 0;
};

// Base class for all derived event types
class VirtualEvent : public MEvent
{
	public:
		virtual ~VirtualEvent();

	public:
		virtual int Push( lua_State *L ) const;

	protected:
		int PrepareDispatch( lua_State *L ) const;

	public:
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;
};

// Base class for all events that want to report an error
class ErrorEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		ErrorEvent();

	public:
		virtual int Push( lua_State *L ) const;

	protected:
		void SetError( const char *errorMsg, int errorCode );

	private:
		const char *fErrorMsg;
		int fErrorCode;
};

// ============================================================================

// Immediately broadcast to "Runtime"
class FrameEvent : public VirtualEvent
{
	// TODO: Add notion of current time or time since last frame event?
	public:
		typedef VirtualEvent Super;

	public:
		static const FrameEvent& Constant();

	protected:
		FrameEvent();

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
};

// ============================================================================

// Immediately broadcast to "Runtime"
class RenderEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		static const RenderEvent& Constant();

	protected:
		RenderEvent();

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class SystemEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		typedef enum _Type
		{
			kOnAppExit = 0,	// Use SystemEvent
			kOnAppStart,	// Use SystemEvent
			kOnAppSuspend,	// Use SystemEvent
			kOnAppResume,	// Use SystemEvent
			kOnAppOpen,		// Use SystemOpenEvent. Delivered after kOnAppStart.

			kNumTypes
		}
		Type;

	public:
		static const char* StringForType( Type type );

	protected:
		// System events should be bottlenecked via Runtime
		SystemEvent( Type type );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		U8 fType;

		friend class Runtime;
};


// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class SystemOpenEvent : public SystemEvent
{
	public:
		typedef SystemEvent Super;

	public:
		SystemOpenEvent( const char *url );

	public:
		void SetCommandLineArgs(int argumentCount, const char **arguments);
		void SetCommandLineDir(const char *directoryPath);
		virtual int Push(lua_State *L) const;

	private:
		const char *fUrl;
		const char *fCommandLineDirectoryPath;
		const char **fCommandLineArgumentArray;
		int fCommandLineArgumentCount;

		friend class Runtime;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class ResizeEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		ResizeEvent();

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class WindowStateEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		WindowStateEvent(bool foreground);

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
	private:
		bool fForeground;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class AccelerometerEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		// We use doubles here b/c lua_Numbers are double and the source data was typically double
		AccelerometerEvent( double* gravity, double* instant, double* raw, bool isShake, double deltaTime, PlatformInputDevice *device = NULL );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		// We use doubles here b/c lua_Numbers are double and the source data was typically double
		double* fGravityAccel;
		double* fInstantAccel;
		double* fRawAccel;
		bool fIsShake;
		double fDeltaTime;
		PlatformInputDevice* fDevice;
};

// ----------------------------------------------------------------------------

/// Provides rotation data from the last gyroscope measurement.
class GyroscopeEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		
	public:
		GyroscopeEvent( double xRotation, double yRotation, double zRotation, double deltaTime, PlatformInputDevice *device = NULL );
	
	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
		
	private:
		double fXRotation;
		double fYRotation;
		double fZRotation;
		double fDeltaTime;
		PlatformInputDevice *fDevice;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class MemoryWarningEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		
	public:
		MemoryWarningEvent( );
		
	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class NotificationEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		typedef enum _Type
		{
			kLocal = 0,
			kRemote,
			kRemoteRegistration,
			
			kNumTypes
		}
		Type;

		typedef enum _ApplicationState
		{
			kBackground = 0,
			kActive,
			kInactive,

			kNumStates
		}
		ApplicationState;
		
	public:
		static const char* StringForType( Type type );
		static const char* StringForApplicationState( ApplicationState state );

	public:
		NotificationEvent( Type t, ApplicationState state );
		
	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		U8 fType;
		U8 fApplicationState;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class InputDeviceStatusEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		InputDeviceStatusEvent(
				PlatformInputDevice *device, bool hasConnectionStateChanged, bool wasReconfigured );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		PlatformInputDevice *fDevice;
		bool fHasConnectionStateChanged;
		bool fWasReconfigured;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class KeyEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

		typedef enum Phase
		{
			kDown = 0,
			kUp,

			kNumPhases
		}
		Phase;

	protected:
		static const char* StringForPhase( Phase phase );

	public:
		KeyEvent(
			PlatformInputDevice *device, Phase phase, const char *keyName, S32 nativeKeyCode,
			bool isShiftDown, bool isAltDown, bool isCtrlDown, bool isCommandDown,
			const char *qwertyKeyName = NULL );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;

	public:
		bool GetResult() const { return fResult; }
		const char* GetQwertyKeyName() const { return fQwertyKeyName; }

	protected:
		PlatformInputDevice *fDevice;
		U32 fPhase;
		const char *fKeyName;
		S32 fNativeKeyCode;
		bool fIsShiftDown;
		bool fIsAltDown;
        bool fIsCtrlDown;
        bool fIsCommandDown;
		const char *fQwertyKeyName;
		mutable bool fResult;
};
	
// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class CharacterEvent : public VirtualEvent
{
public:
	typedef VirtualEvent Super;
	virtual const char* Name() const;
	
public:
	CharacterEvent(PlatformInputDevice *device, const char *character);
	
public:
	virtual int Push( lua_State *L ) const;
	virtual void Dispatch( lua_State *L, Runtime& runtime ) const;
	
public:
	bool GetResult() const { return fResult; }
	
protected:
	PlatformInputDevice *fDevice;
	const char *fCharacter;
	mutable bool fResult;
};

// ----------------------------------------------------------------------------

// Immediately broadcast to "Runtime"
class AxisEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		AxisEvent(PlatformInputDevice *devicePointer, PlatformInputAxis *axisPointer, Rtt_Real rawValue);

		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	protected:
		PlatformInputDevice *fDevicePointer;
		PlatformInputAxis *fAxisPointer;
		Rtt_Real fRawValue;
};

// ----------------------------------------------------------------------------

class ColorSampleEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef ColorSampleEvent Self;

	public:
		static const char kName[];

		ColorSampleEvent( float x, float y, RGBA &color );

		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	protected:
		float fX;
		float fY;
		RGBA fColor;
};

// ============================================================================

class HitTestStream;

// Dispatched to target
class HitEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		HitEvent( Real xScreen, Real yScreen );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;

	public:
		bool DispatchFocused( lua_State *L, Runtime& runtime, StageObject& stage, DisplayObject *focus ) const;

	protected:
		virtual U32 GetListenerMask() const;

	public:
		Rtt_INLINE Real X() const { return fXContent; }
		Rtt_INLINE Real Y() const { return fYContent; }

	public:
//		void SetX( Real newValue ) { fX = newValue; }
//		void SetY( Real newValue ) { fY = newValue; }

	public:
		void InvalidateTime();
		void SetTime( double newValue ) const { fTime = newValue; }

	public:
		void SetId( const void *newValue ) { fId = newValue; }
		const void* GetId() const { return fId; }

	protected:
		void Test( HitTestObject& parent, const Matrix& srcToDstSpace ) const;

	protected:
		static void ScreenToContent( const Display& display,  Real xScreen, Real yScreen, Real& outXContent, Real& outYContent );
		bool DispatchEvent( lua_State *L, HitTestObject& parent ) const;

	private:
		mutable Real fXContent;
		mutable Real fYContent;
		Real fXScreen;
		Real fYScreen;
		mutable double fTime;
		const void *fId;
};

// ----------------------------------------------------------------------------

// Dispatched to target
class TouchEvent : public HitEvent
{
	public:
		typedef HitEvent Super;

	public:
		typedef enum Phase
		{
			kBegan = 0,
			kMoved,
			kEnded,
			kCancelled,

			kNumPhases
		}
		Phase;

	public:
		enum PropertyMask
		{
			kIsPerObjectFocus = 0x1
		};

	public:
		static const char kName[];
		static const char* StringForPhase(Phase phase);

	public:
		TouchEvent();
		TouchEvent( Real x, Real y, Real xStart, Real yStart, Phase phase );

	public:
		Phase GetPhase() const { return (Phase)fPhase; }
		static Phase phaseForType( int touchType );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;

	public:
		virtual bool DispatchFocused( lua_State *L, Runtime& runtime, StageObject& stage, DisplayObject *focus ) const;

	protected:
		virtual U32 GetListenerMask() const;

	public:
		bool IsProperty( U16 mask ) const { return (fProperties & mask) != 0; }
		void SetProperty( U16 mask, bool value );
		Rtt_FORCE_INLINE Real DeltaX() const { return fDeltaX; };
		Rtt_FORCE_INLINE Real DeltaY() const { return fDeltaY; };

	private:
		U16 fPhase;
		U16 fProperties;
		Real fXStartScreen;
		Real fYStartScreen;
		mutable Real fXStartContent;
		mutable Real fYStartContent;
		Real fDeltaX;
		Real fDeltaY;
};

// ----------------------------------------------------------------------------

// In iPhone, multitouch sequences are per UIView.  So a touch outside a given UIView
// doesn't generate add'l events or get included as the touched for that UIView.
// We should solve this by enabling multiple stages (one stage per UIView) 
// in light of larger screens like the iPad.  We can connect the Stages via some
// centralized inter-Stage event dispatch in the runtime.

// Dispatched to target
class MultitouchEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;

	public:
		MultitouchEvent( TouchEvent *touches, int numTouches );

	public:
		virtual const char* Name() const;
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;

	private:
		TouchEvent *fTouches; // Does not own
		int fNumTouches;
};
	
// ----------------------------------------------------------------------------

// On tvOS, touches performed on the remote are relative to the initial touch
// on that device. We require a new event type for this.
// TODO: Support RelativeTouch events on Mac/Windows through the touchpad?

// Dispatched to target
class RelativeTouchEvent : public TouchEvent
{
	public:
		RelativeTouchEvent( Real x, Real y, Phase phase );
	
	public:
		static const char kName[];
	
	public:
		virtual const char* Name() const;
		virtual void Dispatch( lua_State *L, Runtime& runtime ) const;
};

// ----------------------------------------------------------------------------

// Dispatched to target
class DragEvent : public HitEvent
{
	public:
		typedef HitEvent Super;

	public:
		DragEvent( Real x, Real y, Real dstX, Real dstY );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	public:
		Rtt_FORCE_INLINE Real DstX() const { return X() + fDeltaX; }
		Rtt_FORCE_INLINE Real DstY() const { return Y() + fDeltaY; }
		Rtt_FORCE_INLINE Real DeltaX() const { return fDeltaX; }
		Rtt_FORCE_INLINE Real DeltaY() const { return fDeltaY; }

	private:
		Real fDeltaX;
		Real fDeltaY;
};

// ----------------------------------------------------------------------------

class MouseEvent : public HitEvent
{
	public:
		typedef HitEvent Super;
		static const char kName[];

        typedef enum MouseEventType
        {
            kGeneric = 0,  // used on systems with undifferentiated mouse events
            kUp,
            kDown,
            kDrag,
            kMove,
            kScroll,
            
            kNumMouseEventTypes
        }
        MouseEventType;
    
		MouseEvent(MouseEventType eventType, Real x, Real y, Real scrollX, Real scrollY, int clickCount,
                   bool isPrimaryButtonDown, bool isSecondaryButtonDown, bool isMiddleButtonDown,
                   bool isShiftDown, bool isAltDown, bool isCtrlDown, bool isCommandDown);

		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
		virtual U32 GetListenerMask() const;

    protected:
        static const char* StringForMouseEventType( MouseEventType eventType );

	private:
        MouseEventType fEventType;
		bool fIsPrimaryButtonDown;
		bool fIsSecondaryButtonDown;
		bool fIsMiddleButtonDown;
        Real fScrollX;
        Real fScrollY;
        bool fIsShiftDown;
        bool fIsAltDown;
        bool fIsCtrlDown;
        bool fIsCommandDown;
        int fClickCount;
};

// ----------------------------------------------------------------------------

// Local event
class CompletionEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef CompletionEvent Self;

	public:
		static const char kName[];

	public:
		virtual const char* Name() const;
};

// ----------------------------------------------------------------------------

// Local event
class UrlRequestEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef UrlRequestEvent Self;

	public:
		typedef enum _Type
		{
			kUnknown = -1,
			kLink = 0,
			kForm,
			kHistory,
			kReload,
			kFormResubmitted,
			kOther,
			kLoaded,
			
			kNumTypes
		}
		Type;

	public:
		static const char* StringForType( Type type );

	public:
		static const char kName[];

	public:
		UrlRequestEvent( const char *url, Type type = kUnknown );
		UrlRequestEvent( const char *url, const char *errorMsg, S32 errorCode );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		const char *fUrl;
		Type fType;
		const char *fErrorMsg;
		S32 fErrorCode;
};

// ----------------------------------------------------------------------------

// Local event
class UserInputEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef UserInputEvent Self;

	public:
		static const char kName[];

	public:
		typedef enum Phase
		{
			kBegan = 0,
			kEditing,
			kSubmitted,
			kEnded,

			kNumPhases
		}
		Phase;

	protected:
		static const char* StringForPhase( Phase phase );

	public:
		UserInputEvent( Phase phase );
		UserInputEvent(
			int startpos,
			int numdeleted,
			const char *newchars,
			const char *oldstring,
			const char *newstring );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		Phase fPhase;
		int fStartPos;
		int fNumDeleted;
		const char *fNewChars;
		const char *fOldString;
		const char *fNewString;
};

// ----------------------------------------------------------------------------

// Local event
class SpriteEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef SpriteEvent Self;

	public:
		typedef enum Phase
		{
			kBegan = 0,
			kNext,
			kLoop,
			kEnded,
			
			kNumPhases
		}
		Phase;

	protected:
		static const char* StringForPhase( Phase phase );
		
	public:
		static const char kName[];

	public:
		SpriteEvent( const SpriteObject& target, Phase phase, const char* sequenceName, int loopIndex, int effectiveFrameIndex, int frameIndex, int frameIndexInSheet );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
				
	private:
		const SpriteObject& fTarget;
		U8 fPhase;
		const char* fSequenceName;
		int fLoopIndex;
		int fEffectiveFrameIndex;
		int fFrameIndex;
		int fFrameIndexInSheet;
};

// ----------------------------------------------------------------------------

// Local event
class NetworkRequestEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef NetworkRequestEvent Self;

	public:
		NetworkRequestEvent( const char *url, const char *responseString, int statusCode, const char *rawHeader = NULL );
		NetworkRequestEvent( const char *url, const char *errorMessage );

	public:
 		void DispatchEvent( const LuaResource& resource );

	public:
		static const char kName[];

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
				
	private:
		const char *fUrl;
		const char *fResponseString;
		int fStatusCode;
		const char *fRawHeader;
		bool fIsError;
};

// ----------------------------------------------------------------------------

// Local event
class AdsRequestEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef AdsRequestEvent Self;
		
	public:
		static const char kName[];

	public:
		AdsRequestEvent( const char *provider, bool isError );
		
	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
		
	private:
		const char *fProvider;
		bool fIsError;
};

// ----------------------------------------------------------------------------

class PopupClosedEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef PopupClosedEvent Self;
		static const char kName[];
	
		PopupClosedEvent( const char *popupName, bool wasCanceled );
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;
	
	private:
		const char *fPopupName;
		bool fWasCanceled;
};

// ----------------------------------------------------------------------------

// Local event
class VideoEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef VideoEvent Self;

	public:
		typedef enum Phase
		{
			kReady = 0,
			kEnded,
			kFailed,

			kNumPhases
		}
		Phase;

	public:
		static const char kName[];

	public:
		static const char* StringForPhase( Phase type );

	public:
		VideoEvent( Phase phase );

	public:
		virtual const char* Name() const;
		virtual int Push( lua_State *L ) const;

	private:
		Phase fPhase;
};

// ----------------------------------------------------------------------------

// Local event
class FinalizeEvent : public VirtualEvent
{
	public:
		typedef VirtualEvent Super;
		typedef FinalizeEvent Self;
	
	public:
		static const char kName[];

	public:
		FinalizeEvent();

	public:
		virtual const char* Name() const;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

#endif // _Rtt_Event_H__
