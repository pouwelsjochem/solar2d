//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_BitmapUtils_H
#define _Rtt_BitmapUtils_H

#include <stdio.h>
#include "Display/Rtt_PlatformBitmap.h"

namespace bitmapUtil
{
	uint8_t* loadPNG(FILE* infile, int& w, int& h);
	char* savePNG(size_t& length, uint8_t* data, int width, int height, Rtt::PlatformBitmap::Format format);
};

#endif		// _Rtt_BitmapUtils_H
