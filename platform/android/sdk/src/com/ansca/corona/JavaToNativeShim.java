//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;


/**
 * Wrapper for all native calls 
 * 
 * @author eherrman
 */
public class JavaToNativeShim {
	// Corresponds to MPlatformDevice::EventType
	public static final int EventTypeUnknown = -1;
	public static final int EventTypeAccelerometer = 0;
	public static final int EventTypeGyroscope = 1;
	public static final int EventTypeMultitouch = 2;
	public static final int EventTypeNumTypes = 3;

	private static native String nativeGetVersion();
    private static native String nativeGetBuildId(long bridgeAddress);
    private static native void nativePause(long bridgeAddress);
    private static native void nativeResume(long bridgeAddress);
    private static native void nativeDispatchEventInLua(long bridgeAddress);
    private static native void nativeApplicationOpenEvent(long bridgeAddress);
    private static native long nativeInit(CoronaRuntime runtime);
    private static native void nativeResize(
    			long bridgeAddress, String signature, String documentsDir, String applicationSupportDir, String temporaryDir, String cachesDir,
    			String systemCachesDir, String expansionFileDir, int w, int h, boolean isCoronaKit);
    private static native void nativeRender(long bridgeAddress);
    private static native String nativeGetKeyNameFromKeyCode(int keyCode);
    private static native int nativeGetMaxTextureSize(long bridgeAddress);
	private static native int nativeGetHorizontalMarginInPixels(long bridgeAddress);
	private static native int nativeGetVerticalMarginInPixels(long bridgeAddress);
	private static native int nativeGetContentWidthInPixels(long bridgeAddress);
	private static native int nativeGetContentHeightInPixels(long bridgeAddress);
	private static native void nativeUseDefaultLuaErrorHandler();
	private static native void nativeUseJavaLuaErrorHandler();
    private static native void nativeUnloadResources(long bridgeAddress);
    private static native void nativeDone(long bridgeAddress);
    private static native boolean nativeCopyBitmapInfo(
    			long bridgeAddress, long nativeImageMemoryAddress, int width, int height,
    			float downscaleFactor);
    private static native boolean nativeCopyBitmap(
    			long bridgeAddress, long nativeImageMemoryAddress, android.graphics.Bitmap bitmap,
    			float downscaleFactor, boolean convertToGrayscale);
    private static native void nativeSetZipFileEntryInfo(
    			long zipFileEntryMemoryAddress, String packageFilePath, String entryName,
    			long byteOffsetInPackage, long byteCountInPackage, boolean isCompressed);
    private static native void nativeUpdateInputDevice(
    			long bridgeAddress, int coronaDeviceId, int androidDeviceId, int deviceType, String permanentStringId,
    			String productName, String displayName, boolean canVibrate, int playerNumber, int connectionStateId);
    private static native void nativeClearInputDeviceAxes(long bridgeAddress, int coronaDeviceId);
    private static native void nativeAddInputDeviceAxis(
    			long bridgeAddress, int coronaDeviceId, int axisTypeId, float minValue, float maxValue,
    			float accuracy, boolean isAbsolute);
    private static native void nativeInputDeviceStatusEvent(
    			long bridgeAddress, int coronaDeviceId, boolean hasConnectionStateChanged, boolean wasReconfigured);
    private static native void nativeTouchEvent( long bridgeAddress, int x, int y, int xStart, int yStart, int type, long timestamp, int ide );
    private static native void nativeMouseEvent(
    			long bridgeAddress, int x, int y, int scrollX, int scrollY, long timestamp,
    			boolean isPrimaryButtonDown, boolean isSecondaryButtonDown, boolean isMiddleButtonDown);
    private static native boolean nativeKeyEvent(
    			long bridgeAddress, int coronaDeviceId, int phase, int keyCode,
    			boolean isShiftDown, boolean isAltDown, boolean isCtrlDown, boolean isCommandDown);
    private static native void nativeAxisEvent( long bridgeAddress, int coronaDeviceId, int axisIndex, float rawValue );
    private static native void nativeAccelerometerEvent( long bridgeAddress, double x, double y, double z, double deltaTime );
    private static native void nativeGyroscopeEvent( long bridgeAddress, double x, double y, double z, double deltaTime );
    private static native void nativeResizeEvent( long bridgeAddress );
    private static native void nativeAlertCallback( long bridgeAddress, int buttonIndex, boolean cancelled );
    private static native void nativeMultitouchEventBegin(long bridgeAddress);
    private static native void nativeMultitouchEventAdd( long bridgeAddress, int xLast, int yLast, int xStart, int yStart, int phaseType, long timestamp, int id );
    private static native void nativeMultitouchEventEnd( long bridgeAddress );
	private static native void nativeMemoryWarningEvent( long bridgeAddress );
	private static native void nativePopupClosedEvent( long bridgeAddress, String popupName, boolean isError );
	private static native Object nativeGetCoronaRuntime( long bridgeAddress );

	// Load all C/C++ libraries and their dependencies.
	// Note: Loading a library will NOT automatically load its dependencies. We must do that explicitly here.
    static {
		System.loadLibrary("lua");
		System.loadLibrary("jnlua5.1");
		System.loadLibrary("openal");
		// Certain products include this library(Enterprise, SDK) while other products don't(Cards) so it might not exist
		try {
			System.loadLibrary("mpg123");
		} catch (Exception e) {
		} catch (UnsatisfiedLinkError e) {}

		System.loadLibrary("almixer");
		System.loadLibrary("corona");
    }
	
	/** Constructor made private to prevent instances of this class from being made. */
	private JavaToNativeShim() {}
	
    public static String getVersion()
	{
		return nativeGetVersion();
	}

	public static void pause(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativePause(runtime.getJavaToNativeBridgeAddress());
	}

	public static String getBuildId(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return "unknown";
		}
		String ret = nativeGetBuildId(runtime.getJavaToNativeBridgeAddress());
		if (ret == null) {
			ret = "unknown";
		}
		return ret;
	}

	public static void resume(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeResume(runtime.getJavaToNativeBridgeAddress());
	}

	public static void dispatchEventInLua(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeDispatchEventInLua(runtime.getJavaToNativeBridgeAddress());
	}

	public static void applicationOpenEvent(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeApplicationOpenEvent(runtime.getJavaToNativeBridgeAddress());
	}

	public static void init(CoronaRuntime runtime)
	{
		if (runtime != null) {
			long javaToNativeBridgePointer = nativeInit(runtime);
			runtime.setJavaToNativeBridgeAddress(javaToNativeBridgePointer);
		}

	}

	public static void render(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeRender(runtime.getJavaToNativeBridgeAddress());
	}

	public static String getKeyNameFromKeyCode(int keyCode)
	{
		return nativeGetKeyNameFromKeyCode(keyCode);
	}
	
	public static int getMaxTextureSize(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return 0;
		}
		return nativeGetMaxTextureSize(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static int getHorizontalMarginInPixels(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return 0;
		}
		return nativeGetHorizontalMarginInPixels(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static int getVerticalMarginInPixels(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return 0;
		}
		return nativeGetVerticalMarginInPixels(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static int getContentWidthInPixels(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return 0;
		}
		return nativeGetContentWidthInPixels(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static int getContentHeightInPixels(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return 0;
		}
		return nativeGetContentHeightInPixels(runtime.getJavaToNativeBridgeAddress());
	}

	public static void useDefaultLuaErrorHandler()
	{
		nativeUseDefaultLuaErrorHandler();
	}
	
	public static void useJavaLuaErrorHandler()
	{
		nativeUseJavaLuaErrorHandler();
	}
	
	public static void unloadResources(CoronaRuntime runtime)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeUnloadResources(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static void destroy(CoronaRuntime runtime)
	{
		if (runtime == null) {
			return;
		}
		nativeDone(runtime.getJavaToNativeBridgeAddress());
	}

	public static boolean copyBitmapInfo(
		CoronaRuntime runtime, long nativeImageMemoryAddress, int width, int height, float downscaleFactor)
	{
		if (nativeImageMemoryAddress == 0) {
			return false;
		}
		if (runtime == null || runtime.wasDisposed()) {
			return false;
		}
		return nativeCopyBitmapInfo(runtime.getJavaToNativeBridgeAddress(), nativeImageMemoryAddress, width, height, downscaleFactor);
	}

	public static boolean copyBitmap(
		CoronaRuntime runtime, long nativeImageMemoryAddress, android.graphics.Bitmap bitmap,
		float downscaleFactor, boolean convertToGrayscale)
	{
		// Validate.
		if ((nativeImageMemoryAddress == 0) || (bitmap == null)) {
			return false;
		}

		if (runtime == null || runtime.wasDisposed()) {
			return false;
		}

		// Have the native C/C++ side copy the Java bitmap's pixels.
		return nativeCopyBitmap( runtime.getJavaToNativeBridgeAddress(),
					nativeImageMemoryAddress, bitmap, downscaleFactor, convertToGrayscale);
	}

	public static void setZipFileEntryInfo(
		long zipFileEntryMemoryAddress, com.ansca.corona.storage.AssetFileLocationInfo info)
	{
		// Validate.
		if ((zipFileEntryMemoryAddress == 0) || (info == null)) {
			return;
		}
		// Write the given information to the C++ ZipFileEntry object.
		String packageFilePath = null;
		if (info.getPackageFile() != null) {
			packageFilePath = info.getPackageFile().getAbsolutePath();
		}
		nativeSetZipFileEntryInfo(
					zipFileEntryMemoryAddress, packageFilePath, info.getZipEntryName(),
					info.getByteOffsetInPackage(), info.getByteCountInPackage(), info.isCompressed());
	}

	public static void update(CoronaRuntime runtime, com.ansca.corona.input.InputDeviceInterface device) {
		// Validate.
		if (device == null) {
			return;
		}
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}

		// Update the input device's information.
		nativeUpdateInputDevice(
					runtime.getJavaToNativeBridgeAddress(),
					device.getCoronaDeviceId(), device.getDeviceInfo().getAndroidDeviceId(),
					device.getDeviceInfo().getType().toCoronaIntegerId(),
					device.getDeviceInfo().getPermanentStringId(),
					device.getDeviceInfo().getProductName(), device.getDeviceInfo().getDisplayName(),
					device.getDeviceInfo().canVibrate(), device.getDeviceInfo().getPlayerNumber(),
					device.getConnectionState().toCoronaIntegerId());

		// Update the input device's axis information.
		nativeClearInputDeviceAxes(runtime.getJavaToNativeBridgeAddress(),device.getCoronaDeviceId());
		for (com.ansca.corona.input.AxisInfo axisInfo : device.getDeviceInfo().getAxes()) {
			if (axisInfo != null) {
				nativeAddInputDeviceAxis(runtime.getJavaToNativeBridgeAddress(),
						device.getCoronaDeviceId(), axisInfo.getType().toCoronaIntegerId(),
						axisInfo.getMinValue(), axisInfo.getMaxValue(), axisInfo.getAccuracy(),
						axisInfo.isProvidingAbsoluteValues());
			}
		}
	}
	
	public static void inputDeviceStatusEvent(
		CoronaRuntime runtime, 
		com.ansca.corona.input.InputDeviceInterface device,
		com.ansca.corona.input.InputDeviceStatusEventInfo eventInfo)
	{
		if ((device == null) || (eventInfo == null)) {
			return;
		}
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeInputDeviceStatusEvent(runtime.getJavaToNativeBridgeAddress(),
				device.getCoronaDeviceId(), eventInfo.hasConnectionStateChanged(), eventInfo.wasReconfigured());
	}
	
	public static void resize(
		CoronaRuntime runtime, android.content.Context context, int width, int height, boolean isCoronaKit)
	{
		com.ansca.corona.storage.FileServices fileServices = new com.ansca.corona.storage.FileServices(context);

		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeResize(runtime.getJavaToNativeBridgeAddress(),
				context.getPackageName(),
				CoronaEnvironment.getDocumentsDirectory(context).getAbsolutePath(),
				CoronaEnvironment.getApplicationSupportDirectory(context).getAbsolutePath(),
				CoronaEnvironment.getTemporaryDirectory(context).getAbsolutePath(),
				CoronaEnvironment.getCachesDirectory(context).getAbsolutePath(),
				CoronaEnvironment.getInternalCachesDirectory(context).getAbsolutePath(),
				fileServices.getExpansionFileDirectory().getAbsolutePath(),
				width, height, isCoronaKit);
	}
	
	public static void mouseEvent(
		CoronaRuntime runtime, int x, int y, int scrollX, int scrollY, long timestamp,
		boolean isPrimaryButtonDown, boolean isSecondaryButtonDown, boolean isMiddleButtonDown)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeMouseEvent(
				runtime.getJavaToNativeBridgeAddress(),
				x, y, scrollX, scrollY, timestamp,
				isPrimaryButtonDown, isSecondaryButtonDown, isMiddleButtonDown);
	}
	
	public static void touchEvent(CoronaRuntime runtime, com.ansca.corona.input.TouchTracker touchTracker) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeTouchEvent(runtime.getJavaToNativeBridgeAddress(), 
				(int)touchTracker.getLastPoint().getX(),
				(int)touchTracker.getLastPoint().getY(),
				(int)touchTracker.getStartPoint().getX(),
				(int)touchTracker.getStartPoint().getY(),
				touchTracker.getPhase().toCoronaNumericId(),
				touchTracker.getLastPoint().getTimestamp(),
				touchTracker.getTouchId());
	}
	
	public static void multitouchEventBegin(CoronaRuntime runtime) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeMultitouchEventBegin(runtime.getJavaToNativeBridgeAddress());
	}

	public static void multitouchEventAdd(CoronaRuntime runtime, com.ansca.corona.input.TouchTracker touchTracker) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeMultitouchEventAdd(runtime.getJavaToNativeBridgeAddress(), 
				(int)touchTracker.getLastPoint().getX(),
				(int)touchTracker.getLastPoint().getY(),
				(int)touchTracker.getStartPoint().getX(),
				(int)touchTracker.getStartPoint().getY(),
				touchTracker.getPhase().toCoronaNumericId(),
				touchTracker.getLastPoint().getTimestamp(),
				touchTracker.getTouchId());
	}

	public static void multitouchEventEnd(CoronaRuntime runtime) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeMultitouchEventEnd(runtime.getJavaToNativeBridgeAddress());
	}

	public static boolean keyEvent(
		CoronaRuntime runtime, 
		com.ansca.corona.input.InputDeviceInterface device,
		com.ansca.corona.input.KeyPhase phase, int keyCode,
		boolean isShiftDown, boolean isAltDown, boolean isCtrlDown, boolean isCommandDown)
	{
		if (runtime == null || runtime.wasDisposed()) {
			return false;
		}
		int coronaDeviceId = (device != null) ? device.getCoronaDeviceId() : 0;
		boolean wasHandled = nativeKeyEvent(
			runtime.getJavaToNativeBridgeAddress(), coronaDeviceId, phase.toCoronaNumericId(), keyCode,
			isShiftDown, isAltDown, isCtrlDown, isCommandDown);
		return wasHandled;
	}

	public static void axisEvent(
		CoronaRuntime runtime, com.ansca.corona.input.InputDeviceInterface device, com.ansca.corona.input.AxisDataEventInfo eventInfo)
	{
		if ((device == null) || (eventInfo == null)) {
			return;
		}
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		int coronaDeviceId = (device != null) ? device.getCoronaDeviceId() : 0;
		nativeAxisEvent(runtime.getJavaToNativeBridgeAddress(), coronaDeviceId, eventInfo.getAxisIndex(), eventInfo.getDataPoint().getValue());
	}
	
	public static void accelerometerEvent( CoronaRuntime runtime, double x, double y, double z, double deltaTime )
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeAccelerometerEvent( runtime.getJavaToNativeBridgeAddress(), x, y, z, deltaTime );
	}
	
	public static void gyroscopeEvent( CoronaRuntime runtime, double x, double y, double z, double deltaTime )
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeGyroscopeEvent( runtime.getJavaToNativeBridgeAddress(), x, y, z, deltaTime );
	}

	public static void resizeEvent( CoronaRuntime runtime) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeResizeEvent(runtime.getJavaToNativeBridgeAddress());
	}

	public static void alertCallback( CoronaRuntime runtime, int buttonIndex, boolean cancelled ) {
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeAlertCallback( runtime.getJavaToNativeBridgeAddress(), buttonIndex, cancelled );
	}
	
	public static void memoryWarningEvent( CoronaRuntime runtime )
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativeMemoryWarningEvent(runtime.getJavaToNativeBridgeAddress());
	}
	
	public static void popupClosedEvent( CoronaRuntime runtime, String popupName, boolean wasCanceled )
	{
		if (runtime == null || runtime.wasDisposed()) {
			return;
		}
		nativePopupClosedEvent( runtime.getJavaToNativeBridgeAddress(), popupName, wasCanceled );
	}

	public static CoronaRuntime getCoronaRuntimeFromBridge(long address) {
		return (CoronaRuntime)nativeGetCoronaRuntime(address);
	}
}
