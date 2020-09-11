//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_PlatformBitmap_H__
#define _Rtt_PlatformBitmap_H__

#include "Core/Rtt_Macros.h"
#include "Core/Rtt_Real.h"
#include "Core/Rtt_Types.h"

#include "Renderer/Rtt_RenderTypes.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

class PlatformBitmap
{
	public:
		typedef enum Format
		{
			kUndefined = 0,
			kMask,
			kRGB,
			kRGBA, // Channels are (left to right) from MSB to LSB, so A is in the least-significant 8 bits
			kBGRA, // Channels are (left to right) from MSB to LSB, so A is in the least-significant 8 bits
			kABGR, // Channels are (left to right) from MSB to LSB, so A is in the most-significant 8 bits
			kARGB,

			kNumFormats
		}
		Format;

	public:
		// Note: the default for these properties is *always* false,
		// since the default implementation of IsProperty() is to return false.
		typedef enum _PropertyMask
		{
			kIsPremultiplied = 0x1,
			kIsBitsFullResolution = 0x2,
		}
		PropertyMask;

		static Rtt_INLINE bool IsPropertyReadOnly( PropertyMask mask ) { return kIsPremultiplied == mask; }

	public:
		PlatformBitmap();
		virtual ~PlatformBitmap() = 0;

	public:
		// GetBits() returns an uncompressed buffer of the image pixel data.
		// There are two things to note about this pixel data:
		// 
		// (1) Resolution
		// By default, the pixel data is auto-scaled to screen resolution; this
		// is useful for preview purposes. When kIsBitsFullResolution is set, the
		// pixel data is the original resolution of the image; this is useful
		// for saving images to disk. 
		virtual const void* GetBits( Rtt_Allocator* context ) const = 0;
		virtual void FreeBits() const = 0;

		// Returns true if the value (0-100%) of the pixel at row,col (i,j) is greater than threshold
		bool HitTest( Rtt_Allocator *context, int i, int j, U8 threshold = 0 ) const;

		// Returns width of buffer returned by Bits()
		virtual U32 Width() const = 0;

		// Returns height of buffer returned by Bits()
		virtual U32 Height() const = 0;
		virtual Format GetFormat() const = 0;

		// Indicates if the image was downscaled compared to the original image file/source.
		virtual bool WasScaled() const;

		// Gets the scaling factor applied to this image compared to the original image file/source.
		// For example, if the image was downsampled when loaded from file, then this scaler would be less than one.
		virtual Real GetScale() const;

		virtual bool IsProperty( PropertyMask mask ) const;
		virtual void SetProperty( PropertyMask mask, bool newValue );

		bool IsPremultiplied() const { return IsProperty( kIsPremultiplied ); }
		bool HasAlphaChannel() const;

		virtual U8 GetByteAlignment() const;

#ifdef Rtt_ANDROID_ENV
		void SwapRGB();
		static void SwapBitmapRGB( char * base, int w, int h );
#endif

	public:
		size_t NumBytes() const;
		size_t NumTextureBytes( bool roundToNextPow2 ) const;

		// TODO: Remove as this is subsumed by GetFormat();
		Rtt_INLINE bool IsMask() const { return GetFormat() == kMask; }

	public:
		RenderTypes::TextureFilter GetMagFilter() const { return (RenderTypes::TextureFilter)fMagFilter; }
		void SetMagFilter( RenderTypes::TextureFilter newValue ) { fMagFilter = newValue; }

		RenderTypes::TextureFilter GetMinFilter() const { return (RenderTypes::TextureFilter)fMinFilter; }
		void SetMinFilter( RenderTypes::TextureFilter newValue ) { fMinFilter = newValue; }

	public:
		RenderTypes::TextureWrap GetWrapX() const { return (RenderTypes::TextureWrap)fWrapX; }
		void SetWrapX( RenderTypes::TextureWrap newValue ) { fWrapX = newValue; }

		RenderTypes::TextureWrap GetWrapY() const { return (RenderTypes::TextureWrap)fWrapY; }
		void SetWrapY( RenderTypes::TextureWrap newValue ) { fWrapY = newValue; }

		// Allows you to remap textures.
		// Primarily used by gradients
		Real GetNormalizationScaleX() const { return fScaleX; }
		Real GetNormalizationScaleY() const { return fScaleY; }
		void SetNormalizationScaleX( Real newValue ) { fScaleX = newValue; }
		void SetNormalizationScaleY( Real newValue ) { fScaleY = newValue; }

	public:
		static size_t BytesPerPixel( Format format );
		static bool GetColorByteIndexesFor(Format format, int *alphaIndex, int *redIndex, int *greenIndex, int *blueIndex);

	protected:
		friend class PlatformBitAccess;
		virtual void Lock();
		virtual void Unlock();

	private:
		U8 fMagFilter;
		U8 fMinFilter;
		U8 fWrapX;
		U8 fWrapY;
		Real fScaleX;
		Real fScaleY;

/*
	public:
		Rtt_FORCE_INLINE Matrix& Transform() { return fDstToPlatformBitmap; }
		Rtt_FORCE_INLINE Matrix& PlatformBitmapTransform() { return fPlatformBitmapTransform; }
		
	private:
		Matrix fDstToPlatformBitmap;
		Matrix fPlatformBitmapTransform;
*/
};

class PlatformBitAccess
{
	public:
		PlatformBitAccess( PlatformBitmap& bitmap )
		:	fBitmap( bitmap )
		{
			fBitmap.Lock();
		}

		~PlatformBitAccess()
		{
			fBitmap.Unlock();
		}

	private:
		PlatformBitmap& fBitmap;
};


// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_PlatformBitmap_H__
