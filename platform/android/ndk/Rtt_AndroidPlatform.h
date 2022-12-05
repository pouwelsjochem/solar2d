//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _Rtt_AndroidPlatform_H__
#define _Rtt_AndroidPlatform_H__

#include "Rtt_AndroidDevice.h"
#include "Rtt_MPlatform.h"
#include "Rtt_AndroidCrypto.h"
#include "Core/Rtt_String.h"

// ----------------------------------------------------------------------------

class AndroidDisplayObjectRegistry;
class AndroidGLView;
class NativeToJavaBridge;

namespace Rtt
{

class PlatformBitmap;
class PlatformSurface;
class PlatformTimer;
class RenderingStream;

// ----------------------------------------------------------------------------

class AndroidPlatform : public MPlatform
{
	Rtt_CLASS_NO_COPIES( AndroidPlatform )

	public:
		typedef AndroidPlatform Self;

	public:
		AndroidPlatform(
				AndroidGLView * pView, const char * package, const char * documentsDir, const char * applicationSupportDir,
				const char * temporaryDir, const char * cachesDir, const char * systemCachesDir, const char * expansionFileDir,
				NativeToJavaBridge *ntjb);
		~AndroidPlatform();

		const String & GetPackage() const 
		{ 
			return fPackage; 
		}

	public:
		virtual MPlatformDevice& GetDevice() const;
		virtual PlatformSurface* CreateScreenSurface() const;
		virtual PlatformSurface* CreateOffscreenSurface( const PlatformSurface& parent ) const;

    public:
        virtual Rtt_Allocator& GetAllocator() const;
        virtual RenderingStream* CreateRenderingStream() const;
        virtual PlatformTimer* CreateTimerWithCallback( MCallback& callback ) const;
        virtual PlatformBitmap* CreateBitmap( const char* filename, bool convertToGrayscale ) const;

		virtual const MCrypto& GetCrypto() const;
		virtual void GetPreference( Category category, Rtt::String * value ) const;
		virtual Preference::ReadValueResult GetPreference( const char* categoryName, const char* keyName ) const;
		virtual OperationResult SetPreferences( const char* categoryName, const PreferenceCollection& collection ) const;
		virtual OperationResult DeletePreferences( const char* categoryName, const char** keyNameArray, U32 keyNameCount ) const;

		virtual bool CanShowPopup( const char *name ) const;
		virtual bool ShowPopup( lua_State *L, const char *name, int optionsIndex ) const;
		virtual bool HidePopup( const char *name ) const;
		
		virtual PlatformDisplayObject* CreateNativeWebView( const Rect& bounds ) const;

		PlatformDisplayObject* GetNativeDisplayObjectById( const int objectId ) const;

	public:
        virtual void RaiseError( MPlatform::Error e, const char* reason ) const;
        virtual void PathForFile( const char *filename, MPlatform::Directory baseDir, U32 flags, String& result ) const;
		virtual bool FileExists( const char *filename ) const;
		virtual bool ValidateAssetFile(const char *assetFilename, const int assetSize) const;

		virtual int SetSync( lua_State* L ) const;
		virtual int GetSync( lua_State* L ) const;

	protected:
		void PathForResourceFile( const char* filename, String & result ) const;
		void PathForPluginFile( const char * filename, String & result ) const;
		void PathForFile( const char* filename, const char * baseDir, String & result ) const;

	public:
        virtual void BeginRuntime( const Runtime& runtime ) const;
        virtual void EndRuntime( const Runtime& runtime ) const;

		virtual PlatformExitCallback* GetExitCallback();
		virtual bool RequestSystem( lua_State *L, const char *actionName, int optionsIndex ) const;
		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const;

    protected:
        char* CopyString( const char* src, bool useAllocator = true ) const;

	public:
		virtual void SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const;
		virtual bool OpenURL( const char* url ) const;
		virtual int CanOpenURL( const char* url ) const;

		virtual PlatformStoreProvider* GetStoreProvider( const ResourceHandle<lua_State>& handle ) const;

		virtual NativeAlertRef ShowNativeAlert(
			const char *title,
			const char *msg,
			const char **buttonLabels,
			U32 numButtons,
			LuaResource* resource ) const;
		virtual void CancelNativeAlert( NativeAlertRef alert, S32 index ) const;

		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const;

	public:
		virtual void* CreateAndScheduleNotification( lua_State *L, int index ) const;
		virtual void ReleaseNotification( void *notificationId ) const;
		virtual void CancelNotification( void *notificationId ) const;
		
	public:
		virtual void SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const;
		virtual int PushNativeProperty( lua_State *L, const char *key ) const;
		virtual int PushSystemInfo( lua_State *L, const char *key ) const;

		virtual NativeToJavaBridge* GetNativeToJavaBridge() const;

		void Suspend( ) const;
		void Resume( ) const;

	protected:
        Rtt_Allocator* fAllocator;
//        mutable id fHttpPostDelegate;

	private:
		AndroidGLView* fView;
		AndroidDevice fDevice;
//		AlertViewDelegate *fDelegate;
		String fPackage;
		String fDocumentsDir;
		String fApplicationSupportDir;
		String fTemporaryDir;
		String fCachesDir;
		String fSystemCachesDir;
		String fExpansionFileDir;
		AndroidCrypto fCrypto;
		AndroidDisplayObjectRegistry *fDisplayObjectRegistry;
		NativeToJavaBridge *fNativeToJavaBridge;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_AndroidPlatform_H__
