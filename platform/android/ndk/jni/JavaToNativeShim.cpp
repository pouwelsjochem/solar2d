//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "JavaToNativeBridge.h"
#include "AndroidKeyServices.h"
#include "AndroidZipFileEntry.h"

// ----------------------------------------------------------------------------

extern "C"
{
	
// ----------------------------------------------------------------------------
	
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>
#include "com_ansca_corona_JavaToNativeShim.h"

// Generate JNI signatures with:
//	javah -classpath bin5 -o ../corona/jni/com_ansca_corona_JavaToNativeShim.h com.ansca.corona.JavaToNativeShim

static const char * kCoronaVersion = "Corona/Android/1.0";

extern void debugPrint(const char *msg);
extern int importGLInit();
extern int importGLDeinit();

jint JNI_OnLoad( JavaVM *vm, void * reserved )
{
	JNIEnv *env;

	if ( (vm)->GetEnv( (void **) &env, JNI_VERSION_1_4) != JNI_OK )
		return -1;

	// TODO: Need to register methods with RegisterNatives
	// See http://android.git.kernel.org/?p=platform/dalvik.git;a=blob_plain;f=docs/jni-tips.html;hb=HEAD#Native_Libraries

	return JNI_VERSION_1_4;
}

/**
 * Gets the instance of JavaToNativeBridge at the specified memory address.
 * @param bridgeAddress The memory address location for the native to java bridge.
 * @return The bridge object at the specfied address.
 */
JavaToNativeBridge*
JavaToNativeBridgeFromMemoryAddress(jlong bridgeAddress)
{
	JavaToNativeBridge* bridge;
	bridge = (JavaToNativeBridge*)bridgeAddress;
	return bridge;
}

JNIEXPORT jstring JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetVersion(JNIEnv *env, jclass c)
{
    return (env)->NewStringUTF(kCoronaVersion);
}

JNIEXPORT jstring JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetBuildId(JNIEnv *env, jclass c, jlong bridgeAddress)
{
    debugPrint( "> JavaToNativeShim.nativeGetBuildId");
	
	jstring ret = NULL;
	const char * buildId = JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetBuildId();
	if(buildId != NULL)
		ret = (env)->NewStringUTF(buildId);
    debugPrint( "< JavaToNativeShim.nativeGetBuildId");

    return ret;    
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativePause(JNIEnv * env, jclass c, jlong bridgeAddress)
{
    debugPrint( "> JavaToNativeShim.nativePause");

	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->Pause();

    debugPrint( "< JavaToNativeShim.nativePause");
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeResume(JNIEnv * env, jclass c, jlong bridgeAddress)
{
    debugPrint( "> JavaToNativeShim.nativeResume");
    JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->Resume();
    debugPrint( "< JavaToNativeShim.nativeResume");
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeDispatchEventInLua(JNIEnv * env, jclass c, jlong bridgeAddress)
{
    debugPrint( "> JavaToNativeShim.nativeDispatchEventInLua");
    JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->DispatchEventInLua();
    debugPrint( "< JavaToNativeShim.nativeDispatchEventInLua");
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeApplicationOpenEvent(JNIEnv * env, jclass c, jlong bridgeAddress)
{
    debugPrint( "> JavaToNativeShim.nativeApplicationOpenEvent");
    JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->ApplicationOpenEvent();
    debugPrint( "< JavaToNativeShim.nativeApplicationOpenEvent");
}

/* Call to initialize the graphics state */
JNIEXPORT jlong JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeInit( JNIEnv * env, jclass c, jobject runtime )
{
	debugPrint( "> JavaToNativeShim.nativeInit");

    importGLInit();

    JavaToNativeBridge *javaToNativeBridge = new JavaToNativeBridge();
    javaToNativeBridge->SetCoronaRuntime(env->NewGlobalRef(runtime));
	debugPrint( "< JavaToNativeShim.nativeInit");
	return (jlong)(uintptr_t)javaToNativeBridge;
}

JNIEXPORT jstring JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetKeyNameFromKeyCode( JNIEnv * env, jclass c, jint keyCode )
{
    return (env)->NewStringUTF(AndroidKeyServices::GetNameFromKeyCode((int)keyCode));
}

JNIEXPORT jint JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetMaxTextureSize( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	return (jint)JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetMaxTextureSize();
}

JNIEXPORT jint JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetHorizontalMarginInPixels( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	return (jint)JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetHorizontalMarginInPixels();
}

JNIEXPORT jint JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetVerticalMarginInPixels( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	return (jint)JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetVerticalMarginInPixels();
}

JNIEXPORT jint JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetContentWidthInPixels( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	return (jint)JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetContentWidthInPixels();
}

JNIEXPORT jint JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeGetContentHeightInPixels( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	return (jint)JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetContentHeightInPixels();
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeUseDefaultLuaErrorHandler( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->UseDefaultLuaErrorHandler();
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeUseJavaLuaErrorHandler( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->UseJavaLuaErrorHandler();
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeUnloadResources( JNIEnv * env, jclass c, jlong bridgeAddress )
{
	debugPrint( "> JavaToNativeShim.nativeUnloadResources");
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->UnloadResources();
	debugPrint( "< JavaToNativeShim.nativeUnloadResources");
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeDone( JNIEnv * env, jclass c, jlong bridgeAddress )
{
    debugPrint( "> JavaToNativeShim.nativeDone");

    JavaToNativeBridge *bridge = JavaToNativeBridgeFromMemoryAddress(bridgeAddress);
    
	bridge->Deinit();
    importGLDeinit();

    // When the bridge deinits it tries to access the java side runtime still so it can't be delete before that.
    env->DeleteGlobalRef(bridge->GetCoronaRuntime());

    delete bridge;

    debugPrint( "< JavaToNativeShim.nativeDone");
}

JNIEXPORT jboolean JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeCopyBitmapInfo(
	JNIEnv * env, jclass c, jlong bridgeAddress, jlong nativeImageMemoryAddress, jint width, jint height,
	jfloat downscaleFactor, jint rotationInDegrees)
{
	bool wasCopied = JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->CopyBitmapInfo(
							env, nativeImageMemoryAddress, (int)width, (int)height,
							(float)downscaleFactor, (int)rotationInDegrees);
	return wasCopied ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeCopyBitmap(
	JNIEnv * env, jclass c, jlong bridgeAddress, jlong nativeImageMemoryAddress, jobject bitmap,
	jfloat downscaleFactor, jint rotationInDegrees, jboolean convertToGrayscale)
{
	bool wasCopied = JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->CopyBitmap(
							env, nativeImageMemoryAddress, bitmap, (float)downscaleFactor,
							(int)rotationInDegrees, convertToGrayscale ? true : false);
	return wasCopied ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL 
Java_com_ansca_corona_JavaToNativeShim_nativeSetZipFileEntryInfo(
	JNIEnv * env, jclass c, jlong zipFileEntryMemoryAddress, jstring packageFilePath, jstring entryName,
	jlong byteOffsetInPackage, jlong byteCountInPackage, jboolean isCompressed)
{
	if (zipFileEntryMemoryAddress != 0)
	{
		jstringResult packageFilePathConverter(env, packageFilePath);
		jstringResult entryNameConverter(env, entryName);

		AndroidZipFileEntry *zipFileEntryPointer = (AndroidZipFileEntry*)zipFileEntryMemoryAddress;
		zipFileEntryPointer->SetPackageFilePath(packageFilePathConverter.getUTF8());
		zipFileEntryPointer->SetEntryName(entryNameConverter.getUTF8());
		zipFileEntryPointer->SetByteOffsetInPackage((long)byteOffsetInPackage);
		zipFileEntryPointer->SetByteCountInPackage((long)byteCountInPackage);
		zipFileEntryPointer->SetIsCompressed(isCompressed ? true : false);
	}
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeUpdateInputDevice(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId, jint androidDeviceId, jint deviceTypeId,
	jstring permanentStringId, jstring productName, jstring displayName,
	jboolean canVibrate, jint playerNumber, jint connectionStateId)
{
	// debugPrint( "> JavaToNativeShim.nativeUpdateInputDevice");
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->UpdateInputDevice(
			env, coronaDeviceId, androidDeviceId, deviceTypeId, permanentStringId,
			productName, displayName, canVibrate, playerNumber, connectionStateId);
	// debugPrint( "< JavaToNativeShim.nativeUpdateInputDevice");
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeClearInputDeviceAxes(JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId)
{
	// debugPrint( "> JavaToNativeShim.nativeClearInputDeviceAxes");
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->ClearInputDeviceAxes((int)coronaDeviceId);
	// debugPrint( "< JavaToNativeShim.nativeClearInputDeviceAxes");
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeAddInputDeviceAxis(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId, jint axisTypeId, jfloat minValue, jfloat maxValue,
	jfloat accuracy, jboolean isAbsolute)
{
	// debugPrint( "> JavaToNativeShim.nativeAddInputDeviceAxis");
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->AddInputDeviceAxis(
			(int)coronaDeviceId, (int)axisTypeId, (float)minValue, (float)maxValue,
			(float)accuracy, (bool)isAbsolute);
	// debugPrint( "< JavaToNativeShim.nativeAddInputDeviceAxis");
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeInputDeviceStatusEvent(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId, jboolean hasConnectionStatusChanged, jboolean wasReconfigured)
{
	debugPrint( "> JavaToNativeShim.nativeInputDeviceStatusEvent");
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->InputDeviceStatusEvent(
			(int)coronaDeviceId, (bool)hasConnectionStatusChanged, (bool)wasReconfigured);
	debugPrint( "< JavaToNativeShim.nativeInputDeviceStatusEvent");
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeResize(
	JNIEnv * env, jclass c, jlong bridgeAddress, jstring package, jstring documentsDir, jstring applicationSupportDir,
	jstring temporaryDir, jstring cachesDir, jstring systemCachesDir, jstring expansionFileDir,
	jint w, jint h, jboolean isCoronaKit)
{
		JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->Init(
    		env, package, documentsDir, applicationSupportDir, temporaryDir, cachesDir, systemCachesDir,
    		expansionFileDir, w, h, isCoronaKit);
}

/* Call to render the next GL frame */
void
Java_com_ansca_corona_JavaToNativeShim_nativeRender( JNIEnv * env, jclass c, jlong bridgeAddress )
{

	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->Render();
//    debugPrint( "< JavaToNativeShim.nativeRender");
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeTouchEvent(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint x, jint y, jint xStart, jint yStart, jint touchType, jlong timestamp, jint touchId, jfloat pressure)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->TouchEvent( x, y, xStart, yStart, touchType, timestamp, touchId, pressure );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeMouseEvent(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint x, jint y, jint scrollX, jint scrollY, jlong timestamp,
	jboolean isPrimaryButtonDown, jboolean isSecondaryButtonDown, jboolean isMiddleButtonDown)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->MouseEvent(
			x, y, scrollX, scrollY, timestamp, isPrimaryButtonDown, isSecondaryButtonDown, isMiddleButtonDown);
}

JNIEXPORT jboolean JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeKeyEvent(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId, jint phase, jint keyCode,
	jboolean isShiftDown, jboolean isAltDown, jboolean isCtrlDown, jboolean isCommandDown)
{
	const char *keyName = AndroidKeyServices::GetNameFromKeyCode((int)keyCode);
	JavaToNativeBridge* bridge;
	bridge = (JavaToNativeBridge*)bridgeAddress;
	return bridge->KeyEvent(
				coronaDeviceId, phase, keyName, keyCode, isShiftDown, isAltDown, isCtrlDown, isCommandDown);
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeAxisEvent(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint coronaDeviceId, jint axisIndex, jfloat rawValue)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->AxisEvent((int)coronaDeviceId, (int)axisIndex, (float)rawValue);
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeAccelerometerEvent(JNIEnv * env, jclass c, jlong bridgeAddress, jdouble x, jdouble y, jdouble z, jdouble deltaTime)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->AccelerometerEvent( x, y, z, deltaTime );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeGyroscopeEvent(JNIEnv * env, jclass c, jlong bridgeAddress, jdouble x, jdouble y, jdouble z, jdouble deltaTime)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GyroscopeEvent( x, y, z, deltaTime );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeResizeEvent(JNIEnv * env, jclass c, jlong bridgeAddress)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->ResizeEvent();
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeAlertCallback(JNIEnv * env, jclass c, jlong bridgeAddress, jint which, jboolean cancelled)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_INFO, "Corona", "AlertCallback %d", which, cancelled);
#endif

	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->AlertCallback( which, cancelled );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeTextEvent(JNIEnv * env, jclass cd, jlong bridgeAddress, jint id, jboolean hasFocus, jboolean isDone )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->TextEvent( id, hasFocus, isDone );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeTextEditingEvent( JNIEnv *env, jclass cd, jlong bridgeAddress, jint id, jint startPos, jint numDeleted, jstring newCharacters, jstring oldString, jstring newString )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->TextEditingEvent( env, (int)id, (int)startPos, numDeleted, newCharacters, oldString, newString );
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventBegin(JNIEnv * env, jclass c, jlong bridgeAddress)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->MultitouchEventBegin();
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventAdd(
	JNIEnv * env, jclass c, jlong bridgeAddress, jint xLast, jint yLast, jint xStart, jint yStart,
	jint phaseType, jlong timestamp, jint touchId, jfloat pressure)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->MultitouchEventAdd(
			env, xLast, yLast, xStart, yStart, phaseType, timestamp, touchId, pressure);
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeMultitouchEventEnd(JNIEnv * env, jclass c, jlong bridgeAddress)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->MultitouchEventEnd();
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeAdsRequestEvent(JNIEnv * env, jclass cd, jlong bridgeAddress, jboolean isError )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->AdsRequestEvent(isError);
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeMemoryWarningEvent(JNIEnv * env, jclass cd, jlong bridgeAddress )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->MemoryWarningEvent();
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativePopupClosedEvent(JNIEnv * env, jclass cd, jlong bridgeAddress, jstring popupName, jboolean wasCanceled )
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->PopupClosedEvent(env, popupName, wasCanceled);
}

JNIEXPORT void JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeStoreTransactionEvent(
	JNIEnv * env, jclass cd, jlong bridgeAddress, jint state, jint errorType, jstring errorMessage, jstring productId, jstring signature,
	jstring receipt, jstring transactionId, jstring transactionTime,
	jstring originalReceipt, jstring originalTransactionId, jstring originalTransactionTime)
{
	JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->StoreTransactionEvent(
		env, state, errorType, errorMessage, productId, signature, receipt, transactionId, transactionTime,
		originalReceipt, originalTransactionId, originalTransactionTime);
}

JNIEXPORT jobject JNICALL
Java_com_ansca_corona_JavaToNativeShim_nativeGetCoronaRuntime(
	JNIEnv * env, jclass cd, jlong bridgeAddress)
{
	return JavaToNativeBridgeFromMemoryAddress(bridgeAddress)->GetCoronaRuntime();
}
// ----------------------------------------------------------------------------

} // extern "C"

// ----------------------------------------------------------------------------

