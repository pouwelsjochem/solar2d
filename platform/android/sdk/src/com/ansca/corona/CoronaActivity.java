//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import java.lang.*;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.view.animation.AlphaAnimation;
import android.view.Window;
import android.view.WindowManager;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.util.Log;
import android.widget.*;
import com.ansca.corona.events.EventManager;
import com.ansca.corona.permissions.PermissionsSettings;
import com.ansca.corona.permissions.PermissionsServices;
import com.ansca.corona.permissions.PermissionState;
import com.ansca.corona.permissions.RequestPermissionsResultData;
import com.ansca.corona.storage.ResourceServices;
import com.ansca.corona.graphics.opengl.GLSurfaceView;

/** 
 * The activity window that hosts the Corona project. 
 * @see <a href="http://developer.android.com/reference/android/app/Activity.html">Activity</a>
 */
public class CoronaActivity extends Activity {
	
	private final int MIN_REQUEST_CODE = 1;

	private android.content.Intent myInitialIntent = null;
	private boolean myIsActivityResumed = false;
	private com.ansca.corona.graphics.opengl.CoronaGLSurfaceView myGLView;
	private android.widget.ImageView fSplashScreenView = null;
	private com.ansca.corona.purchasing.StoreProxy myStore = null;
	
	private Controller fController;
	private CoronaRuntime fCoronaRuntime;
	private LinearLayout fSplashView;
	private long fStartTime;
	private int SPLASH_SCREEN_DURATION = 1500;

	CoronaRuntime getRuntime() {
		return fCoronaRuntime;
	}

	/** This Corona activity's private event handler for various events. */
	private CoronaActivity.EventHandler fEventHandler;

	/** Handler used to post messages and Runnable objects to main UI thread's message queue. */
    private Handler myHandler = null;

    /** Listens for input events from the root content view and dispatches them to Corona's Lua listeners. */
    private com.ansca.corona.input.ViewInputHandler myInputHandler;
	
	/** Sends CoronaRuntimeTask objects to the Corona runtime's EventManager in a thread safe manner. */
	private CoronaRuntimeTaskDispatcher myRuntimeTaskDispatcher = null;

	/** Dictionary of activity result handlers which uses request codes as the key. */
	private java.util.HashMap<Integer, CoronaActivity.ResultHandler> fActivityResultHandlers =
						new java.util.HashMap<Integer, CoronaActivity.ResultHandler>();

	/** ArrayList of new intent result handlers. */
	private java.util.ArrayList<CoronaActivity.OnNewIntentResultHandler> fNewIntentResultHandlers = new java.util.ArrayList<CoronaActivity.OnNewIntentResultHandler>();

	/** Dictionary of request permissions result handlers which uses request codes as the key. */
	private java.util.HashMap<Integer, CoronaActivity.ResultHandler> fRequestPermissionsResultHandlers =
						new java.util.HashMap<Integer, CoronaActivity.ResultHandler>();
	
	/** 
	 * Base Result Handler interface used for various handler types we may need.
	 */
	private interface ResultHandler {}

	/**
	 * Handler that is invoked by the {@link com.ansca.corona.CoronaActivity CoronaActivity} when a result has been received from a 
	 * child <a href="http://developer.android.com/reference/android/app/Activity.html">Activity</a>.
	 * <p>
	 * This handler is assigned to the {@link com.ansca.corona.CoronaActivity CoronaActivity} via its 
	 * {@link com.ansca.corona.CoronaActivity#registerActivityResultHandler(com.ansca.corona.CoronaActivity.OnActivityResultHandler) 
	 * registerActivityResultHandler()} method, which returns a unique request code for you to use when calling 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a>.
	 * @see CoronaActivity
	 */
	public interface OnActivityResultHandler extends ResultHandler {
		/**
		 * Called when returning from an activity you've launched via the 
		 * <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a>
		 * method. This method will yield the returned result of that activity.
		 * @param activity The {@link com.ansca.corona.CoronaActivity CoronaActivity} that is receiving the result.
		 * @param requestCode The integer request code originally supplied to 
		 * <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a>. Allows you to identify which child activity that the result is coming from.
		 * @param resultCode The integer result code returned by the child activity via its 
		 * <a href="http://developer.android.com/reference/android/app/Activity.html#setResult(int)">setResult()</a> method.
		 * @param data An <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a> object which can return result data to the caller. Can be null.
		 */
		public void onHandleActivityResult(
				CoronaActivity activity, int requestCode, int resultCode, android.content.Intent data);
	}

	/**
	 * Handler that is invoked by the {@link com.ansca.corona.CoronaActivity CoronaActivity} when it receives a new
	 * <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a> from outside the application.
	 * <p>
	 * This handler is assigned to the {@link com.ansca.corona.CoronaActivity CoronaActivity} via its
	 * {@link com.ansca.corona.CoronaActivity#registerNewIntentResultHandler(com.ansca.corona.CoronaActivity.OnNewIntentResultHandler)
	 * registerNewIntentResultHandler()} method.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2869/">daily build 2016.2869</a></b>.
	 * @see CoronaActivity
	 */
	public interface OnNewIntentResultHandler extends ResultHandler {
		/**
		 * Called when {@link com.ansca.corona.CoronaActivity CoronaActivity} receives a new
		 * <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a>.
		 * @param intent An <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a> object which has been received.
		 */
		void onHandleNewIntentResult(android.content.Intent intent);
	}

	/**
	 * Handler that is invoked by the {@link com.ansca.corona.CoronaActivity CoronaActivity} when a result has been received from the request permission dialog.
	 * Only used in Android 6.0 and above.
	 * <p>
	 * This handler is assigned to the {@link com.ansca.corona.CoronaActivity CoronaActivity} via its
	 * {@link com.ansca.corona.CoronaActivity#registerRequestPermissionsResultHandler(com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler) registerRequestPermissionsResultHandler()} 
	 * method, which returns a unique request code for you to use when calling which returns a unique request code for you to use when
	 * calling 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a>.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @see CoronaActivity
	 */
	public interface OnRequestPermissionsResultHandler extends ResultHandler {
		/**
		 * Called when returning from the request permission dialog.
		 * @param activity The {@link com.ansca.corona.CoronaActivity CoronaActivity} that is receiving the result.
		 * @param requestCode The integer request code originally supplied to 
		 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a>. Allows you to identify which permissions request this came from.
		 * @param permissions The requested permissions. Never null. 
		 * @param grantResults The grant results for the corresponding permissions which is either 
		 * <a href="http://developer.android.com/reference/android/content/pm/PackageManager.html#PERMISSION_GRANTED">PERMISSION_GRANTED</a> or 
		 * <a href="http://developer.android.com/reference/android/content/pm/PackageManager.html#PERMISSION_DENIED">PERMISSION_DENIED</a>. Never null.
		 */
		public void onHandleRequestPermissionsResult(
				CoronaActivity activity, int requestCode, String[] permissions, int[] grantResults);
	}
	
	/**
	 * Called when this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>
	 * has been created, just before it starts. Initializes this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> view and member variables.
	 * @param savedInstanceState If the activity is being re-initialized after previously being shut down, then this bundle
	 * contains the data it most recently supplied in <a href="http://developer.android.com/reference/android/app/Activity.html#onSaveInstanceState(android.os.Bundle)">onSaveInstanceState()</a>. Otherwise it is null.
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		fStartTime = System.currentTimeMillis();

		// Work around FileProvider issue in level 25:
		// http://stackoverflow.com/questions/38200282/android-os-fileuriexposedexception-file-storage-emulated-0-test-txt-exposed
		// TODO: Fix this properly by using FileProvider appropriately
		//
		if (android.os.Build.VERSION.SDK_INT >= 24)
		{
			android.os.StrictMode.VmPolicy.Builder builder = new android.os.StrictMode.VmPolicy.Builder();
			android.os.StrictMode.setVmPolicy(builder.build());
		}

		// Store the intent that initially launched this activity. To be passed to Lua as launch argument later.
		// Note: The getIntent() method will return a different intent object if this activity gets resumed
		//       externally such as by a notification or the Facebook app.
		myInitialIntent = getIntent();

		// Fetch this activity's meta-data from the manifest.
		boolean isKeyboardAppPanningEnabled = false;
		try {
			android.content.pm.ActivityInfo activityInfo;
			activityInfo = getPackageManager().getActivityInfo(
						getComponentName(), android.content.pm.PackageManager.GET_META_DATA);
			if ((activityInfo != null) && (activityInfo.metaData != null)) {
				isKeyboardAppPanningEnabled =
						activityInfo.metaData.getBoolean("coronaWindowMovesWhenKeyboardAppears");
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}

		// Show the window fullscreen, if possible.
		// Note: Do not show the window in fullscreen mode if we want the keyboard to pan the app.
		//       We do this because the Android OS does not support ADJUST_PAN when in fullscreen mode.
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		if (isKeyboardAppPanningEnabled == false) {
			getWindow().setFlags(
					WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
					WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
		}
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

		// When soft keyboard is shown, don't slide as that messes up text fields
		getWindow().setSoftInputMode(
				WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN |
				WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

		// If this is a new installation of this app, then delete its "externalized" assets to be replaced by the new ones.
		// We determine if this is a new installation by storing the APK's timestamp to the preferences file since last the app ran.
		if (CoronaEnvironment.isNewInstall(this)) {
			CoronaEnvironment.onNewInstall(this);
		}
		CoronaEnvironment.deleteTempDirectory(this);
		
		// Make this activity available to the rest of the application.
		CoronaEnvironment.setCoronaActivity(this);

		// Create our CoronaRuntime, which also initializes the native side of the CoronaRuntime.
		fCoronaRuntime = new CoronaRuntime(this, false);

		// Set initialSystemUiVisibility before splashScreen comes up
		try {
			android.content.pm.ActivityInfo activityInfo;
			activityInfo = getPackageManager().getActivityInfo(getComponentName(), android.content.pm.PackageManager.GET_META_DATA);
			if ((activityInfo != null) && (activityInfo.metaData != null)) {
				String initialSystemUiVisibility = activityInfo.metaData.getString("initialSystemUiVisibility");
				if (initialSystemUiVisibility != null) {
					if (initialSystemUiVisibility.equals("immersiveSticky") && Build.VERSION.SDK_INT < 19) {
						fCoronaRuntime.getController().setSystemUiVisibility("lowProfile");
					} else {
						fCoronaRuntime.getController().setSystemUiVisibility(initialSystemUiVisibility);
					}
				}
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}

		// fCoronaRuntime = new CoronaRuntime(this, false);		// for Tegra debugging

		showCoronaSplashScreen();

		// Create the Controller instance used to manage general Corona data on the Java side.
		// This also creates the CoronaRuntime object which will manage the LuaState in Java.
		fController = fCoronaRuntime.getController();

		CoronaEnvironment.setController(fController);

		CoronaApiHandler apiHandler = new CoronaApiHandler(this, fCoronaRuntime);
		fController.setCoronaApiListener(apiHandler);

		CoronaShowApiHandler showApiHandler = new CoronaShowApiHandler(this, fCoronaRuntime);
		fController.setCoronaShowApiListener(showApiHandler);

		CoronaSplashScreenApiHandler splashScreenHandler = new CoronaSplashScreenApiHandler(this);
		fController.setCoronaSplashScreenApiListener(splashScreenHandler);

		CoronaStoreApiHandler storeHandler = new CoronaStoreApiHandler(this);
		fController.setCoronaStoreApiListener(storeHandler);

		CoronaSystemApiHandler systemHandler = new CoronaSystemApiHandler(this);
		fController.setCoronaSystemApiListener(systemHandler);

		// Attempt to load/reload this application's expansion files, if they exist.
		com.ansca.corona.storage.FileServices fileServices = new com.ansca.corona.storage.FileServices(this);
		fileServices.loadExpansionFiles();
		
		// Create and set up a store object used to manage in-app purchases.
		myStore = new com.ansca.corona.purchasing.StoreProxy(fCoronaRuntime, fCoronaRuntime.getController());
		myStore.setActivity(this);
		
		// Set up a handler for sending messages and posting Runnable object to the main UI threads message queue.
		myHandler = new Handler();
		
		// Set up a dispatcher for sending tasks to the Corona runtime via the EventManager.
		// This is mostly intended for customers who use the Enterprise version of Corona.
		myRuntimeTaskDispatcher = new CoronaRuntimeTaskDispatcher(fCoronaRuntime);

		// Validate the "AndroidManifest.xml" file and display an error if anything is misconfigured.
		try {
			com.ansca.corona.storage.FileContentProvider.validateManifest(this);
		}
		catch (Exception ex) {
			fController.showNativeAlert("Error", ex.getMessage(), null);
		}
		
		// Create the views for this activity.
		// Create the OpenGL view and initialize the Corona runtime.
		myGLView = fCoronaRuntime.getGLView();
		myGLView.setActivity(this);
		ViewManager viewManager = fCoronaRuntime.getViewManager();

		setContentView(viewManager.getContentView());

		// Set up the input handler used to dispatch key events, touch events, etc. to Corona's Lua listeners.
		myInputHandler = new com.ansca.corona.input.ViewInputHandler(fCoronaRuntime.getController());
		myInputHandler.setDispatcher(myRuntimeTaskDispatcher);
		myInputHandler.setView(viewManager.getContentView());
		
		// Start up the notification system, if not done already.
		// Note: Creating the NotificationServices object is all it takes.
		new com.ansca.corona.notifications.NotificationServices(this);

		// Start up the input device monitoring system, if not done already.
		// Note: Creating the InputDeviceServices object is all it takes.
		new com.ansca.corona.input.InputDeviceServices(this);

		// Set up a private event handler.
		fEventHandler = new CoronaActivity.EventHandler(this);

		// Sync up status of dangerous-level permissions.
		// This is to work around the possibility of a partially granted permission group which 
		// can occur if access to a permission group is granted, but then the app is updated with 
		// another permission in that group after access to the group has been granted.
		if (android.os.Build.VERSION.SDK_INT >= 23) {
			syncPermissionStateForAllPermissions();
		}

	}

	/* Public APIs that connect to the Controller class */

	/**
	 * Creates an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> 
	 * with the proper theme for the given device.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2932/">daily build 2016.2932</a></b>.
	 * @param context The <a href="http://developer.android.com/reference/android/content/Context.html">Context</a> 
	 *				  to help infer what theme to use.
	 * @return Returns an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a>
	 *		   that uses as close to the native device's theme as possible.
	 *		   <p>
	 *		   Returns null if an 
	 *		   <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> could not
	 *		   be created. If this happens, this means that your app is closing and you shouldn't be presenting an alert anyway.
	 */
	public android.app.AlertDialog.Builder createAlertDialogBuilder(android.content.Context context) {
		if (fController == null) return null;
		return fController.createAlertDialogBuilder(context);
	}

	/**
	 * Creates an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> 
	 * with the dark theme.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2961/">daily build 2016.2961</a></b>.
	 * @param context The <a href="http://developer.android.com/reference/android/content/Context.html">Context</a> 
	 *				  to help infer what theme to use.
	 * @return Returns an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a>
	 *		   that uses a dark theme as close to the native device's theme as possible.
	 *		   <p>
	 *		   Returns null if an 
	 *		   <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> could not
	 *		   be created. If this happens, this means that your app is closing and you shouldn't be presenting an alert anyway.
	 */
	public android.app.AlertDialog.Builder createDarkAlertDialogBuilder(android.content.Context context) {
		if (fController == null) return null;
		return fController.createDarkAlertDialogBuilder(context);
	}

	/**
	 * Creates an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> 
	 * with the light theme.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2961/">daily build 2016.2961</a></b>.
	 * @param context The <a href="http://developer.android.com/reference/android/content/Context.html">Context</a> 
	 *				  to help infer what theme to use.
	 * @return Returns an <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a>
	 *		   that uses a light theme as close to the native device's theme as possible.
	 *		   <p>
	 *		   Returns null if an 
	 *		   <a href="http://developer.android.com/reference/android/app/AlertDialog.Builder.html">AlertDialog.Builder</a> could not
	 *		   be created. If this happens, this means that your app is closing and you shouldn't be presenting an alert anyway.
	 */
	public android.app.AlertDialog.Builder createLightAlertDialogBuilder(android.content.Context context) {
		if (fController == null) return null;
		return fController.createLightAlertDialogBuilder(context);
	}

	/**
	 * Displays a native alert stating that this permission is missing from the 
	 * <a href="http://developer.android.com/guide/topics/manifest/manifest-intro.html">AndroidManifest.xml</a>.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param permission The name of the permission that's missing.
	 * @param message A message to explain why the action can't be performed.
	 */
	public void showPermissionMissingFromManifestAlert(String permission, String message) {
		if (fController == null) return;
		fController.showPermissionMissingFromManifestAlert(permission, message);
	}

	/**
	 * Displays a native alert stating that the 
	 * <a href="http://developer.android.com/guide/topics/manifest/manifest-intro.html">AndroidManifest.xml</a>
	 * doesn't contain any permissions from the desired permission group.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param permissionGroup The name of the permission group that's there's no permissions for.
	 */
	public void showPermissionGroupMissingFromManifestAlert(String permissionGroup) {
		if (fController == null) return;
		fController.showPermissionGroupMissingFromManifestAlert(permissionGroup);
	}

	/* End of Public APIs that connect to the Controller class */

	/**
	 * Syncs up all requested permissions in the same permission group to the same status.
	 */
	void syncPermissionStateForAllPermissions() {
		PermissionsServices permissionsServices = new PermissionsServices(this);
		String[] dangerousPermissionGroups = permissionsServices.getSupportedPermissionGroups();

		// Build up a set of all permissions to request for the sync.
		java.util.LinkedHashSet<String> permissionsSet = new java.util.LinkedHashSet<String>();

		// Go through each dangerous permission group
		for (int groupIdx = 0; groupIdx < dangerousPermissionGroups.length; groupIdx++) {
			// Get all requested permissions in the AndroidManifest.xml for this group.
			String[] allPermissionsInGroup = permissionsServices.findAllPermissionsInManifestForGroup(dangerousPermissionGroups[groupIdx]);

			if (allPermissionsInGroup != null && allPermissionsInGroup.length > 1) {
				// Check for descrepencies in permission state.
				boolean foundGrantedPermission = false;
				boolean foundDeniedPermission = false;
				for (int permissionIdx = 0; permissionIdx < allPermissionsInGroup.length; permissionIdx++) {
					PermissionState currentPermissionState = permissionsServices.getPermissionStateFor(allPermissionsInGroup[permissionIdx]);
					switch (currentPermissionState) {
						case GRANTED:
							foundGrantedPermission = true;
							if (foundDeniedPermission) {
								// Found a descrepency and need to request the entire group again.
								// We can request any permission in this group and all will be granted.
								permissionsSet.add(allPermissionsInGroup[permissionIdx]);
							}
							break;
						case DENIED:
							foundDeniedPermission = true;
							if (foundGrantedPermission) {
								// Found a descrepency and need to request the entire group again.
								// We can request any permission in this group and all will be granted.
								permissionsSet.add(allPermissionsInGroup[permissionIdx]);
							}
							break;
						default:
							break;
					}
				}
			}
		}

		if (!permissionsSet.isEmpty()) {
			// Request all desired permissions for sync! TODO: USE PERMISSIONS SERVICES FULLY HERE!
			permissionsServices.requestPermissions(new PermissionsSettings(permissionsSet), new DefaultRequestPermissionsResultHandler());
		}
	}

	/**
	 * Gets this activity's OpenGL view that Corona renders to.
	 * <p>
	 * This is an internal method that can only be called by Corona.
	 * @return Returns the OpenGL view.
	 *         <p>
	 *         Returns null if this activity has not been created yet or if the activity has been destroyed.
	 */
	com.ansca.corona.graphics.opengl.CoronaGLSurfaceView getGLView() {
		return myGLView;
	}
	
	/**
	 * Gets the view that is overlaid on top of this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> main OpenGL view.
	 * <p>
	 * This view is intended to be used as a container for other views such as text fields, web views,
	 * ads and other UI objects. All view objects added to this view group will be overlaid
	 * on top of the main Corona content.
	 * @return Returns a <a href="http://developer.android.com/reference/android/widget/FrameLayout.html">FrameLayout</a> 
	 *		   view group object that is owned by this 
	 *		   <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 *         <p>
	 *         Returns null if this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> has not
	 *		   been created yet or if the <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> has
	 *		   been destroyed.
	 */
	public android.widget.FrameLayout getOverlayView() {
		ViewManager viewManager = fCoronaRuntime.getViewManager();
		if (viewManager == null) {
			return null;
		}
		return viewManager.getOverlayView();
	}

	/**
	 * Gets the <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that created and launched this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * @return Returns the <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that created this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 */
	public android.content.Intent getInitialIntent() {
		return myInitialIntent;
	}

	/**
	 * Gets the <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that last started/resumed this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * <p>
	 * The returned <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> will initially be the 
	 * <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that created and launched this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * <p>
	 * The returned <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> will change when this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> is started by a call to the 
	 * <a href="http://developer.android.com/reference/android/content/Context.html#startActivity(android.content.Intent)">startActivity()</a>
	 * or <a href="http://developer.android.com/reference/android/app/Activity.html#setIntent(android.content.Intent)">setIntent()</a> 
	 * methods, which typically happens external to this application such as when a notification gets tapped by the end-user or by 
	 * another application such as Facebook.
	 * @return Returns the <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that last started 
	 * this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 */
	@Override
	public android.content.Intent getIntent() {
		return super.getIntent();
	}
	
	/**
	 * Gets the horizontal margin of the Corona content. This is the distance 
	 * between the origins of the screen and the Corona content.
	 * @return Returns the value in pixel units.
	 */
	public int getHorizontalMarginInPixels() {
		return JavaToNativeShim.getHorizontalMarginInPixels(fCoronaRuntime);
	}
	
	/**
	 * Gets the vertical margin of the Corona content. This is the distance 
	 * between the origins of the screen and the Corona content.
	 * @return Returns the value in pixel units.
	 */
	public int getVerticalMarginInPixels() {
		return JavaToNativeShim.getVerticalMarginInPixels(fCoronaRuntime);
	}

	/**
	 * Gets the width of the Corona content.
	 * @return Returns the width in pixel units.
	 */
	public int getContentWidthInPixels() {
		return JavaToNativeShim.getContentWidthInPixels(fCoronaRuntime);
	}
	
	/**
	 * Gets the height of the Corona content.
	 * @return Returns the height in pixel units.
	 */
	public int getContentHeightInPixels() {
		return JavaToNativeShim.getContentHeightInPixels(fCoronaRuntime);
	}
	
	/**
	 * Gets the <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> 
	 * <a href="http://developer.android.com/reference/android/os/Handler.html">handler</a> used to post 
	 * <a href="http://developer.android.com/reference/android/os/Message.html">messages</a> and {@link java.lang.Runnable runnables} 
	 * to the <a href="http://developer.android.com/reference/android/os/MessageQueue.html">message queue</a> on the main UI thread.
	 * <p>
	 * You can call this method from any thread. The intention of this method is to provide a thread safe mechanism
	 * for other threads to post {@link java.lang.Runnable Runnable objects} to be ran on the main UI thread.
	 * @return Returns a reference to the <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> 
	 * <a href="http://developer.android.com/reference/android/os/Handler.html">handler</a> which is associated with the main UI thread.
	 */
	public Handler getHandler() {
		return myHandler;
	}
	
	/**
	 * Gets a dispatcher for sending {@link com.ansca.corona.CoronaRuntimeTask CoronaRuntimeTask objects} to the 
	 * {@link com.ansca.corona.CoronaRuntime CoronaRuntime} owned by this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * <p>
	 * You can call this method from any thread. The intention of this method is to provide a thread safe mechanism
	 * for sending tasks to the {@link com.ansca.corona.CoronaRuntime CoronaRuntime} thread so that you can access its 
	 * {@link com.naef.jnlua.LuaState LuaState}.
	 * @return Returns a dispatcher for sending tasks to the {@link com.ansca.corona.CoronaRuntime CoronaRuntime} thread.
	 *         <p>
	 *         Returns null if the CoronaActivity's {@link com.ansca.corona.CoronaActivity#onCreate(Bundle) onCreate()} 
	 *		   method has not been called yet.
	 */
	public CoronaRuntimeTaskDispatcher getRuntimeTaskDispatcher() {
		return myRuntimeTaskDispatcher;
	}

	/**
	 * Returns true if device HAS software navigation bar or false if it hasn't
	 */
	public boolean HasSoftwareKeys()
    {
		boolean hasSoftwareKeys = true;
		if (android.os.Build.VERSION.SDK_INT>=android.os.Build.VERSION_CODES.JELLY_BEAN_MR1){
			android.view.Display display = getWindowManager().getDefaultDisplay();
	
			android.util.DisplayMetrics realDisplayMetrics = new android.util.DisplayMetrics();
			display.getRealMetrics(realDisplayMetrics);
	
			int realHeight = realDisplayMetrics.heightPixels;
			int realWidth = realDisplayMetrics.widthPixels;
	
			android.util.DisplayMetrics displayMetrics = new android.util.DisplayMetrics();
			display.getMetrics(displayMetrics);
	
			int displayHeight = displayMetrics.heightPixels;
			int displayWidth = displayMetrics.widthPixels;
	
			hasSoftwareKeys = (realWidth - displayWidth) > 0 || (realHeight - displayHeight) > 0;
		} else {
			boolean hasMenuKey = android.view.ViewConfiguration.get(CoronaEnvironment.getApplicationContext()).hasPermanentMenuKey();
			boolean hasBackKey = KeyCharacterMap.deviceHasKey(KeyEvent.KEYCODE_BACK);
			hasSoftwareKeys = !(hasMenuKey || hasBackKey);
		}
		return hasSoftwareKeys;
    }

	/**
	 * Tries to get the navigation bar height based on system resources.
	 */
	int resolveNavBarHeight() {
		int height = 0;
		// Get the navigation bar height from the status bar resource.
		int resourceId = getResources().getIdentifier("navigation_bar_height", "dimen", "android");
		if (resourceId > 0)
			height = getResources().getDimensionPixelSize(resourceId);
		return height;
	}
	
	/**
	 * Gets a store proxy object used to manage in-app purchases.
	 * Allows the caller to purchase products, restore purchases, confirm transactions, etc.
	 * <p>
	 * Initially, this store object cannot access a store until it has been told which store. 
	 * The Lua store.init() function is expected to call one of these methods.
	 * @return Returns a reference to the a store proxy object. 
	 */
	com.ansca.corona.purchasing.StoreProxy getStore() {
		return myStore;
	}
	
	/**
	 * Registers a handler to be called when a result has been received by the appropriate callback in CoronaActivity.
	 * @param handler The handler to be called.
	 * @param handlerHashMap The HashMap of handlers that we want to register this handler with.
	 * @return Returns a unique request code that you can pass to the appripriate method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 */
	private int registerResultHandler(CoronaActivity.ResultHandler handler, 
		java.util.HashMap<Integer, CoronaActivity.ResultHandler> handlerHashMap) {

		// Validate.
		if (handler == null || handlerHashMap == null) {
			return -1;
		}

		// Generate a unique request code for the given handler.
		int requestCode = MIN_REQUEST_CODE;
		while (handlerHashMap.containsKey(Integer.valueOf(requestCode))) {
			requestCode++;
			if (requestCode < MIN_REQUEST_CODE) {
				requestCode = MIN_REQUEST_CODE;
			}
		}
		
		// Add the given handler to the collection.
		handlerHashMap.put(Integer.valueOf(requestCode), handler);
		return requestCode;
	}

	/**
	 * Registers a handler to be called when a result has been received by the appropriate callback in CoronaActivity.
	 * The assigned request codes are guarenteed to be contiguous.
	 * @param handler The handler to be called.
	 * @param numRequestCodes The amount of request codes to map this handler to.
	 * @param handlerHashMap The HashMap of handlers that we want to register this handler with.
	 * @return Returns a unique request code that you can pass to the appripriate method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Makes no assumptions about the status of the handler.
	 */
	private int registerResultHandler(CoronaActivity.ResultHandler handler, int numRequestCodes,
		java.util.HashMap<Integer, CoronaActivity.ResultHandler> handlerHashMap) {

		// Validate.
		if (handler == null || numRequestCodes < 1 || handlerHashMap == null) {
			return -1;
		}

		int requestCodeOffset = MIN_REQUEST_CODE;

		// If no other handlers have been registered, then we can start our contiguous block at the beginning
		if (!handlerHashMap.isEmpty()) {

			// Find the beginning of a contiguous block of request codes
			// Grab the keyset for the handlerHashMap and
			// put it in a TreeSet to sort it.
			java.util.TreeSet<Integer> sortedKeys = new java.util.TreeSet<Integer>(handlerHashMap.keySet());

			// Next contiguous block is after that last key
			requestCodeOffset = sortedKeys.last() + 1;
		}

		// Add each of the desired request codes for this handler
		for (int requestCode = requestCodeOffset; 
			requestCode < requestCodeOffset + numRequestCodes; requestCode++) {
			
			// Add the given handler to the collection.
			handlerHashMap.put(Integer.valueOf(requestCode), handler);
		}
		
		return requestCodeOffset;
	}

	/**
	 * Unregisters the handler that was given to the registerResultHandler() method.
	 * This prevents that handler from getting called and frees its request code to be used
	 * for future registered handlers.
	 * @param handler Reference to the handler that was given to the registerResultHandler() method.
	 * @param handlerHashMap The HashMap of handlers that we want to unregister this handler from.
	 * @return Returns an ArrayList of all the request codes that are now available.
	 *         Can be used to free other resources that have some association with these request codes.
	 *         <p>
	 *         Returns null if given an invalid argument.
	 *         <p>
	 */
	private java.util.ArrayList<Integer> unregisterResultHandler(CoronaActivity.ResultHandler handler,
		java.util.HashMap<Integer, CoronaActivity.ResultHandler> handlerHashMap) {

		// Validate arguments.
		if (handler == null || handlerHashMap == null) {
			return null;
		}

		// Fetch all request codes assigned to the given handler.
		java.util.ArrayList<Integer> requestCodes = new java.util.ArrayList<Integer>();
		for (java.util.Map.Entry<Integer, CoronaActivity.ResultHandler> entry :
		     handlerHashMap.entrySet())
		{
			if (entry.getValue() == handler) {
				requestCodes.add(entry.getKey());
			}
		}

		// Remove all references for the given handler.
		for (Integer requestCode : requestCodes) {
			handlerHashMap.remove(requestCode);
		}

		return requestCodes;
	}

	/**
	 * Registers an {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} to be called when a child 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> result has been received by the
	 * {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onActivityResult(int, int, Intent) onActivityResult()} method.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} to be called when the
	 * child <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> result has been received.
	 * @return Returns a unique request code that you can pass into the <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a> 
	 *		   method when launching the child <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 */
	public int registerActivityResultHandler(CoronaActivity.OnActivityResultHandler handler) {
		return registerResultHandler(handler, fActivityResultHandlers);
	}

	/**
	 * Registers an {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} to be called when a child 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> result has been received by the
	 * {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onActivityResult(int, int, Intent) onActivityResult()} method. 
	 * <p>
	 * <b>The assigned request codes are guarenteed to be contiguous!</b>
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2015/2669/">daily build 2015.2669</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} to be called when the
	 * 		  child <a href="http://developer.android.com/reference/android/app/Activity.html">activity's</a> result has been received.
	 * @param numRequestCodes The amount of request codes to map this 
	 * 		  {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} to.
	 * @return Returns a unique request code that you can pass into the <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a> 
	 *		   method when launching the child <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Makes no assumptions about the status of the 
	 *		   {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler}.
	 */
	public int registerActivityResultHandler(CoronaActivity.OnActivityResultHandler handler, int numRequestCodes) {
		return registerResultHandler(handler, numRequestCodes, fActivityResultHandlers);
	}

	/**
	 * Unregisters the {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} that was given to the 
	 * {@link com.ansca.corona.CoronaActivity#registerActivityResultHandler(com.ansca.corona.CoronaActivity.OnActivityResultHandler) registerActivityResultHandler()} 
	 * method. This prevents that {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} from getting
	 * called and frees its request code to be used for future registered 
	 * {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandlers}.
	 * @param handler Reference to the {@link com.ansca.corona.CoronaActivity.OnActivityResultHandler OnActivityResultHandler} that was
	 * given to the {@link com.ansca.corona.CoronaActivity#registerActivityResultHandler(com.ansca.corona.CoronaActivity.OnActivityResultHandler) registerActivityResultHandler()} method.
	 */
	public void unregisterActivityResultHandler(CoronaActivity.OnActivityResultHandler handler) {
		unregisterResultHandler(handler, fActivityResultHandlers);
	}

	/**
	 * Registers an {@link com.ansca.corona.CoronaActivity.OnNewIntentResultHandler OnNewIntentResultHandler} to be called when
	 * {@link com.ansca.corona.CoronaActivity CoronaActivity} receives a new
	 * <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a>.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2869/">daily build 2016.2869</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnNewIntentResultHandler OnNewIntentResultHandler} to be called when
	 * {@link com.ansca.corona.CoronaActivity CoronaActivity} receives a new
	 * <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a>.
	 */
	public void registerNewIntentResultHandler(CoronaActivity.OnNewIntentResultHandler handler) {
		fNewIntentResultHandlers.add(handler);
	}

	/**
	 * Unregisters the {@link com.ansca.corona.CoronaActivity.OnNewIntentResultHandler OnNewIntentResultHandler} that was given to the
	 * {@link com.ansca.corona.CoronaActivity#registerNewIntentResultHandler(com.ansca.corona.CoronaActivity.OnNewIntentResultHandler) registerNewIntentResultHandler()}
	 * method. This prevents that {@link com.ansca.corona.CoronaActivity.OnNewIntentResultHandler OnNewIntentResultHandler} from getting
	 * called.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2869/">daily build 2016.2869</a></b>.
	 * @param handler Reference to the {@link com.ansca.corona.CoronaActivity.OnNewIntentResultHandler OnNewIntentResultHandler} that was
	 * given to the {@link com.ansca.corona.CoronaActivity#registerNewIntentResultHandler(com.ansca.corona.CoronaActivity.OnNewIntentResultHandler) registerNewIntentResultHandler()} method.
	 */
	public void unregisterNewIntentResultHandler(CoronaActivity.OnActivityResultHandler handler) {
		fNewIntentResultHandlers.remove(handler);
	}

	/**
	 * Registers a {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to be
	 * called when a result has been received by the {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onRequestPermissionsResult(int, java.lang.String[], int[]) onRequestPermissionsResult()} method.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}
	 *				  to be called when the request permission dialog's result has been received.
	 * @return Returns a unique request code that you can pass into the 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a> method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Returns 0 if not on Android 6.0 or above, meaning that you don't need 
	 *         to register an 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} 
	 *		   on this version of Android.
	 */
	public int registerRequestPermissionsResultHandler(CoronaActivity.OnRequestPermissionsResultHandler handler) {
		return android.os.Build.VERSION.SDK_INT >= 23 ? 
			registerResultHandler(handler, fRequestPermissionsResultHandlers) : 0;
	}

	/**
	 * Registers a {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to be
	 * called when a result has been received by the {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onRequestPermissionsResult(int, java.lang.String[], int[]) onRequestPermissionsResult()} method.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}
	 *				  to be called when the request permission dialog's result has been received.
	 * @param settings The {@link com.ansca.corona.permissions.PermissionsSettings PermissionsSettings} used to create the request
	 *				   permissions dialog.
	 * @return Returns a unique request code that you can pass into the 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a> method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Returns 0 if not on Android 6.0 or above, meaning that you don't need 
	 *         to register an 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} 
	 *		   on this version of Android.
	 */
	public int registerRequestPermissionsResultHandler(
		CoronaActivity.OnRequestPermissionsResultHandler handler, PermissionsSettings settings) {
		
		int requestCode = registerRequestPermissionsResultHandler(handler);
		if (requestCode > 0) {
			// Add the permission settings since we have a legit request code
			PermissionsSettings.getSettingsToBeServiced().put(Integer.valueOf(requestCode), settings);
		}
		return requestCode;
	}

	/**
	 * Registers a {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to be
	 * called when a result has been received by the {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onRequestPermissionsResult(int, java.lang.String[], int[]) onRequestPermissionsResult()} method.
	 * <p>
	 * <b>The assigned request codes are guarenteed to be contiguous!</b>
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}
	 *				  to be called when the request permission dialog's result has been received.
	 * @param numRequestCodes The amount of request codes to map this 
	 * {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to.
	 * @return Returns a unique request code that you can pass into the 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a> method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Returns 0 if not on Android 6.0 or above, meaning that you don't need 
	 *         to register an 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} 
	 *		   on this version of Android.
	 *         <p>
	 *         Makes no assumptions about the status of the 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}.
	 */
	public int registerRequestPermissionsResultHandler(CoronaActivity.OnRequestPermissionsResultHandler handler, int numRequestCodes) {
		return android.os.Build.VERSION.SDK_INT >= 23 ? 
			registerResultHandler(handler, numRequestCodes, fRequestPermissionsResultHandlers) : 0;
	}

	/**
	 * Registers a {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to be
	 * called when a result has been received by the {@link com.ansca.corona.CoronaActivity CoronaActivity's} 
	 * {@link com.ansca.corona.CoronaActivity#onRequestPermissionsResult(int, java.lang.String[], int[]) onRequestPermissionsResult()} method.
	 * <p>
	 * <b>The assigned request codes are guarenteed to be contiguous!</b>
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param handler The {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}
	 *				  to be called when the request permission dialog's result has been received.
	 * @param numRequestCodes The amount of request codes to map this 
	 * {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} to.
	 * @param settings The {@link com.ansca.corona.permissions.PermissionsSettings PermissionsSettings} used to create the request
	 *				   permissions dialog.
	 * @return Returns a unique request code that you can pass into the 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a> method.
	 *         <p>
	 *         Returns -1 if given an invalid argument.
	 *         <p>
	 *         Returns 0 if not on Android 6.0 or above, meaning that you don't need 
	 *         to register an 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} 
	 *		   on this version of Android.
	 *         <p>
	 *         Makes no assumptions about the status of the 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}.
	 */
	public int registerRequestPermissionsResultHandler(
		CoronaActivity.OnRequestPermissionsResultHandler handler, int numRequestCodes, PermissionsSettings settings) {
		
		int requestCodeOffset = registerRequestPermissionsResultHandler(handler);
		if (requestCodeOffset > 0) {
			// Add the permission settings for each request code granted.
			for(int requestCode = requestCodeOffset; requestCode < requestCodeOffset + numRequestCodes; requestCode++) {
				PermissionsSettings.getSettingsToBeServiced().put(Integer.valueOf(requestCode), settings);
			}
		}
		return requestCodeOffset;
	}

	/**
	 * Unregisters the {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}
	 * that was given to the {@link com.ansca.corona.CoronaActivity#registerRequestPermissionsResultHandler(com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler) registerRequestPermissionsResultHandler()} method.
	 * This prevents that {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} 
	 * from getting called and frees its request code to be used for future registered 
	 * {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandlers}.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param handler Reference to the {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} that was given to the 
	 * {@link com.ansca.corona.CoronaActivity#registerRequestPermissionsResultHandler(com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler) registerRequestPermissionsResultHandler()} method.
	 * @return Returns the {@link com.ansca.corona.permissions.PermissionsSettings PermissionsSettings} used to create the Request
	 *		   Permissions dialog that provided a result to this 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}.
	 *         <p>
	 *         Returns null if no {@link com.ansca.corona.permissions.PermissionsSettings PermissionsSettings} were associated with
	 *		   this {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler} or 
	 *		   given an invalid 
	 *		   {@link com.ansca.corona.CoronaActivity.OnRequestPermissionsResultHandler OnRequestPermissionsResultHandler}.
	 */
	public PermissionsSettings unregisterRequestPermissionsResultHandler(CoronaActivity.OnRequestPermissionsResultHandler handler) {
		
		PermissionsSettings settingsServiced = null;
		java.util.ArrayList<Integer> requestCodesToFree = unregisterResultHandler(handler, fRequestPermissionsResultHandlers);

		// TODO: Resolve edge cases of the PermissionsSettings and Handlers HashTables not aligning.
		if(requestCodesToFree != null && !requestCodesToFree.isEmpty()) {
			// Free the permission settings that depend on these requestCodes.
			for (Integer requestCode : requestCodesToFree) {
				settingsServiced = PermissionsSettings.getSettingsToBeServiced().remove(requestCode);
			}
		}

		return settingsServiced;
	}

	/**
	 * Called just before this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> 
	 * is closed and destroyed.
	 */
    @Override
	protected void onDestroy() {
		// Unsubscribe from events.
		fEventHandler.dispose();

		// wait renderer thread completing
		myGLView.requestExitAndWait();

		// Destroy the Corona runtime and this activity's views.
		myGLView = null;
		myStore.disable();

		fSplashView = null;

		fCoronaRuntime.dispose();
		fCoronaRuntime = null;

		// Remove this activity's reference from the CoronaEnvironment last in case the above
		// destroy methods require access to this activity.
		CoronaEnvironment.setCoronaActivity(null);

		// Destroy this activity.
		super.onDestroy();
	}
	
	/**
	 * This method is called when the user navigates to this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>, forward or back, 
	 * from another <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 */
    @Override
	protected void onStart() {
		super.onStart();
  	}
	
	/**
	 * This method is called when this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>
	 * can be interacted with.
	 */
	@Override
	protected void onResume() {
		super.onResume();

		// Start or resume the Corona runtime.
		myIsActivityResumed = true;
		requestResumeCoronaRuntime();

		// Resume this activity's views if they were suspended.
		if (fCoronaRuntime != null) {
			fCoronaRuntime.updateViews();
		}
	}
	
	/**
	 * This method is called when this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>
	 * cannot be interacted with by the user, which can happen when:
	 * <ul>
	 *  <li> Leaving this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> 
	 *		 to go to another <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 *  <li> The power button was pressed or the screen lock is displayed.
	 *  <li> The task manager view is overlayed on top of this 
	 *		 <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * </ul>
	 */
	@Override
	protected void onPause() {
		super.onPause();

		// Suspend the Corona runtime.
		// (Note: We do not want to destroy the OpenGL surface here because the activity window is still visible.)
		myIsActivityResumed = false;
		requestSuspendCoronaRuntime();
	}
	
	/**
	 * This method is called when backing out of this 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> or when displaying another 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * <p>
	 * <b>This method is NOT called when the user presses the power button or during screen lock.</b>
	 * Because of this behavior, you never want to use this method to suspend the app.
	 */
    @Override
	protected void onStop() {
		super.onStop();
	}

	/**
	 * This method is called when this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>
	 * is resumed with a new <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a>.
	 * This happens when the <a href="http://developer.android.com/reference/android/content/Context.html#startActivity(android.content.Intent)">startActivity()</a> or 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#setIntent(android.content.Intent)">setIntent()</a> 
	 * methods get called for this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * <p>
	 * Note that the {@link com.ansca.corona.CoronaActivity#getIntent() getIntent()} method will return the 
	 * <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> given to this method.
	 * @param intent The new <a href="http://developer.android.com/reference/android/content/Intent.html">intent</a> that has started
	 *				 this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 *               Typically indicates what this 
	 *				 <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> should display to the user.
	 */
	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		
		// Validate.
		if (intent == null || fCoronaRuntime == null) {
			return;
		}

		// Ignore the intent if it contains no values. This typically happens when this activity is resumed
		// by the end-user in which case the activity should display the last screen shown.
		android.os.Bundle bundle = intent.getExtras();
		if ((intent.getData() == null) &&
		    ((bundle == null) || (bundle.size() <= 0)) &&
		    ((intent.getAction() == null) || intent.getAction().equals(Intent.ACTION_MAIN))) {
			return;
		}

		// Ignore the intent if it is for a notification.
		// This is to match iOS behavior which does not raise an "applicationOpen" event for notifications.
		String notificationEventName = com.ansca.corona.events.NotificationReceivedTask.NAME;
		if (intent.hasExtra(notificationEventName)) {
			return;
		}

		// Store the given intent.
		setIntent(intent);

		// Raise an "applicationOpen" event to be recevied by the Corona runtime.
		EventManager eventManager = fCoronaRuntime.getController().getEventManager();
		if (eventManager != null) {
			eventManager.addEvent(new com.ansca.corona.events.RunnableEvent(new Runnable() {
				@Override
				public void run() {
					JavaToNativeShim.applicationOpenEvent(fCoronaRuntime);
				}
			}));
		}

		// Pass the intent to each OnNewIntentResultHandler.
		for (CoronaActivity.OnNewIntentResultHandler handler : fNewIntentResultHandlers) {
			handler.onHandleNewIntentResult(intent);
	}
	}

	/**
	 * This method is called when this <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> 
	 * has gained or lost the focus.
	 * <p>
	 * The <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> window can lose the focus for the
	 * following reasons:
	 * <ul>
	 *  <li> This window is no longer visible because another 
	 *		 <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> was shown.
	 *  <li> This window is no longer visible due to screen lock or the press of the power button.
	 *  <li> A dialog is displayed on top of this window, such as an alert, but this window is still visible.
	 * </ul>
	 * @param hasFocus Set true when this window has gained the focus. Set false when it has lost it.
	 */
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		
		if (hasFocus) {
			// *** The activity window now has the focus and is visible to the user. ***
		}
		else {
			// *** The activity window has lost the focus. ***
		}
	}
	
	/**
	 * Called by Corona's SystemMonitor object indicating when the screen is or isn't locked.
	 * @param isScreenLocked Set true if the screen is currently locked or powered off.
	 *                       Set false if user can interact with screen.
	 */
	void onScreenLockStateChanged(boolean isScreenLocked) {
		// Show/hide this activity's views.
		if (myIsActivityResumed) {
			fCoronaRuntime.updateViews();
		}

		// Resume the Corona runtime if the screen lock is no longer shown on top of this activity.
		requestResumeCoronaRuntime();
	}

	/** Starts/resumes the Corona runtime, but only if this activity's window is visible to the user. */
	private void requestResumeCoronaRuntime() {
		// Do not continue if this activity has not been resumed yet.
		if (myIsActivityResumed == false) {
			return;
		}

		// Do not continue if the controller is no longer available. (Probably terminating.)
		if (fController == null) {
			Log.i("Corona", "ERROR: CoronaActivity.requestResumeCoronaRuntime(): " + 
					"Can't resume the CoronaRuntime because our Controller died!");
			return;
		}

		// Do not continue if the screen lock is currently being displayed.
		SystemMonitor systemMonitor = fController.getSystemMonitor();
		if ((systemMonitor != null) && systemMonitor.isScreenLocked()) {
			return;
		}

		// Start/resume the Corona runtime.
		fController.start();

		// Start/resume the GLSurfaceView.
		if (myGLView != null) {
			myGLView.onResumeCoronaRuntime();
		}

		// Resume all views belonging to the Corona runtime.
		ViewManager viewManager = null;
		if (fCoronaRuntime != null) {
			viewManager = fCoronaRuntime.getViewManager();
		}
		if (viewManager != null) {
			viewManager.resume();
		}
		else {
			Log.i("Corona", "ERROR: CoronaActivity.onResume(): Can't resume the CoronaActivity's views since there's no ViewManager!");
		}
	}
	
	/** Suspends the Corona runtime. To be called when this activity's window is no longer visible. */
	private void requestSuspendCoronaRuntime() {
		// Suspend the Corona runtime.
		if (fController != null) {
			fController.stop();
		}
		else {
			Log.i("Corona", "ERROR: CoronaActivity.requestSuspendCoronaRuntime(): " + 
					"Can't suspend the CoronaRuntime because our Controller died!");
		}

		// Suspend the runtime's GLSurfaceView.
		if (myGLView != null) {
			myGLView.onSuspendCoronaRuntime();
		}

		// Suspend all views belonging to the Corona runtime.
		ViewManager viewManager = null;
		if (fCoronaRuntime != null) {
			viewManager = fCoronaRuntime.getViewManager();
		}
		if (viewManager != null) {
			viewManager.suspend();
		}
		else {
			Log.i("Corona", "ERROR: CoronaActivity.onPause(): Can't suspend the CoronaActivity's views since there's no ViewManager!");
		}
	}
	
	/* No longer used but needs to exist to satisfy external requirements */
	void showSplashScreen() {
	}

	void showCoronaSplashScreen() {

		// Skip splash screens on low memory devices
        if (Runtime.getRuntime().maxMemory() <= (32 * 1024 * 1024))
		{
            Log.v("Corona", "Not enough memory to show splash screen");

			return;
        }

		// After trying many ResourceManager-based methods, the only reliable way to determine
		// whether the splash screen is in the app bundle appears to be to query the APK contents
		android.content.Context context = CoronaEnvironment.getApplicationContext();
		android.content.res.Resources resources = context.getResources();
		com.ansca.corona.storage.FileServices fileServices;
		fileServices = new com.ansca.corona.storage.FileServices(context);
		boolean splashExists = fileServices.doesResourceFileExist("drawable/_corona_splash_screen.png")
				            || fileServices.doesResourceFileExist("drawable/_corona_splash_screen.jpg");

		// Log.v("Corona", "showCoronaSplashScreen: splashExists: " + splashExists);
		if ( splashExists )
		{
			final ViewManager viewManager = fCoronaRuntime.getViewManager();
			int splash_drawable_id = 0;

			try {
				// Fetch the specified resource's unique integer ID by its name.
				splash_drawable_id = resources.getIdentifier("_corona_splash_screen" , "drawable", context.getPackageName());
			}
			catch (Exception ex) {
				Log.v("Corona", "showCoronaSplashScreen load EXCEPTION: " + ex);
			}

			// Log.v("Corona", "showCoronaSplashScreen: splash_drawable_id: " + splash_drawable_id);
			if ( splash_drawable_id != 0 )
			{
				try {
					// LinearLayout
					fSplashView = new LinearLayout(this);
					fSplashView.setOrientation(LinearLayout.VERTICAL);
					fSplashView.setBackgroundColor(0xFF000000);

					// ImageView
					ImageView imageView = new ImageView(this);
					imageView.setScaleType(ImageView.ScaleType.CENTER);

					// Resize the bitmap
					android.view.Display display =
						((android.view.WindowManager)getSystemService(android.content.Context.WINDOW_SERVICE)).getDefaultDisplay();
					Bitmap d = BitmapFactory.decodeResource(resources, splash_drawable_id);

					if (display.getWidth() >= d.getWidth() && display.getHeight() >= d.getHeight())
					{
						// Screen is bigger that the splash screen, we don't scale up so just display it
						// Log.v("Corona", "showCoronaSplashScreen: not scaling: screen: "+ display.getWidth() +"x"+ display.getHeight() + "; original: "+ d.getWidth() +"x"+ d.getHeight());

						imageView.setImageBitmap(d);
					}
					else
					{
						// Scaling the image ourselves avoids issues with "Bitmap too large to be uploaded into a texture" errors

						double widthRatio = ((double)display.getWidth() / d.getWidth());
						double heightRatio = ((double)display.getHeight() / d.getHeight());
						int nw = 0;
						int nh = 0;

						// Scale by height or width, whichever is greater
						if (heightRatio > widthRatio)
						{
							nw = (int) ((double)d.getWidth() * widthRatio);
							nh = (int) ((double)d.getHeight() * widthRatio);
						}
						else
						{
							nw = (int) ((double)d.getWidth() * heightRatio);
							nh = (int) ((double)d.getHeight() * heightRatio);
						}

						Bitmap scaled = Bitmap.createScaledBitmap(d, nw, nh, true);

						// Log.v("Corona", "showCoronaSplashScreen: scaling: screen: "+ display.getWidth() +"x"+ display.getHeight() + "; original: "+ d.getWidth() +"x"+ d.getHeight()+ "; scaled: "+ scaled.getWidth() +"x"+ scaled.getHeight());
						// Log.v("Corona", "showCoronaSplashScreen: new: "+ nw +"x"+ nh +"; ratio: "+ widthRatio + "x" + heightRatio);

						imageView.setImageBitmap(scaled);
					}

					FrameLayout.LayoutParams layoutParams;
					layoutParams = new FrameLayout.LayoutParams(
							FrameLayout.LayoutParams.FILL_PARENT,
							FrameLayout.LayoutParams.FILL_PARENT,
							android.view.Gravity.CENTER_HORIZONTAL | android.view.Gravity.CENTER_VERTICAL);

					imageView.setLayoutParams(layoutParams);

					fSplashView.addView(imageView);
					// Make visible
					viewManager.getContentView().addView(fSplashView, layoutParams);
				}
				catch (Exception ex) {
					Log.v("Corona", "showCoronaSplashScreen display EXCEPTION: " + ex);
				}
			}
		}
	}

	/** Hides the splash screen if currently displayed. */
	/* Called once Corona is ready to render its first frame */
	void hideSplashScreen() {
		if (fSplashView != null)
		{
			long timeToWait = this.SPLASH_SCREEN_DURATION - (System.currentTimeMillis() - fStartTime);

			if (timeToWait > 0)
			{
				// Log.v("Corona", "===============> Showing Splash for a further " + timeToWait + "ms");
				try {
					synchronized(this){
						// Ugly but the Corona runtime continues when this function returns so we
						// have to delay that until the splash has been shown for long enough
						wait(timeToWait);
					}
				}
				catch(InterruptedException ex) {}
			}

			// Animate fade of splash view
			AlphaAnimation opacityAnim = new AlphaAnimation(1.0f, 0.0f);
			opacityAnim.setDuration(500);
			fSplashView.startAnimation(opacityAnim);

			// Remove the splash view at the end of fade animation.
			// Also, null out our splash reference so that the view and its image can be garbage collected.
			final LinearLayout splashView = fSplashView;
			fSplashView = null;
			splashView.postDelayed(new Runnable() {
				@Override
				public void run() {
					android.view.ViewGroup parent = (android.view.ViewGroup)splashView.getParent();
					if (parent != null) {
						parent.removeView(splashView);
					}
				}
			}, 500);

		}
	}

	/**
	 * Determines if the splash screen is currently shown onscreen.
	 * @return Returns true if the splash screen is currently
	 */
	boolean isSplashScreenShown() {
		return (fSplashView != null);
	}

	private boolean canWriteToExternalStorage() {
		String permissionName = android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
		if (checkCallingOrSelfPermission(permissionName) == android.content.pm.PackageManager.PERMISSION_GRANTED 
			&& android.os.Environment.MEDIA_MOUNTED.equals(android.os.Environment.getExternalStorageState())) {
			return true;
		}
		return false;
	}

	/**
	 * Displays a "Send Mail" activity using the given settings.
	 * @param settings Initializes e-mail window's To, CC, Subject, Body text, and other fields.
	 */
	void showSendMailWindowUsing(MailSettings settings) {
		// Use default mail settings if given an invalid settings object.
		if (settings == null) {
			settings = new MailSettings();
		}
		
		// Display the mail activity.
		android.content.Intent intent = settings.toIntent();
		int requestCode = registerActivityResultHandler(new PopupActivityResultHandler("mail"));
		startActivityForResult(intent, requestCode);
	}
	
	/**
	 * Displays a "Send SMS" activity using the given settings.
	 * @param settings Initializes SMS window's recipients and text field.
	 */
	void showSendSmsWindowUsing(SmsSettings settings) {
		// Use default SMS settings if given an invalid settings object.
		if (settings == null) {
			settings = new SmsSettings();
		}
		
		// Display the SMS activity.
		android.content.Intent intent = settings.toIntent();
		int requestCode = registerActivityResultHandler(new PopupActivityResultHandler("sms"));
		startActivityForResult(intent, requestCode);
	}

	/**
	 * Displays a "Request Permissions" dialog using the given settings.
	 * @param settings Initializes Request Permissions window's permissions and rationale.
	 */
	void showRequestPermissionsWindowUsing(PermissionsSettings settings) {
		// Don't show the dialog if given invalid arguments or not on an API level that does runtime permissions.
		if (settings == null || android.os.Build.VERSION.SDK_INT < 23) {
			Log.v("Corona", "Cannot request permissions. Invalid environment!");
			return;
		}

		java.util.LinkedHashSet<String> appPermissions = settings.getPermissions();

		// Don't show the dialog if no permissions are requested!
		if (appPermissions == null || appPermissions.isEmpty()) {
			Log.v("Corona", "No App Permissions requested!");
			return;
		}
		
		// Convert PermissionsSettings from Lua to Android permissions.
		PermissionsServices permissionsServices = new PermissionsServices(CoronaEnvironment.getApplicationContext());
		java.util.LinkedHashSet<String> requestList = new java.util.LinkedHashSet<String>();
		for (String appPermission : appPermissions) {
			
			String[] permissionsToRequest;

			if (permissionsServices.isPAAppPermissionName(appPermission)) {
				// Translate platform-agnostic app permission name to Android permission groups.
				// We grab all permissions from the desired permission group and add them to the request list.
				String permissionGroup = permissionsServices.getPermissionGroupFromPAAppPermissionName(appPermission);
				permissionsToRequest = permissionsServices.findAllPermissionsInManifestForGroup(permissionGroup);

				if (permissionsToRequest == null || permissionsToRequest.length <= 0) {
					// The developer forgot to add any permission from this group to their build.settings/AndroidManifest.xml.
					showPermissionGroupMissingFromManifestAlert(permissionGroup);
					return;
				}

			} else if (permissionsServices.isSupportedPermissionGroupName(appPermission)) {
				// Grab all requested permissions in the group and add them to the request list!
				permissionsToRequest = permissionsServices.findAllPermissionsInManifestForGroup(appPermission);

				if (permissionsToRequest == null || permissionsToRequest.length <= 0) {
					// The developer forgot to add any permission from this group to their build.settings/AndroidManifest.xml.
					showPermissionGroupMissingFromManifestAlert(appPermission);
					return;
				}
			
			} else {
				// This is already an Android permission. So just add it to the request list!
				// TODO: SEE IF THE WAY THIS FAILS WHEN GIVEN GARBAGE IS SUFFICIENT!
				permissionsToRequest = new String[]{appPermission};
			}

			// Add all Android permissions gathered this pass to the request list.
			for (int permissionIdx = 0; permissionIdx < permissionsToRequest.length; permissionIdx++) {
				requestList.add(permissionsToRequest[permissionIdx]);
			}
		}

		// Lock in our request list.
		settings.setPermissions(requestList);

		// Request the desired permissions.
		permissionsServices.requestPermissions(settings, new DefaultRequestPermissionsResultHandler());
	}

	/**
	 * Handles the activity result from a "mail" or "sms" popup activity.
	 * <p>
	 * This class is only used by the showSendMailWindowUsing() and showSendSmsWindowUsing() methods.
	 */
	private class PopupActivityResultHandler implements CoronaActivity.OnActivityResultHandler {
		/** The unique name of the popup, such as "mail" or "sms". */
		private String fPopupName;

		/**
		 * Creates a new activity result handler.
		 * @param The unique name of the popup, such as "mail" or "sms".
		 */
		private PopupActivityResultHandler(String popupName) {
			fPopupName = popupName;
		}

		/**
		 * Handles the result received from a "mail" or "sms" popup.
		 * @param activity The Corona activity that is receiving the result.
		 * @param requestCode The integer request code originally supplied to startActivityForResult().
		 * @param resultCode The integer result code returned by the child activity via its setResult() method.
		 * @param data An Intent object which can return result data to the caller. Can be null.
		 */
		@Override
		public void onHandleActivityResult(
			CoronaActivity activity, int requestCode, int resultCode, android.content.Intent data)
		{
			// Unregister this handler.
			activity.unregisterActivityResultHandler(this);

			 if (fCoronaRuntime == null)
			 {
			 return;
			 }

			// We've returned from the mail or sms activity. Send an event to the Lua listener.
			// Note: Mail and SMS activities always returns "canceled" for its result code, even after a
			//       successful send. So, we have no choice but to indicate that it was not canceled.
			com.ansca.corona.events.EventManager eventManager = fCoronaRuntime.getController().getEventManager();
			if (eventManager != null) {
				final String popupName = fPopupName;
				eventManager.addEvent(new com.ansca.corona.events.RunnableEvent(new java.lang.Runnable() {
					@Override
					public void run() {
						if (fCoronaRuntime.getController() != null) {
							JavaToNativeShim.popupClosedEvent(fCoronaRuntime, popupName, false);
						}
					}
				}));
			}
		}
	}
	
	/**
	 * Called when an <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> you launched exits,
	 * yielding the results of that <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a>.
	 * @param requestCode The integer request code originally supplied to <a href="http://developer.android.com/reference/android/app/Activity.html#startActivityForResult(android.content.Intent, int)">startActivityForResult()</a>.
	 * Allows you to identify which <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> 
	 * the result is coming from.
	 * @param resultCode The integer result code returned by the child 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html">activity</a> through its 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#setResult(int)">setResult()</a> method.
	 * @param data An <a href="http://developer.android.com/reference/android/content/Intent.html">Intent</a> object which can return
	 * result data to the caller. Can be null.
	 */
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		// Fetch the handler that was assigned the given request code.
		CoronaActivity.OnActivityResultHandler handler = null;
		handler = (CoronaActivity.OnActivityResultHandler)fActivityResultHandlers.get(Integer.valueOf(requestCode));

		// Do not continue if the given request code is unknown.
		if (handler == null) {
			return;
		}

		// Pass the result to its handler.
		handler.onHandleActivityResult(this, requestCode, resultCode, data);
	}

	/**
	 * Callback for the result from requesting permissions. This method is invoked for every call on 
	 * <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a>.
	 * This callback will only be invoked on Android 6.0 and above devices!
	 * <p>
	 * <b>Note:</b> It is possible that the permissions request interaction with the user is interrupted. In this case you will receive
	 * empty permissions and results arrays which should be treated as a cancellation.
	 * <p>
	 * <b>Added in <a href="http://developer.coronalabs.com/release-ent/2016/2828/">daily build 2016.2828</a></b>.
	 * @param requestCode The request code passed in <a href="http://developer.android.com/reference/android/app/Activity.html#requestPermissions(java.lang.String[], int)">requestPermissions()</a>.
	 * @param permissions The requested permissions. Never null.
	 * @param grantResults The grant results for the corresponding permissions which is either 
	 * <a href="http://developer.android.com/reference/android/content/pm/PackageManager.html#PERMISSION_GRANTED">PERMISSION_GRANTED</a>
	 * or 
	 * <a href="http://developer.android.com/reference/android/content/pm/PackageManager.html#PERMISSION_DENIED">PERMISSION_DENIED</a>. 
	 * Never null.
	 */
	@Override
	public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);

		// Fetch the handler that was assigned the given request code.
		CoronaActivity.OnRequestPermissionsResultHandler handler = null;
		handler = (CoronaActivity.OnRequestPermissionsResultHandler)fRequestPermissionsResultHandlers.get(Integer.valueOf(requestCode));

		// Do not continue if the given request code is unknown.
		if (handler == null) {
			return;
		}

		// Pass the result to its handler.
		handler.onHandleRequestPermissionsResult(this, requestCode, permissions, grantResults);

	}

	/**
	 * Called when a key has been pressed down.
	 * @param keyCode Unique integer ID of the key, matching a key code constant in the 
	 *				  <a href="http://developer.android.com/reference/android/view/KeyEvent.html">KeyEvent</a> class.
	 * @param event Provides all information about the key event such as the key pressed,
	 *              modifiers such as Shift/Ctrl, and the device it came from.
	 * @return Returns true if the key event was handled. Returns false if not.
	 */
	@Override
	public boolean onKeyDown(int keyCode, android.view.KeyEvent event) {
		// Send the key event to Corona's input handler and Lua listeners first, if not already received.
		// Note: The input handler is tied to this activity's root content view and will normally receive
		//       input events before the activity. However, the input handler will not receive key
		//       events if another view has the focus. So, this activity must pass key events to the
		//       input handler manually here in order to work-around this issue.
		boolean wasHandled = myInputHandler.handle(event);
		if (wasHandled) {
			return true;
		}

		// *** Corona's Lua listeners have not overriden the key event. ***

		// Handle volume up/down events ourselves.
		// We have to do this because this activity's ViewInputHandler typically steals this event
		// from the system and the Activity's default handler does not control volume.
		if ((keyCode == android.view.KeyEvent.KEYCODE_VOLUME_UP) ||
		    (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_DOWN)) {
			try {
				int audioDirection;
				if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_UP) {
					audioDirection = android.media.AudioManager.ADJUST_RAISE;
				}
				else {
					audioDirection = android.media.AudioManager.ADJUST_LOWER;
				}
				int flags = android.media.AudioManager.FLAG_SHOW_UI |
							android.media.AudioManager.FLAG_PLAY_SOUND |
							android.media.AudioManager.FLAG_VIBRATE;
				((android.media.AudioManager)getSystemService(AUDIO_SERVICE)).adjustSuggestedStreamVolume(
						audioDirection, getVolumeControlStream(), flags);
				return true;
			}
			catch (Exception ex) { }
		}
		
		// Perform the default handling for the key event.
		return super.onKeyDown(keyCode, event);
	}

	/**
	 * Called when a key has been released.
	 * @param keyCode Unique integer ID of the key, matching a key code constant in the 
	 *				  <a href="http://developer.android.com/reference/android/view/KeyEvent.html">KeyEvent</a> class.
	 * @param event Provides all information about the key event such as the key pressed,
	 *              modifiers such as Shift/Ctrl, and the device it came from.
	 * @return Returns true if the key event was handled. Returns false if not.
	 */
	@Override
	public boolean onKeyUp(int keyCode, android.view.KeyEvent event) {
		// Send the key event to Corona's input handler and Lua listeners first, if not already received.
		// Note: The input handler is tied to this activity's root content view and will normally receive
		//       input events before the activity. However, the input handler will not receive key
		//       events if another view has the focus. So, this activity must pass key events to the
		//       input handler manually here in order to work-around this issue.
		boolean wasHandled = myInputHandler.handle(event);
		if (wasHandled) {
			return true;
		}

		// Corona's Lua listeners have not overriden the key event.
		// Perform the default handling for the received event.
		return super.onKeyUp(keyCode, event);
	}


	/** Handles events for this Corona activity. */
	private static class EventHandler implements android.view.ViewTreeObserver.OnGlobalLayoutListener {
		/** Reference to the activity that owns this event handler. */
		private CoronaActivity fActivity;

		/** Set true if this handler is in the middle of updating the screen's layout. */
		private boolean fIsUpdatingLayout;

		/**
		 * If "fIsUpdatingLayout" is set to true, then this schedule when this event handler should
		 * stop updating the layout.
		 */
		private Ticks fUpdateLayoutEndTicks;


		/**
		 * Creates a new private event handler for the CoronaActivity.
		 * @param activity Reference to the CoronaActivity that owns this event handler.
		 *                 Cannot be null or else an exception will be thrown.
		 */
		public EventHandler(CoronaActivity activity) {
			// Validate.
			if (activity == null) {
				throw new NullPointerException();
			}

			// Initialize member variables.
			fActivity = activity;
			fIsUpdatingLayout = false;
			fUpdateLayoutEndTicks = Ticks.fromCurrentTime();

			// Subscribe to the GlobalLayout event, but only if the activity is not fullscreen.
			// We do this to work-around an Android rendering bug when the keyboard pans the app.
			int flags = fActivity.getWindow().getAttributes().flags;
			if ((flags & android.view.WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS) == 0) {
				android.view.View rootView = getContentView();
				if (rootView != null) {
					rootView.getViewTreeObserver().addOnGlobalLayoutListener(this);
				}
			}
		}

		/**
		 * Unsubscribes from events and disposes of this object's resources.
		 * Expected to be called by the activity's onDestroy() method.
		 */
		public void dispose() {
			// Unsubscribe from all events.
			android.view.View rootView = getContentView();
			if (rootView != null) {
				if (android.os.Build.VERSION.SDK_INT >= 16) {
					ApiLevel16.removeOnGlobalLayoutListener(rootView.getViewTreeObserver(), this);
				}
				else {
					rootView.getViewTreeObserver().removeGlobalOnLayoutListener(this);
				}
			}
		}

		/**
		 * Called when the activity window layou has changed.
		 * This can happen when it is resized or panned by the virtual keyboard, if enabled.
		 */
		@Override
		public void onGlobalLayout() {
			// Determine if the virtual keyboard is currently shown.
			String serviceName = android.content.Context.INPUT_METHOD_SERVICE;
			android.view.inputmethod.InputMethodManager inputMethodManager =
						(android.view.inputmethod.InputMethodManager)fActivity.getSystemService(serviceName);
			boolean isVirtualKeyboardVisible = inputMethodManager.isAcceptingText();

			// If the virtual keyboard is currently shown, then schedule this handler to redraw the UI
			// to work-around an Android rendering bug where native UI displays glitchy when the app has been
			// panned by the keyboard.
			Ticks currentTicks = Ticks.fromCurrentTime();
			if (isVirtualKeyboardVisible) {
				fIsUpdatingLayout = true;
				fUpdateLayoutEndTicks = currentTicks.addSeconds(2);
			}

			// Do not continue if we do not need to redraw the native UI.
			if (fIsUpdatingLayout && (fUpdateLayoutEndTicks.compareTo(currentTicks) < 0)) {
				fIsUpdatingLayout = false;
			}
			if (fIsUpdatingLayout == false) {
				return;
			}

			// Re-draw and re-layout all views.
			android.view.View rootView = getContentView();
			if (rootView != null) {
				rootView.requestLayout();
			}
		}

		/**
		 * Fetches the activity's root content view.
		 * @return Returns a reference to the activity's root content view.
		 *         <p>
		 *         Returns null if the root view is no longer available, which can happen if
		 *         the activity has been destroyed.
		 */
		private android.view.View getContentView() {
			CoronaRuntime runtime = fActivity.fCoronaRuntime;
			if (runtime != null) {
				ViewManager viewManager = runtime.getViewManager();
				if (viewManager != null) {
					android.view.View rootView = viewManager.getContentView();
					if (rootView != null) {
						return rootView;
					}
				}
			}
			return null;
		}
	}

	/** Default handling of permissions on Android 6+. 
	 * <!-- TODO: CLEAN THIS UP SINCE IT'LL BE AVAILABLE FOR ENTERPRISE DEVS TO CREATE!
	 * OPEN THIS UP TO ENTERPRISE WHEN READY!--> */
	static class DefaultRequestPermissionsResultHandler 
		implements CoronaActivity.OnRequestPermissionsResultHandler {

		/** TODO: Have this use the CoronaData class instead, since it's well-exercised code!
		 * Creates a Lua table out of an array of strings.
		 * Leaves the Lua table on top of the stack.
	     */
	    private static int createLuaTableFromStringArray(com.naef.jnlua.LuaState L, String[] array) {

	        L.newTable(array.length, 0);
	        for (int i = 0; i < array.length; i++) {
	            // Push this string to the top of the stack
	            L.pushString(array[i]);

	            // Assign this string to the table 2nd from the top of the stack.
	            // Lua arrays are 1-based so add 1 to index correctly.
	            L.rawSet(-2, i + 1);
	        }

	        // Result is on top of the lua stack.
	        return 1;
	    }

	    /**
	     * Send the results of this permission request to Lua.
	     * <!--TODO: LOTS OF THINGS IN HERE SHOULD REALLY BE NULL-CHECKED!-->
	     */
	    public void forwardRequestPermissionsResultToLua(final RequestPermissionsResultData data) {
    		// Invoke the lua listener if PermissionSettings are provided.
			CoronaRuntimeTask invokeListenerTask = new CoronaRuntimeTask() {
				@Override
				public void executeUsing(CoronaRuntime runtime) {
					try {
						// Fetch the Corona runtime's Lua state.
						com.naef.jnlua.LuaState L = runtime.getLuaState();

						// Dispatch the lua callback
						int luaListenerRegistryId = data.getPermissionsSettings().getListener();
						if (CoronaLua.REFNIL != luaListenerRegistryId && 0 != luaListenerRegistryId) {
							// Setup the event
							CoronaLua.newEvent( L, "popup" );

							// Event type
							L.pushString( "appPermissionRequest" );
							L.setField( -2, "type" );

							// Convert the permission data to lua tables to send over
							// Granted permissions
							java.util.ArrayList<String> grantedPermissions = data.getGrantedPermissions();
							if (grantedPermissions != null) {
								Object[] grantedPermissionsArray = grantedPermissions.toArray();
								if (createLuaTableFromStringArray(L, java.util.Arrays.copyOf(
										grantedPermissionsArray, grantedPermissionsArray.length, String[].class)) > 0) {
									L.setField(-2, "grantedAppPermissions");
								}
							} else {
								// Push an empty table for grantedAppPermissions
								L.newTable(0, 0);
								L.setField(-2, "grantedAppPermissions");
							}

							// Denied permissions
							java.util.ArrayList<String> deniedPermissions = data.getDeniedPermissions();
							if (deniedPermissions != null) {
								Object[] deniedPermissionsArray = deniedPermissions.toArray();
								if (createLuaTableFromStringArray(L, java.util.Arrays.copyOf(
										deniedPermissionsArray, deniedPermissionsArray.length, String[].class)) > 0) {
									L.setField(-2, "deniedAppPermissions");
								}
							} else {
								// Push an empty table for deniedAppPermissions
								L.newTable(0, 0);
								L.setField(-2, "deniedAppPermissions");
							}

				            // Never Ask Again flag
				            L.pushBoolean( data.getUserHitNeverAskAgain() );
				            L.setField( -2, "neverAskAgain");

							// Dispatch the event
							CoronaLua.dispatchEvent( L, luaListenerRegistryId, 0 );
						} else {
							Log.i("Corona", "ERROR: CoronaActivity.DefaultRequestPermissiosnResultHandler.forwardRequestPermissionsResultToLua():" + 
								"Cannot forward results to Lua as no registry ID was found!");
						}
					}
					catch ( Exception ex ) { 
						ex.printStackTrace(); 
					}
				}
			};
			// TODO: Null checks on things!
			// Dispatch this task.
			android.util.Log.d("Corona", "Execute the lua listener task");
			data.getCoronaActivity().getRuntimeTaskDispatcher().send(invokeListenerTask);

			// Force this task to execute this frame in case we have no other reason to request a render
			data.getCoronaActivity().getRuntime().getController().getEventManager().sendEvents();
			data.getPermissionsSettings().markAsServiced();
	    }

		@Override
		public void onHandleRequestPermissionsResult(
				CoronaActivity activity, int requestCode, String[] permissions, int[] grantResults) {
			PermissionsServices permissionsServices = new PermissionsServices(activity);
			
			// Seperate out the granted and denied permissions.
			java.util.ArrayList<String> androidGrantedPermissions = new java.util.ArrayList<String>();
			final java.util.ArrayList<String> grantedPermissions = new java.util.ArrayList<String>();
			java.util.ArrayList<String> androidDeniedPermissions = new java.util.ArrayList<String>();
			final java.util.ArrayList<String> deniedPermissions = new java.util.ArrayList<String>();

			for (int permissionIdx = 0; permissionIdx < permissions.length; permissionIdx++) {
				
				// Convert permissions to the appropriate platform-agnostic app permission names if needed.
				String currentPermission = permissions[permissionIdx];
				if (permissionsServices.isPartOfPAAppPermission(currentPermission)) {
					currentPermission = permissionsServices.getPAAppPermissionNameFromAndroidPermission(currentPermission);
				}

				// Now, categorize the permissions.
				if (grantResults[permissionIdx] == android.content.pm.PackageManager.PERMISSION_GRANTED) {
					
					androidGrantedPermissions.add(permissions[permissionIdx]);
					// Only add to platform-agnostic app permission list if this isn't a duplicate.
					if (!grantedPermissions.contains(currentPermission) 
						&& !deniedPermissions.contains(currentPermission)) {
						grantedPermissions.add(currentPermission);
					}
				} else { // The permission was denied!
					
					androidDeniedPermissions.add(permissions[permissionIdx]);
					// Only add to platform-agnostic app permission list if this isn't a duplicate.
					if (!grantedPermissions.contains(currentPermission) 
						&& !deniedPermissions.contains(currentPermission)) {
						deniedPermissions.add(currentPermission);
					}
				}
			}

			// Finalize data to be accessed on GLThread.
			final PermissionsSettings permissionsSettings = activity.unregisterRequestPermissionsResultHandler(this);

			if (permissionsSettings != null) {

				boolean userHitNeverAskAgain = false;

				RequestPermissionsResultData requestPermissionsResultData = null;

				// Present rationale for these permissions if needed.
				if (!androidDeniedPermissions.isEmpty()) {
					com.ansca.corona.permissions.PermissionUrgency urgency = permissionsSettings.getUrgency();
					for (String deniedPermission : androidDeniedPermissions) {
						boolean showRequestPermissionRationale = activity.shouldShowRequestPermissionRationale(deniedPermission);

						if (urgency != com.ansca.corona.permissions.PermissionUrgency.LOW 
							&& showRequestPermissionRationale && permissionsSettings.needsService()) {
							
							// Gather data to send back to Lua.
							requestPermissionsResultData = new RequestPermissionsResultData(
								permissionsSettings, grantedPermissions, deniedPermissions, false, requestCode, activity, this);
							
							// We show the permission rationale dialog.
							activity.getRuntime().getController().showPermissionRationaleAlert
								(deniedPermission, requestPermissionsResultData);
							permissionsSettings.markAsServiced();
							return;
						} else if (!showRequestPermissionRationale) { // TODO: Verify the safety of this conditional.
							
							// The user hit the "Never Ask Again button".
							if (urgency == com.ansca.corona.permissions.PermissionUrgency.CRITICAL) {

								// Gather data to send back to Lua.
								requestPermissionsResultData = new RequestPermissionsResultData(
									permissionsSettings, grantedPermissions, deniedPermissions, true, requestCode, activity, this);

								// We present the Settings Redirect dialog.
								activity.getRuntime().getController().showSettingsRedirectForPermissionAlert
									(deniedPermission, requestPermissionsResultData);
								permissionsSettings.markAsServiced();
								return;
							} else {
								// Let the Lua side know that the user doesn't want to be bothered by permission requests!
								userHitNeverAskAgain = true;
							}
						}
					}
				}

				// Send out results back to Lua!
				requestPermissionsResultData = new RequestPermissionsResultData(
					permissionsSettings, grantedPermissions, deniedPermissions, userHitNeverAskAgain, requestCode, activity, this);
				forwardRequestPermissionsResultToLua(requestPermissionsResultData);

			} else {
				// TODO: We don't have a Lua listener to invoke and give the user an update on the status of permissions! Say something!
			}
		}
	}


	/**
	 * Provides access to API Level 16 (Jelly Bean) features.
	 * Should only be accessed if running on an operating system matching this API Level.
	 * <p>
	 * You cannot create instances of this class.
	 * You are expected to call its static methods instead.
	 */
	private static class ApiLevel16 {
		/** Constructor made private to prevent instances from being made. */
		private ApiLevel16() {}

		/**
		 * Removes the given listener from the ViewTreeObserver.
		 * @param viewTreeObserver The object to remove the listener from. Can be null.
		 * @param listener The listener reference to be removed. Can be null.
		 */
		public static void removeOnGlobalLayoutListener(
			android.view.ViewTreeObserver viewTreeObserver,
			android.view.ViewTreeObserver.OnGlobalLayoutListener listener)
		{
			// Validate.
			if ((viewTreeObserver == null) || (listener == null)) {
				return;
			}

			// Remove the listener.
			try {
				viewTreeObserver.removeOnGlobalLayoutListener(listener);
			}
			catch (Exception ex) {}
		}
	}
}
