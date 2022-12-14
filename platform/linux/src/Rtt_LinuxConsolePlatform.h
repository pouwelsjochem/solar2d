//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Rtt_LinuxDevice.h"
#include "Rtt_MPlatform.h"
#include "Rtt_LinuxCrypto.h"
#include "Core/Rtt_String.h"
#include "Rtt_PlatformSimulator.h"

namespace Rtt
{
	class LinuxConsolePlatform : public MPlatform
	{
	Rtt_CLASS_NO_COPIES(LinuxConsolePlatform) public :
		typedef LinuxConsolePlatform Self;

	public:
		LinuxConsolePlatform(const char *resourceDir, const char *documentsDir, const char *temporaryDir,
		                     const char *cachesDir, const char *systemCachesDir, const char *skinDir, const char *installDir);
		virtual ~LinuxConsolePlatform();

	public:
		/*
		virtual void RuntimeErrorNotification(const char *errorType, const char *message, const char *stacktrace) const;
		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const override;
		virtual Rtt_Allocator &GetAllocator() const;
		virtual MPlatformDevice &GetDevice() const;
		virtual const MCrypto &GetCrypto() const;
		virtual bool FileExists(const char *filename) const;
		virtual void PathForFile(const char *filename, MPlatform::Directory baseDir, U32 flags, String &result) const;
		virtual Preference::ReadValueResult GetPreference(const char *categoryName, const char *keyName) const override;
		virtual void GetPreference(Category category, Rtt::String *value) const;
		virtual OperationResult SetPreferences(const char *categoryName, const PreferenceCollection &collection) const override;
		virtual OperationResult DeletePreferences(const char *categoryName, const char **keyNameArray, U32 keyNameCount) const override;
		virtual int PushSystemInfo(lua_State *L, const char *key) const;
		*/

		virtual int PushSystemInfo(lua_State *L, const char *key) const;

		virtual Rtt_Allocator& GetAllocator() const;
		virtual MPlatformDevice& GetDevice() const;
		virtual RenderingStream* CreateRenderingStream(bool antialias) const { return NULL; };
		virtual PlatformSurface* CreateScreenSurface() const { return NULL; };
		virtual PlatformSurface* CreateOffscreenSurface(const PlatformSurface& parent) const { return NULL; };
		virtual PlatformTimer* CreateTimerWithCallback(MCallback& callback) const { return NULL; };
		virtual PlatformBitmap* CreateBitmap(const char* filename, bool convertToGrayscale) const { return NULL; };
		virtual void SaveBitmap(PlatformBitmap* bitmap, Rtt::Data<const char> & pngBytes) const { return false; };
		virtual const MCrypto& GetCrypto() const;
		virtual void GetPreference(Category category, Rtt::String * value) const;
		virtual Preference::ReadValueResult GetPreference(const char* categoryName, const char* keyName) const;
		virtual OperationResult SetPreferences(const char* categoryName, const PreferenceCollection& collection) const;
		virtual OperationResult DeletePreferences(const char* categoryName, const char** keyNameArray, U32 keyNameCount) const;
		virtual bool OpenURL(const char* url) const { return false; };
		virtual int CanOpenURL(const char* url) const { return 0; };
		virtual void HttpPost(const char* url, const char* key, const char* value) const { return; };
		virtual NativeAlertRef ShowNativeAlert(
		    const char *title, const char *message, const char **buttonLabels,
		    U32 buttonCount, LuaResource *resourcePointer) const { return nullptr; };
		virtual void CancelNativeAlert(NativeAlertRef alertReference, S32 buttonIndex) const { return; };
		virtual bool CanShowPopup(const char *name) const { return false; };
		virtual bool ShowPopup(lua_State *L, const char *name, int optionsIndex) const { return false; };
		virtual bool HidePopup(const char *name) const { return false; };
		virtual PlatformDisplayObject* CreateNativeWebView(const Rect& bounds) const { return nullptr; };
		virtual void* CreateAndScheduleNotification(lua_State *L, int index) const { return nullptr; };
		virtual void ReleaseNotification(void *notificationId) const { return; };
		virtual void CancelNotification(void *notificationId) const { return; };
		virtual void SetNativeProperty(lua_State *L, const char *key, int valueIndex) const { return; };
		virtual int PushNativeProperty(lua_State *L, const char *key) const { return 0; };
		virtual void RuntimeErrorNotification(const char *errorType, const char *message, const char *stacktrace) const;
		virtual void SetProjectResourceDirectory(const char* path) { return; };
		virtual void SetSkinResourceDirectory(const char* path) { return; };
		virtual void RaiseError(MPlatform::Error e, const char* reason) const { return; };
		virtual void PathForFile(const char* filename, MPlatform::Directory baseDir, U32 flags, String & result) const;
		virtual bool FileExists(const char * filename) const;
		virtual int SetSync(lua_State* L) const { return 0; };
		virtual int GetSync(lua_State* L) const { return 0; };
		virtual void BeginRuntime(const Runtime& runtime) const { return; };
		virtual void EndRuntime(const Runtime& runtime) const { return; };
		virtual PlatformExitCallback* GetExitCallback() { return nullptr; };
		virtual bool RequestSystem(lua_State *L, const char *actionName, int optionsIndex) const { return false; };
		virtual void Suspend() const override { Rtt_ASSERT_MSG( 0, "Code should NOT be reached" ); }
		virtual void Resume() const override { Rtt_ASSERT_MSG( 0, "Code should NOT be reached" ); }
		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const override;
		virtual int RunSystemCommand(std::string command) const;
		virtual const std::string GetDirectoryPath() const;

	protected:
		void PathForFile(const char *filename, const char *baseDir, String &result) const;

	protected:
		Rtt_Allocator *fAllocator;

	private:
		LinuxDevice fDevice;
		String fResourceDir;
		String fDocumentsDir;
		String fTemporaryDir;
		String fCachesDir;
		String fSystemCachesDir;
		String fInstallDir;
		String fSkinDir;
		LinuxCrypto fCrypto;
	};
}; // namespace Rtt
