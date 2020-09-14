//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

public class ViewManager {
	private Context myContext;
	private ViewGroup myContentView;
	private CoronaRuntime myCoronaRuntime;

	public ViewManager(Context context, CoronaRuntime runtime)
	{
		myContext = context;
		myCoronaRuntime = runtime;
		myContentView = null;
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
	}

}