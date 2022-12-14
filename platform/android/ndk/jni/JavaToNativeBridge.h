//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _H_JavaToNativeBridge_
#define _H_JavaToNativeBridge_

#include "Core/Rtt_Build.h"
#include "Core/Rtt_Time.h"
#include "jniUtils.h"

// ----------------------------------------------------------------------------

class AndroidGLView;
class NativeToJavaBridge;

namespace Rtt {
	class AndroidPlatform;
	class AndroidRuntimeDelegate;
	class Runtime;
	class TouchEvent;
};

// ----------------------------------------------------------------------------

class JavaToNativeBridge
{
	public:
		JavaToNativeBridge();
		virtual ~JavaToNativeBridge();

	public:
		void Init(
				JNIEnv * env, jstring package, jstring documentsDir, jstring applicationSupportDir, jstring temporaryDir, jstring cachesDir,
				jstring systemCachesDir, int width, int height, bool isCoronaKit );
		void UnloadResources();
		void ReloadResources();
		void Deinit();
		size_t GetMaxTextureSize();
		int GetHorizontalMarginInPixels();
		int GetVerticalMarginInPixels();
		int GetContentWidthInPixels();
		int GetContentHeightInPixels();
		bool CopyBitmapInfo(
					JNIEnv *env, jlong nativeImageMemoryAddress, int width, int height,
					float downscaleFactor);
		bool CopyBitmap(
					JNIEnv *env, jlong nativeImageMemoryAddress, jobject bitmap,
					float downscaleFactor, bool convertToGrayscale);
		void UpdateInputDevice(
					JNIEnv * env, jint coronaDeviceId, jint androidDeviceId, jint deviceTypeId,
					jstring permanentStringId, jstring productName, jstring displayName,
					jboolean canVibrate, jint playerNumber, jint connectionStateId);
		void ClearInputDeviceAxes(int coronaDeviceId);
		void AddInputDeviceAxis(
					int coronaDeviceId, int axisTypeId, float minValue, float maxValue,
					float accuracy, bool isAbsolute);
		void InputDeviceStatusEvent(int coronaDeviceId, bool hasConnectionStatusChanged, bool wasReconfigured);
		void UseDefaultLuaErrorHandler();
		void UseJavaLuaErrorHandler();
		void Render();
		void Pause();
		void Resume();
		void DispatchEventInLua();
		void ApplicationOpenEvent();
		void TouchEvent(int x, int y, int xStart, int yStart, int touchType, long timestamp, int touchId );
		void MouseEvent(
				int x, int y, int scrollX, int scrollY, long timestamp,
				bool isPrimaryButtonDown, bool isSecondaryButtonDown, bool isMiddleButtonDown);
		bool KeyEvent(
				int coronaDeviceId, int phase, const char *keyName, int keyCode,
				bool isShiftDown, bool isAltDown, bool isCtrlDown, bool isCommandDown);
		void AxisEvent(int coronaDeviceId, int axisIndex, float rawValue);
		void AccelerometerEvent(double x, double y, double z, double deltaTime);
		void GyroscopeEvent(double x, double y, double z, double deltaTime);
		void ResizeEvent();
		void AlertCallback(int which, bool cancelled);
		void MultitouchEventBegin();
		void MultitouchEventAdd( JNIEnv * env, int x, int y, int xStart, int yStart, int touchType, long timestamp, int touchId );
		void MultitouchEventEnd();
		void WebViewShouldLoadUrl( JNIEnv * env, int id, jstring url, int sourceType );
		void WebViewFinishedLoadUrl( JNIEnv * env, int id, jstring url );
		void WebViewDidFailLoadUrl( JNIEnv * env, int id, jstring url, jstring msg, int code );
		void WebViewHistoryUpdated( JNIEnv * env, int id, jboolean canGoBack, jboolean canGoForward );
		void WebViewClosed( JNIEnv * env, int id );
		void AdsRequestEvent(bool isError);
		void MemoryWarningEvent();
		void PopupClosedEvent(JNIEnv *env, jstring popupName, jboolean wasCanceled);
		const char* GetBuildId();

	public:
		Rtt::Runtime* GetRuntime() { return fRuntime; }
		const Rtt::Runtime* GetRuntime() const { return fRuntime; }

		void SetCoronaRuntime(jobject runtime);
		jobject GetCoronaRuntime();

	private:
		AndroidGLView *fView;
		Rtt::AndroidPlatform *fPlatform;
		Rtt::Runtime *fRuntime;
		Rtt::AndroidRuntimeDelegate *fRuntimeDelegate;
		bool fIsResourcesLoaded;
		double fGravityAccel[3];
		double fInstantAccel[3];
		Rtt_AbsoluteTime fPreviousShakeTime;
		Rtt::TouchEvent *fMultitouchEventBuffer;
		int fMultitouchEventCount;
		jobject fCoronaRuntime;
		NativeToJavaBridge *fNativeToJavaBridge;

		enum
		{
			/// Maximum number of touch events allowed for the same multitouch event.
			/// This represents the number of pointers/fingers on the screen at the same time.
			kMaxMultitouchPointerEvents = 16
		};
};

// ----------------------------------------------------------------------------

#endif // _H_JavaToNativeBridge_
