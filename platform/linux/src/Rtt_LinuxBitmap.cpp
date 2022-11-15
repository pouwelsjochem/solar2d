//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"
#include "Rtt_GPUStream.h"
#include "Rtt_LinuxBitmap.h"
#include "Rtt_LinuxContainer.h"
#include "Display/Rtt_Display.h"
#include "Core/Rtt_Types.h"
#include "Rtt_BitmapUtils.h"

namespace Rtt
{
	// LinuxBaseBitmap
	LinuxBaseBitmap::LinuxBaseBitmap()
		: Super(), fData(NULL), fWidth(0), fHeight(0), fFormat(kUndefined), fProperties(0)
	{
	}

	LinuxBaseBitmap::LinuxBaseBitmap(Rtt_Allocator *context, int w, int h, uint8_t *rgba)
		: Super(), fData(NULL), fWidth(w), fHeight(h), fFormat(kRGBA), fProperties(0)
	{
		int size = fHeight * fWidth * 4;
		fData = (U8 *)Rtt_MALLOC(&context, size);
		memset(fData, 0, size);

		U8 *dst = fData;

		if (rgba)
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				// premultiple alpha
				dst[0] = (rgba[0] * rgba[3]) >> 8;
				dst[1] = (rgba[1] * rgba[3]) >> 8;
				dst[2] = (rgba[2] * rgba[3]) >> 8;
				dst[3] = rgba[3];

				dst += 4;
				rgba += 4;
			}
		}

		fFormat = kRGBA;
	}

	LinuxBaseBitmap::~LinuxBaseBitmap()
	{
		free(fData);
	}

	const void *LinuxBaseBitmap::GetBits(Rtt_Allocator *context) const
	{
		return fData;
	}

	U32 LinuxBaseBitmap::Width() const
	{
		return fWidth;
	}

	U32 LinuxBaseBitmap::Height() const
	{
		return fHeight;
	}

	PlatformBitmap::Format LinuxBaseBitmap::GetFormat() const
	{
		return fFormat;
	}

	bool LinuxBaseBitmap::IsProperty(PropertyMask mask) const
	{
		return IsPropertyInternal(mask);
	}

	void LinuxBaseBitmap::SetProperty(PropertyMask mask, bool newValue)
	{
		if (!Super::IsPropertyReadOnly(mask))
		{
			const U8 p = fProperties;
			const U8 propertyMask = (U8)mask;
			fProperties = (newValue ? p | propertyMask : p & ~propertyMask);
		}

		switch (mask)
		{
			case kIsBitsFullResolution:
				break;
			default:
				break;
		}
	}

	bool LinuxBaseBitmap::LoadFileBitmap(Rtt_Allocator &context, const char *path)
	{
		Rtt_ASSERT(fData == NULL);

		FILE* f = fopen(path, "rb");
		if (f)
		{
			fData = bitmapUtil::loadPNG(f, fWidth, fHeight);
			if (fData)
			{
				fFormat = kRGBA;
			}
			fclose(f);
		}

		if (fData && fFormat == kRGBA)
		{
			U8* dst = fData;
			for (int y = 0; y < fHeight; y++)
			{
				for (int x = 0; x < fWidth; x++)
				{
					// premultiple alpha
					if (dst[3] < 255)
					{
						dst[0] = (dst[0] * dst[3]) >> 8;
						dst[1] = (dst[1] * dst[3]) >> 8;
						dst[2] = (dst[2] * dst[3]) >> 8;
					}
					dst += 4;
				}
			}
		}

		return fData != NULL;
	}

	bool LinuxBaseBitmap::SaveBitmap(Rtt_Allocator *context, PlatformBitmap *bitmap, const char *filePath)
	{
		// Validate.
		if ((NULL == bitmap) || (NULL == filePath))
			return false;

		S32 w = bitmap->Width();
		S32 h = bitmap->Height();
		if (w <= 0 || h <= 0)
			return false;

		// Fetch the given bitmap's bits.
		U8* bits = (U8*)(bitmap->GetBits(context));
		if (bits == NULL)
			return false;

		Format fmt = bitmap->GetFormat();
		bool rc = false;

		std::string path = filePath;
		rc = bitmapUtil::savePNG(filePath, bits, w, h, fmt);
		return rc;
	}

	// FileBitmap
	LinuxFileBitmap::LinuxFileBitmap(Rtt_Allocator &context, const char *path)
		: Super(), fPath(&context, path)
	{
		LoadFileBitmap(context, path);
	}

	LinuxFileBitmap::~LinuxFileBitmap()
	{
	}

	// MaskFileBitmap
	LinuxMaskFileBitmap::LinuxMaskFileBitmap(Rtt_Allocator &context, const char *filePath)
		: Super(), fPath(&context, filePath)
	{
		if (LoadFileBitmap(context, filePath))
		{
			Rtt_ASSERT(fFormat == kRGBA || fFormat == kRGB);

			// convert to grayscale
			int size = fHeight * fWidth;
			U8 *newData = (U8 *)Rtt_MALLOC(&context, size);
			U8 *src = fData;
			U8 *dst = newData;

			for (int y = 0; y < fHeight; y++)
			{
				for (int x = 0; x < fWidth; x++)
				{
					*dst++ = (U8)(0.30f * src[0] + 0.59f * src[1] + 0.11f * src[2]);
					src += (fFormat == kRGBA) ? 4 : 3;
				}
			}

			free(fData);
			fData = newData;
			fFormat = kMask;
		}
	}

	LinuxMaskFileBitmap::~LinuxMaskFileBitmap()
	{
	}
}; // namespace Rtt
