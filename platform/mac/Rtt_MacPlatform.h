//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_MacPlatform_H__
#define _Rtt_MacPlatform_H__

#include "Rtt_ApplePlatform.h"
#include "Rtt_MPlatformServices.h"

#include "Rtt_MacDevice.h"
#import <AppKit/NSView.h>
#import <AppKit/NSOpenGLView.h>

#include <sys/types.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

// ----------------------------------------------------------------------------

@class AlertDelegate;
@class GLView;

extern NSString* const kOpenFolderPath;
extern NSString* const kBuildFolderPath;
extern NSString* const kKeyStoreFolderPath;
extern NSString* const kKeyStoreFolderPathAndFile;
extern NSString* const kDstFolderPath;
extern NSString* const kDidAgreeToLicense;
extern NSString* const kUserPreferenceUsersCurrentSelectedSkin;
extern NSString* const kUserPreferenceCustomBuildID;
extern NSString* const kUserPreferenceLastIOSCertificate;
extern NSString* const kUserPreferenceLastTVOSCertificate;
extern NSString* const kUserPreferenceLastOSXCertificate;
extern NSString* const kUserPreferenceLastAndroidKeyAlias;
extern NSString* const kUserPreferenceLastAndroidKeystore;

namespace Rtt
{

class MacConsoleDevice;
class MacViewSurface;
class PlatformSimulator;
class PlatformSurface;

// ----------------------------------------------------------------------------

class MacPlatform : public ApplePlatform
{
	public:
		typedef ApplePlatform Super;

	public:
		MacPlatform(CoronaView *view);
		virtual ~MacPlatform();

	public:
		void Initialize( GLView* pView );

		void SetResourcePath( const char resourcePath[] );

		NSString* GetSandboxPath() const { return fSandboxPath; }

		BOOL IsFilenameCaseCorrect(const char *filename, NSString *path) const;


	public:
		virtual MPlatformDevice& GetDevice() const;
		virtual PlatformSurface* CreateScreenSurface() const;

#ifdef Rtt_AUTHORING_SIMULATOR
	protected:
		ValueResult<Rtt::SharedConstStdStringPtr> GetSimulatedAppPreferenceKeyFor(const char* keyName) const;
#endif

	public:
		virtual Preference::ReadValueResult GetPreference( const char* categoryName, const char* keyName ) const;
		virtual OperationResult SetPreferences( const char* categoryName, const PreferenceCollection& collection ) const;
		virtual OperationResult DeletePreferences( const char* categoryName, const char** keyNameArray, U32 keyNameCount ) const;
		virtual void SaveBitmap( PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const;
		virtual bool OpenURL( const char* url ) const;
		virtual int CanOpenURL( const char* url ) const;

		virtual void SetIdleTimer( bool enabled ) const;
		virtual bool GetIdleTimer() const;

		virtual NativeAlertRef ShowNativeAlert(
			const char *title,
			const char *msg,
			const char **buttonLabels,
			U32 numButtons,
			LuaResource* resource ) const;
		virtual void CancelNativeAlert( NativeAlertRef alert, S32 index ) const;

		virtual PlatformDisplayObject* CreateNativeWebView( const Rect& bounds ) const;
		virtual PlatformDisplayObject* CreateNativeVideo( const Rect& bounds ) const;
		virtual PlatformDisplayObject* CreateNativeTextField( const Rect& bounds ) const;
		virtual void SetKeyboardFocus( PlatformDisplayObject *object ) const;

		virtual int PushSystemInfo( lua_State *L, const char *key ) const;
		virtual const char* GetKeyNameForQwertyKeyName( const char* qwertyKeyName ) const;

		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const;

	public:
		virtual NSString *PathForResourceFile( const char *filename ) const;

	protected:
		virtual NSString *PathForDocumentsFile( const char *filename ) const;
		virtual NSString *CachesParentDir() const;
		virtual NSString *PathForTmpFile( const char *filename ) const;
		virtual NSString *PathForPluginsFile( const char *filename ) const;
		virtual NSString *PathForApplicationSupportFile( const char* filename ) const;
		virtual NSString *PathForFile( const char* filename, NSString* baseDir ) const;



	public:
		virtual void BeginRuntime( const Runtime& runtime ) const;
		virtual void EndRuntime( const Runtime& runtime ) const;

		virtual PlatformExitCallback* GetExitCallback();
		virtual bool RequestSystem( lua_State *L, const char *actionName, int optionsIndex ) const;
		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const;
#ifdef Rtt_AUTHORING_SIMULATOR
        virtual void SetCursorForRect(const char *cursorName, int x, int y, int width, int height) const;
#endif
		virtual void SetNativeProperty(lua_State *L, const char *key, int valueIndex) const;
		virtual int PushNativeProperty( lua_State *L, const char *key ) const;

		virtual void Suspend( ) const;
		virtual void Resume( ) const;

	public:
		MacConsoleDevice& GetMacDevice() const { return static_cast< MacConsoleDevice& >( GetDevice() ); }		
        GLView* GetView() const { return fView; }

	private:
		GLView* fView;
		NSString *fSandboxPath;
		MacConsoleDevice fDevice;
		mutable pthread_mutex_t fMutex;
		mutable int fMutexCount;
		AlertDelegate *fDelegate;
		PlatformExitCallback* fExitCallback;
		mutable IOPMAssertionID fAssertionID;
		mutable bool fIsVsyncEnabled;
};

// TODO: Move this to a separate file
#if !defined( Rtt_WEB_PLUGIN )

class MacGUIPlatform : public MacPlatform
{
	public:
		typedef MacPlatform Super;

	public:
		MacGUIPlatform( PlatformSimulator& simulator );

	public:
		virtual MPlatformDevice& GetDevice() const;
		virtual PlatformSurface* CreateScreenSurface() const;

	public:
		virtual bool RequestSystem( lua_State *L, const char *actionName, int optionsIndex ) const;

	private:
		MacDevice fMacDevice;
};

// ----------------------------------------------------------------------------

class MacAppPlatform : public MacPlatform
{
	public:
		typedef MacPlatform Super;

	public:
		MacAppPlatform();

	public:
		virtual MPlatformDevice& GetDevice() const;

	private:
		MacAppDevice fMacAppDevice;
};

// ----------------------------------------------------------------------------

	
#ifdef Rtt_AUTHORING_SIMULATOR

class MacPlatformServices : public MPlatformServices
{
	Rtt_CLASS_NO_COPIES( MacPlatformServices )

	public:
		typedef MacPlatformServices Self;

	public:
		MacPlatformServices( const MPlatform& platform );

	public:
		// MPlatformServices
		virtual const MPlatform& Platform() const;
		virtual void GetPreference( const char *key, Rtt::String * value ) const;
		virtual void SetPreference( const char *key, const char *value ) const;
		virtual void GetSecurePreference( const char *key, Rtt::String * value ) const;
		virtual bool SetSecurePreference( const char *key, const char *value ) const;
		virtual bool IsInternetAvailable() const;
		virtual bool IsLocalWifiAvailable() const;
		virtual void Terminate() const;

		virtual void Sleep( int milliseconds ) const;

	private:
		const MPlatform& fPlatform;
};

#endif // Rtt_AUTHORING_SIMULATOR
	
// ----------------------------------------------------------------------------

#endif // Rtt_WEB_PLUGIN

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_MacPlatform_H__
