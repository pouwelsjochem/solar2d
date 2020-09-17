//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rtt_WinScreenSurface.h"
#include "Core\Rtt_Build.h"
#include "Interop\UI\RenderSurfaceControl.h"
#include "Interop\UI\Window.h"
#include "Interop\MDeviceSimulatorServices.h"
#include "Interop\RuntimeEnvironment.h"
#include "Rtt_NativeWindowMode.h"


namespace Rtt
{

WinScreenSurface::WinScreenSurface(Interop::RuntimeEnvironment& environment)
:	Super(),
	fEnvironment(environment),
	fPreviousClientWidth(0),
	fPreviousClientHeight(0)
{
}

WinScreenSurface::~WinScreenSurface()
{
}

void WinScreenSurface::SetCurrent() const
{
	auto surfaceControlPointer = fEnvironment.GetRenderSurface();
	if (surfaceControlPointer)
	{
		surfaceControlPointer->SelectRenderingContext();
	}
}

void WinScreenSurface::Flush() const
{
	auto surfaceControlPointer = fEnvironment.GetRenderSurface();
	if (surfaceControlPointer)
	{
		surfaceControlPointer->SwapBuffers();
	}
}

S32 WinScreenSurface::Width() const
{
	// Return zero if we do not have a surface to render to.
	auto renderSurfacePointer = fEnvironment.GetRenderSurface();
	if (!renderSurfacePointer)
	{
		return 0;
	}

	// Fetch the surface's client width in pixels.
	int width = 0;
	auto windowPointer = fEnvironment.GetMainWindow();
	if (windowPointer && windowPointer->GetWindowMode().Equals(Rtt::NativeWindowMode::kMinimized))
	{
		// The window hosting the surface has been minimized, causing the client area to have a zero width/height.
		// Use the client length before it was minimized to avoid triggering an unnecessary "resize" event in Corona.
		width = fPreviousClientWidth;
	}
	else
	{
		// Fetch the requested length and store it in case the window gets minimized later.
		width = renderSurfacePointer->GetClientWidth();
		fPreviousClientWidth = width;
	}

	// Corona's rendering system will assert if given a zero length. So, floor it to 1.
	if (width <= 0)
	{
		width = 1;
	}

	return width;
}

S32 WinScreenSurface::Height() const
{
	// Return zero if we do not have a surface to render to.
	auto renderSurfacePointer = fEnvironment.GetRenderSurface();
	if (!renderSurfacePointer)
	{
		return 0;
	}

	// Fetch the surface's client height in pixels.
	int height = 0;
	auto windowPointer = fEnvironment.GetMainWindow();
	if (windowPointer && windowPointer->GetWindowMode().Equals(Rtt::NativeWindowMode::kMinimized))
	{
		// The window hosting the surface has been minimized, causing the client area to have a zero width/height.
		// Use the client length before it was minimized to avoid triggering an unnecessary "resize" event in Corona.
		height = fPreviousClientHeight;
	}
	else
	{
		// Fetch the requested length and store it in case the window gets minimized later.
		height = renderSurfacePointer->GetClientHeight();
		fPreviousClientHeight = height;
	}

	// Corona's rendering system will assert if given a zero length. So, floor it to 1.
	if (height <= 0)
	{
		height = 1;
	}

	return height;
}

}	// namespace Rtt
