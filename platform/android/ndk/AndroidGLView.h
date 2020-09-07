//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _AndroidGLView_H__
#define _AndroidGLView_H__

// ----------------------------------------------------------------------------

namespace Rtt
{
	class Runtime;
}	

class NativeToJavaBridge;

// ----------------------------------------------------------------------------

class AndroidGLView {
private:

	Rtt::Runtime * fRuntime;
	int fWidth;
	int fHeight;
	NativeToJavaBridge *fNativeToJavaBridge;


	
public:
	AndroidGLView()
	{
		fRuntime = NULL;
		fWidth = 0; // TODO: this is not right
		fHeight = 0;
		fNativeToJavaBridge = NULL;
	}
	
	void SetCallback( Rtt::Runtime *c )
	{
		fRuntime = c;
	}

	void SetNativeToJavaBridge( NativeToJavaBridge *ntjb )
	{
		fNativeToJavaBridge = ntjb;
	}
	
	void Flush();
	bool CreateFramebuffer( int width, int height );
	void DestroyFramebuffer();
	void Render();
	bool HasAlphaChannel();

	int Width() const
	{
		return fWidth;
	}

	int Height() const
	{
		return fHeight;
	}

	// Returns physical width and height of device screen when device
	int DeviceWidth() const { return fWidth; }
	int DeviceHeight() const { return fHeight; }

	void Resize( int width, int height );
};

// ----------------------------------------------------------------------------
	
#endif
