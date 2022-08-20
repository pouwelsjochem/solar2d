//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Runtime_H__
#define _Rtt_Runtime_H__

#include "Core/Rtt_Time.h"
#include "Rtt_MCallback.h"
#include "Rtt_MCriticalSection.h"
#include "Rtt_MPlatform.h"
#include "Rtt_Resource.h"

#if defined(Rtt_AUTHORING_SIMULATOR)
#include <atomic>
#include <thread>
#endif

// TODO: Move VirtualEvent to separate header and then move SystemEvent into Runtime.h
// Then, we can remove Event.h from the include list here:
#include "Rtt_Event.h"

// ----------------------------------------------------------------------------

struct lua_State;

namespace Rtt
{

class MEvent;
class MPlatform;
class Archive;
class Display;
class DisplayObject;
class LuaContext;
class MRuntimeDelegate;
class RenderingStream;
class BitmapPaint;
class PlatformExitCallback;
class PlatformSurface;
class PlatformTimer;
class Scheduler;

// ----------------------------------------------------------------------------

class Runtime : public MCallback,
				public MCachedResourceLibrary,
				public MCriticalSection
{
	Rtt_CLASS_NO_COPIES( Runtime )

	public:
		typedef enum _Properties
		{
			kIsDebuggerConnected      = 0x1,
			kIsApplicationLoaded      = 0x2,
			kIsApplicationExecuting   = 0x4,
			kIsUsingCustomCode        = 0x8,
			kUseExitOnErrorHandler    = 0x10,
			kIsLuaParserAvailable     = 0x20,
			kDeferUpdate              = 0x40,
			kRenderAsync              = 0x80,
			kIsApplicationNotArchived = 0x100,
		}
		Properties;

		enum
		{
			// TODO: Remove prefix "Is" to improve clarity
			// TODO: Add suffix "MaskSet" to improve clarity
			//
			// NOTE: Do not use these in IsProperty b/c it violates the semantics, e.g.
			kIsCoronaKit = ( kIsLuaParserAvailable ),
			kLinuxMaskSet = ( kIsLuaParserAvailable ),
		};

	public:
		typedef enum _LaunchOptions
		{
			// Mask
			kConnectToDebugger = 0x1,
			kLaunchDeviceShell = 0x2,
			kCoronaCardsOption = 0x8,

			// Convenience defaults
			kDeviceLaunchOption = kLaunchDeviceShell,
			kSimulatorLaunchOption = kLaunchDeviceShell,
			kCoronaViewOption = (  kLaunchDeviceShell ),
			kWebPluginLaunchOption = 0, // TODO: Remove???
			kLinuxLaunchOption = ( kLaunchDeviceShell ),

#ifdef Rtt_AUTHORING_SIMULATOR
			kDefaultLaunchOption = kSimulatorLaunchOption,
#else
			kDefaultLaunchOption = kDeviceLaunchOption,
#endif
		}
		LaunchOptions;

		typedef enum _SuspendOverrides
		{
			kSuspendAll = 0x0,
			kBackgroundAudio = 0x1,
			kBackgroundLocation = 0x2,
			kBackgroundVoIP = 0x8,
			kSuspendNone = 0xFFFFFFFF
		}
		SuspendOverrides;
		
		// TODO: Rename to LoadResult (to match LoadParameters below)
		typedef enum _LoadApplicationReturnCodes
		{
			kSuccess		= 0,
			kGeneralFail	= 1
		}
		LoadApplicationReturnCodes;
	
	public:
		Runtime( const MPlatform& platform, MCallback *viewCallback = NULL );
		virtual ~Runtime();

	public:
		Rtt_INLINE Rtt_Allocator* Allocator() { return & fAllocator; }
		Rtt_INLINE Rtt_Allocator* Allocator() const { return & fAllocator; }
		Rtt_INLINE bool IsDisplayValid() const { return NULL != fDisplay; }
		Rtt_INLINE Display& GetDisplay() { return * fDisplay; }
		Rtt_INLINE const Display& GetDisplay() const { return * fDisplay; }
		Rtt_INLINE Scheduler& GetScheduler() const { return * fScheduler; }
		Rtt_INLINE const MPlatform& Platform() const { return fPlatform; }

		Rtt_INLINE bool IsVMContextValid() const { return NULL != fVMContext; }
		Rtt_INLINE const LuaContext& VMContext() const { return * fVMContext; }
		Rtt_INLINE LuaContext& VMContext() { return * fVMContext; }

		Rtt_INLINE Archive* GetArchive() { return fArchive; }

		PlatformTimer* GetTimer() { return fTimer; }

	public:
		void SetDelegate( const MRuntimeDelegate *newValue ) { fDelegate = newValue; }
		const MRuntimeDelegate *GetDelegate() const { return fDelegate; }
		
		//const AutoPtr< GPUBitmapTexture >& CreateOrFindTextureWithFilename( const char* filename );

	protected:
		void InitializeArchive( const char *filePath );
		bool VerifyApplication( const char *filePath );

		bool PushConfig( lua_State *L ); // loads config.lua and pushes application.content to top of stack
		void ReadConfig( lua_State *L ); // call before fDisplay is instantiated
		void PopAndClearConfig( lua_State *L );

	protected:
		void AddDownloadablePlugin(
				lua_State *L, const char *pluginName, const char *publisherId,
				int downloadablePluginsIndex, bool isSupportedOnThisPlatform, const char *pluginEntryJSON);

	public:
		void FindDownloadablePlugins( const char *simPlatformName );
		void PushDownloadablePlugins( lua_State *L );
		bool RequiresDownloadablePlugins() const;
		const char *GetSimulatorPlaformName() const { return fSimulatorPlatformName; }

	public:
		// Pushes Lua-table for launch args on the stack.
		int PushLaunchArgs( bool createIfNotExists );
		void ClearLaunchArgs();

	public:
		// Inits VM and then loads script file.
		LoadApplicationReturnCodes LoadApplication( U32 launchOptions );

	public:
		// Inits renderer and then begins timer
		void BeginRunLoop();

	protected:
		void OnSystemEvent( SystemEvent::Type t );

	public:
		// Pauses run loop. If already suspended, then no-op.
		void Suspend(bool sendApplicationEvents = true);

		// Resumes run loop. If already running, then no-op.
		void Resume(bool sendApplicationEvents = true);

		bool IsSuspended() const { return fIsSuspended > 0; }

	private:
		// Internal helper methods to help break up Corona into separate subsystems that can be backgrounded/suspended independently.
		void CoronaAllSuspend();
		void CoronaCoreSuspend();
		void CoronaResumeAll();
		void CoronaCoreResume();
		void CoronaInvokeSystemSuspendEvent();
		void CoronaInvokeSystemResumeEvent();

	public:
		// Broadcast event dispatch, i.e. to Runtime:dispatchEvent()
		void DispatchEvent( const MEvent& e );
//		void DispatchEvent( const MEvent& e, const char *errorMsg, S32 errorCode );

		// Desired fps and interval
		U8 GetFPS() const { return fFPS; }
		float GetFrameInterval() const { return 1.f / ((float)fFPS); }

		U32 GetFrame() const { return fFrame; }

		// Number of ms since app launch
		double GetElapsedMS() const;
		Rtt_AbsoluteTime GetElapsedTime() const;

		void Collect();

		Rtt_INLINE bool IsProperty( U16 mask ) const { return (fProperties & mask) != 0; }
		Rtt_INLINE void ToggleProperty( U16 mask ) { fProperties ^= mask; }
		void SetProperty( U16 mask, bool value );

		bool IsSuspendOverrideProperty( U32 mask ) const { return (fSuspendOverrideProperties & mask) != 0; }
		void ToggleSuspendOverrideProperty( U32 mask ) { fSuspendOverrideProperties ^= mask; }
		void SetSuspendOverrideProperty( U32 mask, bool value );
	
		bool IsCoronaKit() const { return kIsCoronaKit == ( fProperties & kIsCoronaKit ); }
	
	public:
		PlatformExitCallback* GetExitCallback();

	public:
		void UnloadResources();
		void ReloadResources();

	public:
		// MCachedResourceLibrary
		virtual CachedResource* LookupResource( const char key[] );
		virtual void SetResource( CachedResource* resource, const char key[] );
		virtual Rtt_Allocator* GetAllocator() const;
		virtual void MarkRecentlyUsed( CachedResource& resource );

	protected:
		lua_State* PushResourceRegistry();

	public:
		// MCallback
		virtual void operator()();

		void Render();
		
	public:
		// MCriticalSection
		virtual void Begin() const;
		virtual void End() const;

#if defined(Rtt_AUTHORING_SIMULATOR)
	public:
		static int ShellPluginCollector_Async(lua_State* L);
	private:
		static void FinalizeWorkingThreadWithEvent(Runtime *runtime, lua_State *L);
		std::atomic<std::string*> m_fAsyncResultStr;
		std::atomic<std::thread*> m_fAsyncThread;
		std::atomic<void*> m_fAsyncListener;
#endif

	private:
		String fBuildId;
		Rtt_Allocator& fAllocator;
		const MPlatform& fPlatform;
		const Rtt_AbsoluteTime fStartTime;
		Rtt_AbsoluteTime fStartTimeCorrection;
		Rtt_AbsoluteTime fSuspendTime;
		CachedResource* fResourcesHead; // Dummy node.
		Display *fDisplay;
		LuaContext* fVMContext;
		PlatformTimer* fTimer;
		Scheduler* fScheduler;
		Archive* fArchive;
	
#ifdef Rtt_USE_ALMIXER
		PlatformOpenALPlayer* fOpenALPlayer;
#endif

		U8 fFPS;
		S8 fIsSuspended;
		U16 fProperties;
		U32 fSuspendOverrideProperties;
		U32 fFrame;
		int fLaunchArgsRef;
		const char *fSimulatorPlatformName;
		int fDownloadablePluginsRef;
		int fDownloadablePluginsCount;
		const MRuntimeDelegate *fDelegate;

	private:
		friend class LoadMainTask;

	public:
		// to prevent recursive handleError!
		bool fErrorHandlerRecursionGuard;
	public:
		const char* GetBuildId() { return fBuildId.GetString(); }

#ifdef Rtt_USE_ALMIXER
	public:
		PlatformOpenALPlayer* getOpenALPlayer() const{ return fOpenALPlayer; }
#endif
};

// Use this guard to surround code blocks that call Lua as a side effect of
// processing external events, e.g. events driven by the native platform.
class RuntimeGuard
{
	Rtt_CLASS_NO_COPIES( RuntimeGuard )
	Rtt_CLASS_NO_DYNAMIC_ALLOCATION

	public:
		RuntimeGuard( Runtime& runtime );
		~RuntimeGuard();

	private:
		Runtime& fRuntime;
		const MPlatform& fPlatform;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Runtime_H__
