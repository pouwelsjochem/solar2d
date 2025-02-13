//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_ApplePlatform_H__
#define _Rtt_ApplePlatform_H__

#include "Core/Rtt_Types.h"
#include "Rtt_MPlatform.h"

#include "Rtt_AppleCrypto.h"

#include <objc/objc.h>

#include <map>
#include <string>

// ----------------------------------------------------------------------------

@class NSData;
@class NSDictionary;
@class NSMutableDictionary;
@class NSString;
@class NSArray;
struct ALmixer_Data;

// ----------------------------------------------------------------------------

namespace Rtt
{

struct KeyValuePair;

class RenderingStream;

// ----------------------------------------------------------------------------

class ApplePlatform : public MPlatform
{
	Rtt_CLASS_NO_COPIES( ApplePlatform )

	public:
		typedef ApplePlatform Self;

	// TODO: Move these to a LuaAppleBridge utils section
	public:
		// Converts value at 'index' to corresponding Obj-C value
		static id ToValue( lua_State *L, int index );

		// Creates a dictionary from the Lua table at index 't'
		// TODO: Rename (it returns an autoreleased object which contradicts the 'Create' naming convention)
		static NSMutableDictionary* CreateDictionary( lua_State *L, int t );

		// Copies from NSDictionary into Lua table at 'index'
		static void CopyDictionary( lua_State *L, int index, NSDictionary *src );

		// Creates a Lua table at top of stack and copies in entries from 'src'
		static int CreateAndPushTable( lua_State *L, NSDictionary *src );

		static bool CreateAndPushArray( lua_State *L, NSArray *array );

		// Pushes the 'value' on the Lua stack. Returns false if cannot map 'value' to a Lua type.
		static bool Push( lua_State *L, id value );

	public:
		ApplePlatform();
		virtual ~ApplePlatform();

	public:
		void SetResourceDirectory( NSString *resourcePath );

	protected:
		NSString *GetResourceDirectory() const { return fResourcePath; }

	// MPlatform
	public:
		virtual Rtt_Allocator& GetAllocator() const;
		virtual RenderingStream* CreateRenderingStream() const;
		virtual PlatformSurface* CreateOffscreenSurface( const PlatformSurface& parentDevice ) const;
		virtual PlatformTimer* CreateTimerWithCallback( MCallback& callback ) const;
		virtual PlatformBitmap* CreateBitmap( const char *filePath, bool convertToGrayscale ) const;
		virtual const MCrypto& GetCrypto() const;

		virtual void GetPreference( Category category, Rtt::String * value ) const;
		virtual Preference::ReadValueResult GetPreference( const char* categoryName, const char* keyName ) const;
		virtual OperationResult SetPreferences( const char* categoryName, const PreferenceCollection& collection ) const;
		virtual OperationResult DeletePreferences( const char* categoryName, const char** keyNameArray, U32 keyNameCount ) const;

	public:
		virtual bool CanShowPopup( const char *name ) const;
		virtual bool ShowPopup( lua_State *L, const char *name, int optionsIndex ) const;
		virtual bool HidePopup( const char *name ) const;

	public:
		virtual void* CreateAndScheduleNotification( lua_State *L, int index ) const;
		virtual void ReleaseNotification( void *notificationId ) const;
		virtual void CancelNotification( void *notificationId ) const;

	public:
		virtual void SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const;
		virtual int PushNativeProperty( lua_State *L, const char *key ) const;
		virtual int PushSystemInfo( lua_State *L, const char *key ) const;

	private:
		void NetworkBaseRequest( lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex, NSString *path ) const;

	public:
		void HttpRequest( NSString *url, NSString *method, NSDictionary *params,
			NSString *body, void (^block)( NSData* ) ) const;
		virtual void NetworkRequest( lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex ) const;
		virtual void NetworkDownload( lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex, const char *filename, MPlatform::Directory baseDir ) const;

		virtual PlatformReachability* NewReachability( const ResourceHandle<lua_State>& handle, PlatformReachability::PlatformReachabilityType type, const char* address ) const;
		virtual bool SupportsNetworkStatus() const;
	
	public:
		virtual void RaiseError( MPlatform::Error e, const char* reason ) const;
		virtual void PathForFile( const char* filename, Directory baseDir, U32 flags, String & result ) const;
		virtual bool FileExists( const char * filename ) const;
		virtual bool ValidateAssetFile(const char *assetFilename, const int assetSize) const;

	protected:
		NSString* PathForDir( const char *folderName, NSString *baseDir, bool createIfNotExist ) const;


	protected:
		virtual NSString *PathForResourceFile( const char *filename ) const;
		NSString *PathForSystemResourceFile( const char *filename ) const;
		NSString *PathForCoronaResourceFile( const char* filename ) const;
		virtual NSString *PathForDocumentsFile( const char *filename ) const;
		virtual NSString *PathForApplicationSupportFile( const char* filename ) const;
		virtual NSString *CachesParentDir() const;
		NSString *PathForCachesFile( const char *filename ) const;
		NSString *PathForSystemCachesFile( const char *filename ) const;
		virtual NSString *PathForTmpFile( const char *filename ) const;
		virtual NSString *PathForPluginsFile( const char *filename ) const;
		virtual NSString* PathForFile( const char* filename, NSString* baseDir ) const;

	public:
		virtual int SetSync( lua_State* L ) const;
		virtual int GetSync( lua_State* L ) const;
	
	protected:
		bool SetSynciCloud( NSString* filepath, bool value, NSString** errorstr ) const;
		bool GetSynciCloud( NSString* filepath, bool& value, NSString** errorstr ) const;

	public:
		virtual void BeginRuntime( const Runtime& runtime ) const;
		virtual void EndRuntime( const Runtime& runtime ) const;

		virtual PlatformExitCallback* GetExitCallback();
		virtual bool RequestSystem( lua_State *L, const char *actionName, int optionsIndex ) const;

		virtual void Suspend() const;
		virtual void Resume() const;

	public:
		void SetBundleClass( Class newValue );
#ifdef Rtt_AUTHORING_SIMULATOR
        virtual void SetCursorForRect(const char *cursorName, int x, int y, int width, int height) const;
#endif

	protected:
		Rtt_Allocator* fAllocator;
		NSString *fResourcePath;
		mutable id fHttpPostDelegate;
		mutable id fCustomConnectionDelegate;
		AppleCrypto fCrypto;
		Class fClass;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_ApplePlatform_H__
