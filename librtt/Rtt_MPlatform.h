//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Platform_H__
#define _Rtt_Platform_H__

#include "Core/Rtt_Types.h"

#include "Display/Rtt_PlatformBitmap.h"
#include "Core/Rtt_Data.h"
#include "Core/Rtt_ResourceHandle.h"
#include "Rtt_PlatformReachability.h" // needed because of enum type
#include "Rtt_Preference.h"

#include <map>
#include <string>

// ----------------------------------------------------------------------------

struct lua_State;
struct ALmixer_Data;

namespace Rtt
{

class LuaResource;
class MCallback;
class MCrypto;
class MPlatformDevice;
class RenderingStream;
class PlatformBitmap;
class PlatformDisplayObject;
class PlatformExitCallback;
class PlatformOpenALPlayer;
class PlatformSurface;
class PlatformTimer;
class PreferenceCollection;
class Runtime;
struct Rect;
class String;

// ----------------------------------------------------------------------------

typedef void *NativeAlertRef;

// TODO: Separate this into 2 interfaces:
// (1) MPlatform: All non-UI api's
// (2) MPlatformInterface: All UI-related api's
class MPlatform
{
	public:
    
		typedef enum _Error
		{
			kOutOfMemoryError,
			kEGLContextInitError,
			//kRuntimeInitError,
			//kPlatformInitError,

			kNumError
		}
		Error;

		/*
		 * The distinction between kResourceDir and kSystemResourceDir is:
		 * 
		 * On the simulator, we need to distinguish between resource files in the app bundle 
		 * (system resources) vs project files (plain resources).  
		 * On the simulator, load a resource.car file that lives in the app bundle (system resource dir).  
		 * It contains the compiled lua for each device skin and the remdebug.engine.
         *
		 * On the device, there's no distinction.
		 */

		typedef enum _Directory
		{
			kResourceDir = 0,
			kDocumentsDir,
			kTmpDir,
			kSystemResourceDir,
			kCachesDir,
			kSystemCachesDir,
			kPluginsDir,
			kVirtualTexturesDir,
			kApplicationSupportDir,
			
			kNumDirs,
			kUnknownDir = -1
		}
		Directory;
	
		static bool IsVirtualDirectory(Directory directory)
		{
			return kVirtualTexturesDir == directory;
		}

		typedef enum _Category
		{
			kLocaleIdentifier = 0,
			kLocaleLanguage,
			kLocaleCountry,
			kUILanguage,
			
			kNumCategories,
			kUnknownCategory = -1
		}
		Category;

	public:
		// Factory methods
		virtual Rtt_Allocator& GetAllocator() const = 0;
		virtual MPlatformDevice& GetDevice() const = 0;
		virtual RenderingStream* CreateRenderingStream() const = 0;
		virtual PlatformSurface* CreateScreenSurface() const = 0;
		virtual PlatformSurface* CreateOffscreenSurface( const PlatformSurface& parent ) const = 0;
		virtual PlatformTimer* CreateTimerWithCallback( MCallback& callback ) const = 0;
		virtual PlatformBitmap* CreateBitmap( const char *filePath, bool convertToGrayscale ) const = 0;
		virtual void SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const = 0;
        virtual bool OpenURL( const char* url ) const = 0;
		// Return values of CanOpenURL: -1 Unknown; 0 No; 1 Yes
		virtual int CanOpenURL( const char* url ) const = 0;
		virtual const MCrypto& GetCrypto() const = 0;

		virtual void GetPreference( Category category, Rtt::String * value ) const = 0;
		virtual Preference::ReadValueResult GetPreference(const char* categoryName, const char* keyName) const = 0;
		virtual OperationResult SetPreferences( const char* categoryName, const PreferenceCollection& collection ) const = 0;
		virtual OperationResult DeletePreferences( const char* categoryName, const char** keyNameArray, U32 keyNameCount ) const = 0;

		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const = 0;

		virtual NativeAlertRef ShowNativeAlert(
			const char *title,
			const char *msg,
			const char **buttonLabels,
			U32 numButtons,
			LuaResource* resource ) const = 0;

		// index is the (0-based) index of the button pressed
		virtual void CancelNativeAlert( NativeAlertRef alert, S32 index ) const = 0;

		// Show modal-like native interfaces that pop up above the Corona view
		// E.g. mail composers, etc.
		virtual bool CanShowPopup( const char *name ) const = 0;
		virtual bool ShowPopup( lua_State *L, const char *name, int optionsIndex ) const = 0;
		virtual bool HidePopup( const char *name ) const = 0;
		
		virtual PlatformDisplayObject* CreateNativeWebView( const Rect& bounds ) const = 0;
		
	public:
		// Creates notification based on table at 'index'. Returns notificationId
		virtual void* CreateAndScheduleNotification( lua_State *L, int index ) const = 0;

		// Creates notification based on table at 'index'. Returns notificationId
		virtual void ReleaseNotification( void *notificationId ) const = 0;

		// Pass "NULL" to cancel all pending notifications
		virtual void CancelNotification( void *notificationId ) const = 0;

	public:
		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const = 0;
#ifdef Rtt_AUTHORING_SIMULATOR
        virtual void SetCursorForRect(const char *cursorName, int x, int y, int width, int height) const = 0;
#endif

	public:
		virtual void SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const = 0;
		virtual int PushNativeProperty( lua_State *L, const char *key ) const = 0;
		virtual int PushSystemInfo( lua_State *L, const char *key ) const = 0;

	public:
		virtual void RaiseError( Error e, const char* reason ) const = 0;

		enum PathBitFlags
		{
			kDefaultPathFlags = 0x0,
			kResultOwnedByCaller = 0x1,
			kTestFileExists = 0x2
		};
        virtual void PathForFile( const char* filename, Directory baseDir, U32 flags, String & result ) const = 0;

		virtual bool FileExists( const char * filename ) const = 0;
	
		virtual int SetSync( lua_State* L ) const = 0;
		virtual int GetSync( lua_State* L ) const = 0;

	public:
		virtual void BeginRuntime( const Runtime& runtime ) const = 0;
		virtual void EndRuntime( const Runtime& runtime ) const = 0;

		virtual PlatformExitCallback* GetExitCallback() = 0;
		virtual bool RequestSystem( lua_State *L, const char *actionName, int optionsIndex ) const = 0;

		virtual void Suspend() const = 0;
		virtual void Resume() const = 0;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Platform_H__
