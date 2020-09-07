//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#include "Core/Rtt_Build.h"

#include "AndroidGLView.h"
#include "NativeToJavaBridge.h"

#include "Rtt_Runtime.h"
#include "Rtt_AndroidBitmap.h"
#include "Rtt_RenderingStream.h"

#include "Rtt_Event.h"
#include "Rtt_GPU.h"

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <android/log.h>

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// At some point in the future, these could implement lower level GL drawing, but
// we're going to go with the default for now.

void
AndroidGLView::Flush()
{
	fNativeToJavaBridge->DisplayUpdate();
}

bool 
AndroidGLView::CreateFramebuffer( int width, int height )
{
	fWidth = width;
	fHeight = height;
	return true;
}

void 
AndroidGLView::DestroyFramebuffer()
{
}

void 
AndroidGLView::Render()
{
	if ( fRuntime )
	{
		(*fRuntime)();
	}
}

void
AndroidGLView::Resize( int width, int height )
{
	fWidth = width;
	fHeight = height;
}

/// Determines if this OpenGL view is set up with an alpha channel.
/// This is determined by calling glGetIntegerv(GL_ALPHA_BITS, value).
/// @return Returns true if this view supports alpha. Returns false if not.
bool
AndroidGLView::HasAlphaChannel()
{
	GLint alphaBits = 0;
	glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
	return (alphaBits > 0);
}

// ----------------------------------------------------------------------------
	
// ----------------------------------------------------------------------------

