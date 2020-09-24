//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core\Rtt_Build.h"
#include "Core\Rtt_String.h"
#include "Display\Rtt_PlatformBitmap.h"


#pragma region Forward Declarations
namespace Gdiplus
{
	class Bitmap;
	class BitmapData;
}
namespace Interop
{
	class RuntimeEnvironment;
}

#pragma endregion


namespace Rtt
{

class WinBitmap : public PlatformBitmap
{
	public:
		typedef PlatformBitmap Super;
		typedef WinBitmap Self;
		static const int kBytePackingAlignment = 4;

	protected:
		WinBitmap();
		virtual ~WinBitmap();

	public:
		virtual const void* GetBits( Rtt_Allocator* context ) const;
		virtual void FreeBits() const;
		virtual U32 Width() const;
		virtual U32 Height() const;
		virtual PlatformBitmap::Format GetFormat() const;

	protected:
		mutable void * fData;

	protected:
		Gdiplus::Bitmap *	fBitmap;
		Gdiplus::BitmapData * fLockedBitmapData;

		virtual void Lock();
		virtual void Unlock();
};

class WinFileBitmap : public WinBitmap
{

	public:
		typedef WinBitmap Super;

	protected:
		WinFileBitmap( Rtt_Allocator &context );

	public:
		WinFileBitmap( const char *inPath, Rtt_Allocator &context );
		virtual ~WinFileBitmap();

	private:
		void InitializeMembers();

	protected:
		float CalculateScale() const;

	protected:
		Rtt_INLINE bool IsPropertyInternal( PropertyMask mask ) const { return (fProperties & mask) ? true : false; }

	public:
		virtual U32 Width() const;
		virtual U32 Height() const;
		virtual bool IsProperty( PropertyMask mask ) const;
		virtual void SetProperty( PropertyMask mask, bool newValue );

	private:
		float fScale;
		U8 fProperties;
		S16 fAngle; // [0, +-90, +-180]

	protected:
#ifdef Rtt_DEBUG
		String fPath;
#endif
};

class WinFileGrayscaleBitmap : public WinFileBitmap
{
	public:
		typedef WinFileBitmap Super;

		WinFileGrayscaleBitmap( const char *inPath, Rtt_Allocator &context );
		virtual ~WinFileGrayscaleBitmap();

		virtual void FreeBits() const;
		virtual PlatformBitmap::Format GetFormat() const;
		virtual U32 Width() const;
		virtual U32 Height() const;

	protected:
		virtual void Lock();
		virtual void Unlock();

		U32 fWidth;
		U32 fHeight;
};

} // namespace Rtt
