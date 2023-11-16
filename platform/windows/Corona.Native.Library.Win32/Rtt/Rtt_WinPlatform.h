//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Interop\Storage\MStoredPreferences.h"
#include "Rtt_MPlatform.h"
#include "Rtt_PlatformSimulator.h"
#include "Rtt_String.h"
#include "Rtt_WinCrypto.h"
#include "Rtt_WinDevice.h"
#include "Rtt_WinExitCallback.h"
#include "WinString.h"
#include <memory>
#include <windows.h>


#pragma region Forward Declarations
namespace Interop
{
	class RuntimeEnvironment;
}
namespace Rtt
{
	class PlatformBitmap;
	class PlatformSurface;
	class PlatformTimer;
	class RenderingStream;
}
class WinGLView;

#pragma endregion

namespace Rtt
{

class WinPlatform : public MPlatform
{
	Rtt_CLASS_NO_COPIES(WinPlatform)

	public:
		typedef WinPlatform Self;

		WinPlatform(Interop::RuntimeEnvironment& environment);
		virtual ~WinPlatform();

		virtual Rtt_Allocator& GetAllocator() const;
		virtual MPlatformDevice& GetDevice() const;
		virtual RenderingStream* CreateRenderingStream() const;
		virtual PlatformSurface* CreateScreenSurface() const;
		virtual PlatformSurface* CreateOffscreenSurface(const PlatformSurface& parent) const;
		virtual PlatformTimer* CreateTimerWithCallback(MCallback& callback) const;
		virtual PlatformBitmap* CreateBitmap(const char* filename, bool convertToGrayscale) const;
		virtual void SaveBitmap(PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes ) const;
		virtual const MCrypto& GetCrypto() const;
		virtual void GetPreference(Category category, Rtt::String * value) const;
		virtual Preference::ReadValueResult GetPreference(const char* categoryName, const char* keyName) const;
		virtual OperationResult SetPreferences(const char* categoryName, const PreferenceCollection& collection) const;
		virtual OperationResult DeletePreferences(const char* categoryName, const char** keyNameArray, U32 keyNameCount) const;
		virtual bool OpenURL(const char* url) const;
		virtual int CanOpenURL(const char* url) const;
		virtual void SetIdleTimer(bool enabled) const;
		virtual bool GetIdleTimer() const;
		virtual NativeAlertRef ShowNativeAlert(
					const char *title, const char *message, const char **buttonLabels,
					U32 buttonCount, LuaResource *resourcePointer) const;
		virtual void CancelNativeAlert(NativeAlertRef alertReference, S32 buttonIndex) const;
		virtual bool CanShowPopup(const char *name) const;
		virtual bool ShowPopup(lua_State *L, const char *name, int optionsIndex) const;
		virtual bool HidePopup(const char *name) const;
		virtual PlatformDisplayObject* CreateNativeWebView(const Rect& bounds) const;
		virtual PlatformDisplayObject* CreateNativeVideo(const Rect& bounds) const;
		virtual void* CreateAndScheduleNotification(lua_State *L, int index) const;
		virtual void ReleaseNotification(void *notificationId) const;
		virtual void CancelNotification(void *notificationId) const;
		virtual void SetNativeProperty(lua_State *L, const char *key, int valueIndex) const;
		virtual int PushNativeProperty(lua_State *L, const char *key) const;
		virtual int PushSystemInfo(lua_State *L, const char *key) const;
		virtual void RuntimeErrorNotification(const char *errorType, const char *message, const char *stacktrace) const;
		virtual void RaiseError(MPlatform::Error e, const char* reason) const;
		virtual void PathForFile(const char* filename, MPlatform::Directory baseDir, U32 flags, String & result) const;
		virtual bool FileExists(const char * filename) const;
		virtual bool ValidateAssetFile(const char *assetFilename, const int assetSize) const;
		virtual int SetSync(lua_State* L) const;
		virtual int GetSync(lua_State* L) const;
		virtual void BeginRuntime(const Runtime& runtime) const;
		virtual void EndRuntime(const Runtime& runtime) const;
		virtual PlatformExitCallback* GetExitCallback();
		virtual bool RequestSystem(lua_State *L, const char *actionName, int optionsIndex) const;
#ifdef Rtt_AUTHORING_SIMULATOR
		virtual void SetCursorForRect(const char *cursorName, int x, int y, int width, int height) const;
#endif

		virtual void Suspend( ) const;
		virtual void Resume( ) const;
		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const;

	private:
		typedef std::shared_ptr<Interop::Storage::MStoredPreferences> SharedMStoredPreferencesPointer;

		void CopyAppNameTo(WinString& destinationString) const;
		int GetEncoderClsid(const WCHAR *format, CLSID *pClsid) const;
		Rtt::ValueResult<SharedMStoredPreferencesPointer> GetStoredPreferencesByCategoryName(const char* categoryName) const;

		Interop::RuntimeEnvironment& fEnvironment;
		WinDevice fDevice;
		WinCrypto fCrypto;
		WinExitCallback fExitCallback;
		mutable bool fIsIdleTimerEnabled;
};

}	// namespace Rtt
