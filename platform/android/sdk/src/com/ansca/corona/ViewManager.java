//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import com.ansca.corona.permissions.PermissionsServices;
import com.ansca.corona.permissions.PermissionsSettings;
import com.ansca.corona.permissions.PermissionState;

import java.io.File;
import java.util.HashMap;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.webkit.CookieManager;
import android.widget.AbsoluteLayout;
import android.widget.RelativeLayout;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;

public class ViewManager {
	private java.util.ArrayList<android.view.View> myDisplayObjects;
	private Context myContext;
	private ViewGroup myContentView;
	private android.widget.FrameLayout myOverlayView;
	private android.widget.AbsoluteLayout myAbsoluteViewLayout;
	private CoronaRuntime myCoronaRuntime;
	private Handler myHandler;
	
	/**
	 * HashMap derived class which uses type String for the keys and type Object for the values.
	 * Instances of this class are tagged to views for storing various information.
	 */
	private class StringObjectHashMap extends java.util.HashMap<String, Object> { }
	
	
	public ViewManager(Context context, CoronaRuntime runtime)
	{
		myContext = context;
		myCoronaRuntime = runtime;
		myDisplayObjects =  new java.util.ArrayList<android.view.View>();
		myContentView = null;
		myOverlayView = null;
		myAbsoluteViewLayout = null;

		// The main looper lives on the main thread of the application.  
		// This will bind the handler to post all runnables to the main ui thread.
		myHandler = new Handler(Looper.getMainLooper());
	}
	
	/**
	 * Gets the root view belonging to the CoronaActivity which was assigned to it via Activity.setContentView() method.
	 * This view contains all other views such as the OpenGL view, overlay view, and the absolute layout view.
	 * @return Returns the root view on the activity the contains all other views.
	 */
	public ViewGroup getContentView()
	{
		return myContentView;
	}
	
	/**
	 * Gets the view that is overlaid on top of the OpenGL view.
	 * UI objects such as text fields, ads, etc. should be added to this view.
	 * @return Returns a FrameLayout view. Returns null if its CoronaActivity is not available.
	 */
	public android.widget.FrameLayout getOverlayView()
	{
		return myOverlayView;
	}
	
	/**
	 * Gets an absolute layout view group contained with the overlay view.
	 * This view allows you to set pixel position of UI objects on top of the OpenGL view.
	 * @return Returns an AbsoluteLayout object. Returns null if its CoronaActivity is not available.
	 */
	public android.widget.AbsoluteLayout getAbsoluteViewLayout()
	{
		return myAbsoluteViewLayout;
	}
	
	/**
	 * Fetches a Corona display object by its unique ID.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns the specified display object. Returns null if not found.
	 */
	public android.view.View getDisplayObjectById(int id) {
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if ((view != null) && (view.getId() == id)) {
					return view;
				}
			}
		}
		return null;
	}
	
	/**
	 * Fetches a Corona display object by its unique ID.
	 * @param type The type of display object to look for. Must derive from the "View" class.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns the specified display object. Returns null if not found.
	 */
	public <T extends android.view.View> T getDisplayObjectById(Class<T> type, int id) {
		synchronized (myDisplayObjects) {
			for (android.view.View view : myDisplayObjects) {
				if (type.isInstance(view) && (view.getId() == id)) {
					return (T)view;
				}
			}
		}
		return null;
	}
	
	/**
	 * Determines if a display object having the given ID exists.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns true if a display object having the given ID exists. Returns false if not.
	 */
	public boolean hasDisplayObjectWithId(int id) {
		return (getDisplayObjectById(id) != null);
	}
	
	/**
	 * Determines if a display object having the given ID exists.
	 * @param type The type of display object to look for. Must derive from the "View" class.
	 * @param id The unique integer ID assigned to the object via the View.setId() method.
	 * @return Returns true if a display object having the given ID exists. Returns false if not.
	 */
	public <T extends android.view.View> boolean hasDisplayObjectWithId(Class<T> type, int id) {
		return (getDisplayObjectById(type, id) != null);
	}
	
	/**
	 * Posts the given Runnable object to the main UI thread's message queue in a thread safe manner.
	 * @param runnable The Runnable object to be posted and executed on the main UI thread.
	 */
	private void postOnUiThread(Runnable runnable) {
		// Validate.
		if (runnable == null) {
			return;
		}
		
		if (myHandler != null &&
			myHandler.getLooper() != null &&
			myHandler.getLooper().getThread() != null &&
			!myHandler.getLooper().getThread().isAlive()) {

			return;
		}
			
		myHandler.post(runnable);
	}
	
	public void suspend() {

	}

	public void resume() {

	}

	public void setGLView(View glView)
	{
		// Set up the root content view that will contain all other views.
		// A FrameLayout is best because its child views be rendered on top of each other in the order that they were added.
		// Make this view focusable so that we can clear the focus from native fields by setting the focus to the container.
		myContentView = new android.widget.FrameLayout(myContext);
		myContentView.setFocusable(true);
		myContentView.setFocusableInTouchMode(true);
		
		// Add the given OpenGL view to the root content view first.
		// This way it is at the back of the z-order so that all other views are overlaid on top.
		myContentView.addView(glView);
		
		// Add an invisible overlay view to the root content view.
		// This is a view container for all UI objects such as text fields, ads, etc.
		// Add an AbsoluteLayout to this overlay view, which can be used to set pixel positions of native fields.
		myOverlayView = new android.widget.FrameLayout(myContext);
		myAbsoluteViewLayout = new android.widget.AbsoluteLayout(myContext);
		myOverlayView.addView(myAbsoluteViewLayout);
		myContentView.addView(myOverlayView);
	}

	public void destroy() {
		destroyAllDisplayObjects();
	}
	
	public void destroyAllDisplayObjects() {
		View view;

		// Destroy all display objects in the collection.
		do {
			// Fetch the next display object.
			synchronized (myDisplayObjects) {
				if (myDisplayObjects.isEmpty()) {
					view = null;
				}
				else {
					view = myDisplayObjects.get(myDisplayObjects.size() - 1);
				}
			}

			// Destroy the display object.
			if (view != null) {
				destroyDisplayObject(view.getId());
			}
		} while (view != null);
	}
	
	public void destroyDisplayObject( final int id ) {
		// Fetch the display object.
		final View view = getDisplayObjectById(id);
		if (view == null) {
			return;
		}
		
		// Remove the display object from the collection before destroying it on the UI thread.
		synchronized (myDisplayObjects) {
			myDisplayObjects.remove(view);
		}
		
		// Remove the display object via the UI thread.
		postOnUiThread(new Runnable() {
			@Override
			public void run() {
				// Remove the view from the layout.
				android.view.ViewParent parentView = view.getParent();
				if ((parentView != null) && (parentView instanceof android.view.ViewGroup)) {
					((android.view.ViewGroup)parentView).removeView(view);
				}

				// Set the view's ID to an invalid value.
				// This causes any upcoming events that may get raised by the destroyed view to be ignored.
				// Some views send an event upon destruction, so changing this ID must come after destroying them.
				view.setId(0);
			}
		});
	}
	
	public void setDisplayObjectVisible( final int id, final boolean visible ) {
		postOnUiThread( new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if (view != null) {
					view.setVisibility(visible ? View.VISIBLE : View.GONE);
					if (visible) {
						setDisplayObjectAlpha(id, getDisplayObjectAlpha(id));
					}
					else {
						view.setAnimation(null);
					}
				}
			}
		} );
	}
	
	public void displayObjectUpdateScreenBounds(
		final int id, final int left, final int top, final int width, final int height )
	{
		postOnUiThread( new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if ( view != null ) {
					LayoutParams params = null;
					params = new AbsoluteLayout.LayoutParams( width, height, left, top );
					view.setLayoutParams(params);
				}
			}
		} );
	}
	
	public boolean getDisplayObjectVisible( int id ) {
		boolean result = false;
		
		View view = getDisplayObjectById(id);
		if ( view != null ) {
			result = (view.getVisibility() == View.VISIBLE);
		}
		return result;
	}
	
	public float getDisplayObjectAlpha(int id) {
		float alpha = 1.0f;
		
		View view = getDisplayObjectById(id);
		if (view != null) {
			Object tag = view.getTag();
			if (tag instanceof StringObjectHashMap) {
				Object value = ((StringObjectHashMap)tag).get("alpha");
				if (value instanceof Float) {
					alpha = ((Float)value).floatValue();
				}
			}
		}
		return alpha;
	}
	
	public void setDisplayObjectAlpha(final int id, final float alpha) {
		postOnUiThread(new Runnable() {
			public void run() {
				View view = getDisplayObjectById(id);
				if (view != null) {
					// Do not allow the alpha color to exceed its bounds.
					float newAlpha = alpha;
					if (newAlpha < 0) {
						newAlpha = 0;
					}
					else if (newAlpha > 1.0f) {
						newAlpha = 1.0f;
					}
					
					// Have the view store the new alpha value to be retrieved by the getDisplayObjectAlpha() method.
					Object tag = view.getTag();
					if (tag instanceof StringObjectHashMap) {
						((StringObjectHashMap)tag).put("alpha", Float.valueOf(newAlpha));
					}
					
					// Alpha blend the view via an animation object.
					if ((newAlpha < 0.9999f) && (view.getVisibility() == View.VISIBLE)) {
						android.view.animation.AlphaAnimation animation;
						animation = new android.view.animation.AlphaAnimation(1.0f, newAlpha);
						animation.setDuration(0);
						animation.setFillAfter(true);
						view.startAnimation(animation);
					}
					else {
						view.setAnimation(null);
					}
				}
			}
		});
	}
	
	public void setDisplayObjectBackground( final int id, final boolean isVisible ) {
		postOnUiThread( new Runnable() {
			public void run() {
				// Fetch the view.
				View view = getDisplayObjectById(id);
				if (view == null) {
					return;
				}
				
				// Fetch the view's background drawable object.
				// Note that it will be null if the background has been hidden.
				android.graphics.drawable.Drawable background = view.getBackground();
				
				// Do not continue if the background visibility state isn't going to changing.
				if ((isVisible && (background != null)) ||
				    (!isVisible && (background == null))) {
					return;
				}
				
				// If the background was not found, then attempt to fetch it from the HashMap tagged to the view.
				StringObjectHashMap hashMap = null;
				Object tag = view.getTag();
				if (tag instanceof StringObjectHashMap) {
					hashMap = (StringObjectHashMap)tag;
				}
				if ((background == null) && (hashMap != null)) {
					Object value = hashMap.get("backgroundDrawable");
					if (value instanceof android.graphics.drawable.Drawable) {
						background = (android.graphics.drawable.Drawable)value;
					}
				}
				
				// Hide the background by settings the background drawable to null.
				// Show the background by assigning the view the last displayed background.
				// If the last displayed background is unknown, then generate a new background drawable.
				if (isVisible && (background == null)) {
					view.setBackgroundColor(android.graphics.Color.WHITE);
					background = view.getBackground();
				}
				else {
					view.setBackgroundDrawable(isVisible ? background : null);
				}
				
				// If we are hiding the background, then store it to the HashMap to be restored later.
				if ((isVisible == false) && (hashMap != null)) {
					hashMap.put("backgroundDrawable", background);
				}
				
				// Redraw the view.
				view.invalidate();
			}
		} );
	}
	
	public boolean getDisplayObjectBackground(final int id) {
		boolean hasBackground = false;
		
		// Fetch the view.
		View view = getDisplayObjectById(id);
		if (view == null) {
			return false;
		}
		
		// Determine if there is a background by looking at its drawable object.
		android.graphics.drawable.Drawable background = view.getBackground();
		if (background instanceof android.graphics.drawable.ColorDrawable) {
			hasBackground = (((android.graphics.drawable.ColorDrawable)background).getAlpha() > 0);
		}
		else if (background instanceof android.graphics.drawable.ShapeDrawable) {
			android.graphics.Paint paint = ((android.graphics.drawable.ShapeDrawable)background).getPaint();
			if (paint != null) {
				hasBackground = (paint.getColor() != android.graphics.Color.TRANSPARENT);
			}
		}
		else if (background != null) {
			hasBackground = true;
		}
		return hasBackground;
	}
	
	private void setHardwareAccelerationEnabled(View view, boolean enabled) {
		// Validate.
		if (view == null) {
			return;
		}
		
		// Enable/disable hardware acceleration via reflection.
		try {
			java.lang.reflect.Method setLayerTypeMethod = android.view.View.class.getMethod(
					"setLayerType", new Class[] {Integer.TYPE, android.graphics.Paint.class});
			int layerType = enabled ? 2 : 1;
			setLayerTypeMethod.invoke(view, new Object[] {layerType, null});
		}
		catch (Exception ex) { }
	}
	
}
