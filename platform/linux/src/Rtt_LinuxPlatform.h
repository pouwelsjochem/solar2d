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
#include "Rtt_PlatformTimer.h"
#include "Rtt_PlatformSimulator.h"

namespace Rtt
{
	class PlatformBitmap;
	class PlatformSurface;
	class PlatformTimer;
	class RenderingStream;
	class LinuxScreenSurface;

	class LinuxTimer : public PlatformTimer
	{
	public:
		LinuxTimer(MCallback &callback) : PlatformTimer(callback) {};

		virtual void Start() {};
		virtual void Stop() {};
		virtual void SetInterval(U32 milliseconds) {};
		virtual bool IsRunning() const { return true; };
	};

	class LinuxPlatform : public MPlatform
	{
	Rtt_CLASS_NO_COPIES(LinuxPlatform) public :
		typedef LinuxPlatform Self;

	public:
		LinuxPlatform(const char *resourceDir, const char *documentsDir, const char *temporaryDir,
		              const char *cachesDir, const char *systemCachesDir, const char *skinDir, const char *installDir);
		virtual ~LinuxPlatform();

	public:
		virtual MPlatformDevice &GetDevice() const;
		virtual PlatformSurface *CreateScreenSurface() const;
		virtual PlatformSurface *CreateOffscreenSurface(const PlatformSurface &parent) const;

	public:
		virtual Rtt_Allocator &GetAllocator() const;
		virtual RenderingStream *CreateRenderingStream(bool antialias) const;
		virtual PlatformTimer *CreateTimerWithCallback(MCallback &callback) const;
		virtual PlatformBitmap *CreateBitmap(const char *filename, bool convertToGrayscale) const;
		virtual void HttpPost(const char *url, const char *key, const char *value) const;
		virtual const MCrypto &GetCrypto() const;
		virtual void GetPreference(Category category, Rtt::String *value) const;
		virtual void SetActivityIndicator(bool visible) const;
		virtual bool CanShowPopup(const char *name) const;
		virtual bool ShowPopup(lua_State *L, const char *name, int optionsIndex) const;
		virtual bool HidePopup(const char *name) const;
		virtual PlatformDisplayObject *CreateNativeWebView(const Rect &bounds) const;
		PlatformDisplayObject *GetNativeDisplayObjectById(const int objectId) const;
		virtual PlatformFBConnect *GetFBConnect() const;

	public:
		virtual void RaiseError(MPlatform::Error e, const char *reason) const;
		virtual void PathForFile(const char *filename, MPlatform::Directory baseDir, U32 flags, String &result) const;
		virtual void SetProjectResourceDirectory(const char *filename);
		virtual void SetSkinResourceDirectory(const char *filename);
		virtual bool FileExists(const char *filename) const;
		virtual int SetSync(lua_State *L) const;
		virtual int GetSync(lua_State *L) const;

	protected:
		void PathForFile(const char *filename, const char *baseDir, String &result) const;

	public:
		virtual void BeginRuntime(const Runtime &runtime) const;
		virtual void EndRuntime(const Runtime &runtime) const;
		virtual PlatformExitCallback *GetExitCallback();
		virtual bool RequestSystem(lua_State *L, const char *actionName, int optionsIndex) const;
		virtual void RuntimeErrorNotification(const char *errorType, const char *message, const char *stacktrace) const;

#ifdef Rtt_AUTHORING_SIMULATOR
		virtual void SetCursorForRect(const char* cursorName, int x, int y, int width, int height) const override;
#endif

	protected:
		char *CopyString(const char *src, bool useAllocator = true) const;

	public:
		virtual bool SaveBitmap(PlatformBitmap *bitmap, const char *filePath, float jpegQuality) const;
		virtual bool OpenURL(const char *url) const;
		virtual int CanOpenURL(const char *url) const;
		virtual PlatformStoreProvider *GetStoreProvider(const ResourceHandle<lua_State> &handle) const;
		virtual void SetIdleTimer(bool enabled) const;
		virtual bool GetIdleTimer() const;

		virtual NativeAlertRef ShowNativeAlert(
		    const char *title,
		    const char *msg,
		    const char **buttonLabels,
		    U32 numButtons,
		    LuaResource *resource) const;
		virtual void CancelNativeAlert(NativeAlertRef alert, S32 index) const;

	public:
		virtual void *CreateAndScheduleNotification(lua_State *L, int index) const;
		virtual void ReleaseNotification(void *notificationId) const;
		virtual void CancelNotification(void *notificationId) const;

	public:
		virtual void SetNativeProperty(lua_State *L, const char *key, int valueIndex) const;
		virtual int PushNativeProperty(lua_State *L, const char *key) const;
		virtual int PushSystemInfo(lua_State *L, const char *key) const;

	protected:
		void NetworkBaseRequest(lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex, const char *path) const;

	public:
		virtual void NetworkRequest(lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex) const;
		virtual void NetworkDownload(lua_State *L, const char *url, const char *method, LuaResource *listener, int paramsIndex, const char *filename, MPlatform::Directory baseDir) const;
		virtual PlatformReachability *NewReachability(const ResourceHandle<lua_State> &handle, PlatformReachability::PlatformReachabilityType type, const char *address) const;
		virtual bool SupportsNetworkStatus() const;
		const char *getInstallDir() const { return fInstallDir.GetString(); }
		void setWindow(void *ctx);
		bool fShowRuntimeErrors;

	protected:
		Rtt_Allocator *fAllocator;
		mutable bool isMouseCursorVisible;

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
		mutable PlatformStoreProvider *fStoreProvider;
		mutable PlatformFBConnect *fFBConnect;
		mutable LinuxScreenSurface *fScreenSurface;

	public:
		virtual void GetSafeAreaInsetsPixels(Rtt_Real &top, Rtt_Real &left, Rtt_Real &bottom, Rtt_Real &right) const override;
		virtual Preference::ReadValueResult GetPreference(const char *categoryName, const char *keyName) const override;
		virtual OperationResult SetPreferences(const char *categoryName, const PreferenceCollection &collection) const override;
		virtual OperationResult DeletePreferences(const char *categoryName, const char **keyNameArray, U32 keyNameCount) const override;
		virtual void Suspend() const override;
		virtual void Resume() const override;
	};

	class LinuxGUIPlatform : public LinuxPlatform
	{
	public:
		typedef LinuxPlatform Super;

	public:
		LinuxGUIPlatform(PlatformSimulator &simulator);

	public:
		virtual MPlatformDevice &GetDevice() const;
		virtual PlatformSurface *CreateScreenSurface() const;

	public:
		virtual bool RequestSystem(lua_State *L, const char *actionName, int optionsIndex) const;

	public:
		void SetAdaptiveWidth(S32 newValue) { fAdaptiveWidth = newValue; }
		void SetAdaptiveHeight(S32 newValue) { fAdaptiveHeight = newValue; }

	private:
		LinuxDevice fMacDevice;
		S32 fAdaptiveWidth;
		S32 fAdaptiveHeight;
	};

}; // namespace Rtt
