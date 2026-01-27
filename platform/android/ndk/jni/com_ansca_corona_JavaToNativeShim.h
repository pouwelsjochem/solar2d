//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include <jni.h>
/* Header for class com_ansca_corona_JavaToNativeShim */

#ifndef _Included_com_ansca_corona_JavaToNativeShim
#define _Included_com_ansca_corona_JavaToNativeShim
#ifdef __cplusplus
extern "C" {
#endif
#undef com_ansca_corona_JavaToNativeShim_TouchEventPhaseBegan
#define com_ansca_corona_JavaToNativeShim_TouchEventPhaseBegan 0L
#undef com_ansca_corona_JavaToNativeShim_TouchEventPhaseMoved
#define com_ansca_corona_JavaToNativeShim_TouchEventPhaseMoved 1L
#undef com_ansca_corona_JavaToNativeShim_TouchEventPhaseEnded
#define com_ansca_corona_JavaToNativeShim_TouchEventPhaseEnded 2L
#undef com_ansca_corona_JavaToNativeShim_TouchEventPhaseCancelled
#define com_ansca_corona_JavaToNativeShim_TouchEventPhaseCancelled 3L
#undef com_ansca_corona_JavaToNativeShim_EventTypeUnknown
#define com_ansca_corona_JavaToNativeShim_EventTypeUnknown -1L
#undef com_ansca_corona_JavaToNativeShim_EventTypeAccelerometer
#define com_ansca_corona_JavaToNativeShim_EventTypeAccelerometer 1L
#undef com_ansca_corona_JavaToNativeShim_EventTypeGyroscope
#define com_ansca_corona_JavaToNativeShim_EventTypeGyroscope 2L
#undef com_ansca_corona_JavaToNativeShim_EventTypeMultitouch
#define com_ansca_corona_JavaToNativeShim_EventTypeMultitouch 5L
#undef com_ansca_corona_JavaToNativeShim_EventTypeNumTypes
#define com_ansca_corona_JavaToNativeShim_EventTypeNumTypes 6L
/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetVersion
  (JNIEnv *, jclass);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativePause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativePause
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeResume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeResume
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeDispatchEventInLua
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeDispatchEventInLua
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeApplicationOpenEvent
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeApplicationOpenEvent
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeInit
 * Signature: ()V
 */
JNIEXPORT jlong JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeInit
  (JNIEnv *, jclass, jobject);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetKeyNameFromKeyCode
 * Signature: (I)Ljava/lang/String;Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetKeyNameFromKeyCode
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetMaxTextureSize
 * Signature: ()I;
 */
JNIEXPORT jint JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetMaxTextureSize
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetHorizontalMarginInPixels
 * Signature: ()I;
 */
JNIEXPORT jint JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetHorizontalMarginInPixels
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetVerticalMarginInPixels
 * Signature: ()I;
 */
JNIEXPORT jint JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetVerticalMarginInPixels
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetContentWidthInPixels
 * Signature: ()I;
 */
JNIEXPORT jint JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetContentWidthInPixels
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGetContentHeightInPixels
 * Signature: ()I;
 */
JNIEXPORT jint JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetContentHeightInPixels
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeResize
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;III)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeResize
  (JNIEnv *, jclass, jlong, jstring, jstring, jstring, jstring, jstring, jstring, jint, jint, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeRender
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeRender
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeUnloadResources
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeUnloadResources
(JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeDone
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeDone
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeCopyBitmapInfo
 * Signature: (JIIFI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeCopyBitmapInfo
  (JNIEnv *, jclass, jlong, jlong nativeImageMemoryAddress, jint width, jint height,
   jfloat downscaleFactor);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeCopyBitmap
 * Signature: (JLandroid.graphics.Bitmap;FIZ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeCopyBitmap
  (JNIEnv *, jclass, jlong, jlong nativeImageMemoryAddress, jobject bitmap, jfloat downscaleFactor,
   jboolean convertToGrayscale);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeSetZipFileEntryInfo
 * Signature: (JLjava/lang/String;Ljava/lang/String;JJZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeSetZipFileEntryInfo
  (JNIEnv *, jclass, jlong, jstring, jstring, jlong, jlong, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeUpdateInputDevice
 * Signature: (IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ZIIII)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeUpdateInputDevice
  (JNIEnv *, jclass, jlong, jint, jint, jint, jstring, jstring, jstring, jboolean, jint, jint, jint, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeClearInputDeviceAxes
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeClearInputDeviceAxes
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeAddInputDeviceAxis
 * Signature: (IIFFFZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeAddInputDeviceAxis
  (JNIEnv *, jclass, jlong, jint, jint, jfloat, jfloat, jfloat, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeInputDeviceStatusEvent
 * Signature: (IZZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeInputDeviceStatusEvent
  (JNIEnv *, jclass, jlong, jint, jboolean, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeTouchEvent
 * Signature: (IIIIIJI)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeTouchEvent
  (JNIEnv *, jclass, jlong, jint, jint, jint, jint, jint, jlong, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeMouseEvent
 * Signature: (IIIIJZZZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeMouseEvent
  (JNIEnv *, jclass, jlong, jint, jint, jint, jint, jlong, jboolean, jboolean, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeKeyEvent
 * Signature: (IIIZZZZ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeKeyEvent
  (JNIEnv *, jclass, jlong, jint, jint, jint, jboolean, jboolean, jboolean, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeAxisEvent
 * Signature: (IIF)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeAxisEvent
  (JNIEnv *, jclass, jlong, jint, jint, jfloat);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeAccelerometerEvent
 * Signature: (DDDD)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeAccelerometerEvent
  (JNIEnv *, jclass, jlong, jdouble, jdouble, jdouble, jdouble);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeGyroscopeEvent
 * Signature: (DDDD)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGyroscopeEvent
(JNIEnv *, jclass, jlong, jdouble, jdouble, jdouble, jdouble);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeResizeEvent
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeResizeEvent
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeAlertCallback
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeAlertCallback
  (JNIEnv *, jclass, jlong, jint, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeTextEvent
 * Signature: (IZZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeTextEvent
  (JNIEnv *, jclass, jlong, jint, jboolean, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeTextEditingEvent
 * Signature: (IIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeTextEditingEvent
  (JNIEnv *, jclass, jlong, jint, jint, jint, jstring, jstring, jstring);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeMultitouchEventBegin
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventBegin
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeMultitouchEventAdd
 * Signature: (IIIIILI)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventAdd
  (JNIEnv *, jclass, jlong, jint, jint, jint, jint, jint, jlong, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeMultitouchEventEnd
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventEnd
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeWebViewShouldLoadUrl
 * Signature: (ILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeWebViewShouldLoadUrl
  (JNIEnv *, jclass, jlong, jint, jstring, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeWebViewFinishedLoadUrl
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeWebViewFinishedLoadUrl
  (JNIEnv *, jclass, jlong, jint, jstring);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeWebViewDidFailLoadUrl
 * Signature: (ILjava/lang/String;Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeWebViewDidFailLoadUrl
  (JNIEnv *, jclass, jlong, jint, jstring, jstring, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeWebViewHistoryUpdated
 * Signature: (IZZ)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeWebViewHistoryUpdated
  (JNIEnv *, jclass, jlong, jint, jboolean, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeWebViewShouldLoadUrl
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeWebViewClosed
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeAdsRequestEvent
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeAdsRequestEvent
  (JNIEnv *, jclass, jlong, jboolean);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativeMemoryWarningEvent
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeMemoryWarningEvent
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_ansca_corona_JavaToNativeShim
 * Method:    nativePopupClosedEvent
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_ansca_corona_JavaToNativeShim_nativePopupClosedEvent
  (JNIEnv *, jclass, jlong, jstring, jboolean);
JNIEXPORT jobject JNICALL Java_com_ansca_corona_JavaToNativeShim_nativeGetCoronaRuntime
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif
