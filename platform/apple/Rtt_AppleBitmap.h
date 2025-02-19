//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_AppleBitmap_H__
#define _Rtt_AppleBitmap_H__

#include "Display/Rtt_PlatformBitmap.h"

// ----------------------------------------------------------------------------

struct CGImage;
@class NSString;
#ifdef Rtt_IPHONE_ENV
	@class UIImage;
#endif

namespace Rtt
{

// ----------------------------------------------------------------------------

#define Rtt_TEST_BITMAP

class AppleBitmap : public PlatformBitmap
{
	public:
		typedef PlatformBitmap Super;
		typedef AppleBitmap Self;

#ifdef Rtt_TEST_BITMAP
	public:
		static void Test( class Runtime& runtime );
#endif

	protected:
		AppleBitmap();
		virtual ~AppleBitmap();

	public:
		virtual void FreeBits() const;

	protected:
		mutable void* fData;
};

class AppleFileBitmap : public AppleBitmap
{
	public:
		typedef AppleBitmap Super;

	public:
		AppleFileBitmap( const char* inPath, bool isMask = false );
		virtual ~AppleFileBitmap();

	protected:
		#if defined( Rtt_IPHONE_ENV ) || defined( Rtt_TVOS_ENV )
			AppleFileBitmap( UIImage *image, bool isMask );
		#endif
		#ifdef Rtt_MAC_ENV
			AppleFileBitmap( NSImage *image, bool isMask );
		#endif

	
	protected:
		void Initialize();
		float CalculateScale() const;
	
	protected:
		Rtt_INLINE bool IsPropertyInternal( PropertyMask mask ) const { return (fProperties & mask) ? true : false; }

#ifdef Rtt_DEBUG
		void PrintChannel( const U8 *bytes, int channel, U32 bytesPerPixel ) const;
#endif
		void* GetBitsGrayscale( Rtt_Allocator* context ) const;
		void* GetBitsColor( Rtt_Allocator* context ) const;

	public:
		virtual const void* GetBits( Rtt_Allocator* context ) const;
		virtual U32 Width() const;
		virtual U32 Height() const;
		virtual PlatformBitmap::Format GetFormat() const;
		virtual bool WasScaled() const;
		virtual Real GetScale() const;
		virtual bool IsProperty( PropertyMask mask ) const;
		virtual void SetProperty( PropertyMask mask, bool newValue );

	private:
		struct CGImage* fImage;
		float fScale;
		U8 fProperties;
		U8 fIsMask;
};

#if defined( Rtt_IPHONE_ENV ) || defined( Rtt_TVOS_ENV )
// * Some API's only give access to UIImage, not CGImage.
class IPhoneFileBitmap : public AppleFileBitmap
{
	public:
		typedef AppleFileBitmap Super;

	public:
		IPhoneFileBitmap( UIImage *image, bool isMask = false );
		virtual ~IPhoneFileBitmap();

	private:
		UIImage *fUIImage;
};
#endif

#ifdef Rtt_MAC_ENV
class MacFileBitmap : public AppleFileBitmap
{
	public:
		typedef AppleFileBitmap Super;
		
	public:
		MacFileBitmap( NSImage *image, bool isMask = false );
		
	private:
//		NSImage *fNSImage;
	};
#endif


// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_AppleBitmap_H__
