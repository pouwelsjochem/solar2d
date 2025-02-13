//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

//#include "Core/Rtt_Build.h"

#include "Display/Rtt_TextureResourceBitmap.h"

#include "Display/Rtt_Display.h"
#include "Display/Rtt_TextureFactory.h"
#include "Display/Rtt_PlatformBitmapTexture.h"
#include "Renderer/Rtt_TextureVolatile.h"
#include "Renderer/Rtt_TextureBitmap.h"

#include "Display/Rtt_BufferBitmap.h"
#include "Rtt_TextureResourceBitmapAdapter.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

	
TextureResourceBitmap *
TextureResourceBitmap::Create(
	TextureFactory& factory,
	int w, int h,
	Texture::Format format,
	Texture::Filter filter,
	Texture::Wrap wrap,
	bool save_to_file )
{
	Display& display = factory.GetDisplay();
	Texture *texture = Rtt_NEW( display.GetAllocator(),
		TextureVolatile( display.GetAllocator(), w, h, format, filter, wrap, wrap ) );

	BufferBitmap *bitmap = NULL;

	if( save_to_file )
	{
		bitmap = Rtt_NEW( allocator,
							BufferBitmap( display.GetAllocator(),
											w,
											h,
											ConvertFormat( format ) ) );
	}

	TextureResourceBitmap *result =
		Rtt_NEW( display.GetAllocator(), TextureResourceBitmap( factory, texture, bitmap ) );

	return result;
}
	
	
TextureResourceBitmap *
TextureResourceBitmap::CreateDefault(
	TextureFactory& factory,
	Texture::Format format,
	Texture::Filter filter)
{
	Display& display = factory.GetDisplay();
	TextureBitmap *texture = Rtt_NEW( display.GetAllocator(),
		TextureBitmap( display.GetAllocator(), 1, 1, format, filter) );
		
	U8 *data = texture->WriteAccess();
	for ( size_t i = 0, iMax = texture->GetSizeInBytes(); i < iMax; i++ )
	{
		*data++ = 0xFF;
	}

	TextureResourceBitmap *result =
		Rtt_NEW( display.GetAllocator(), TextureResourceBitmap( factory, texture, NULL ) );

	return result;
}
			
TextureResourceBitmap *
TextureResourceBitmap::Create(TextureFactory& factory, PlatformBitmap *bitmap )
{
	Display& display = factory.GetDisplay();
	Texture *texture = Rtt_NEW( display.GetAllocator(),
		PlatformBitmapTexture( display.GetAllocator(), * bitmap ) );
	TextureResourceBitmap *result =
		Rtt_NEW( display.GetAllocator(), TextureResourceBitmap( factory, texture, bitmap ) );

	return result;
}

TextureResourceBitmap::TextureResourceBitmap(
	TextureFactory &factory,
	Texture *texture )
	: TextureResource(factory, texture, NULL, kTextureResourceBitmap)
{
	
}

TextureResourceBitmap::TextureResourceBitmap(
	TextureFactory &factory,
	Texture *texture,
	PlatformBitmap *bitmap )
	: TextureResource(factory, texture, bitmap, kTextureResourceBitmap)
{

}

TextureResourceBitmap::~TextureResourceBitmap()
{
}
	
const MLuaUserdataAdapter&
TextureResourceBitmap::GetAdapter() const
{
	return TextureResourceBitmapAdapter::Constant();
}
	
} // namespace Rtt

