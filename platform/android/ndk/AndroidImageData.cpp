//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////


#include "AndroidImageData.h"
#include "Core/Rtt_Build.h"
#include "Core/Rtt_Types.h"
#include "Display/Rtt_PlatformBitmap.h"


// ----------------------------------------------------------------------------
// Constructors/Destructors
// ----------------------------------------------------------------------------

/// Creates a new image data object.
/// @param allocatorPointer Allocator needed to create this object and its byte buffer. Cannot be NULL.
AndroidImageData::AndroidImageData(Rtt_Allocator *allocatorPointer)
:	fAllocatorPointer(allocatorPointer),
	fImageByteBufferPointer(NULL),
	fWidth(0),
	fHeight(0),
	fScale(Rtt_REAL_1),
	fIsPixelFormatGrayscale(false)
{
}

/// Destroys this image data object and its byte buffer.
AndroidImageData::~AndroidImageData()
{
	DestroyImageByteBuffer();
}


// ----------------------------------------------------------------------------
// Public Member Functions
// ----------------------------------------------------------------------------

/// Gets the allocator this object uses to create its image byte buffer.
/// @return Returns a pointer to the allocator.
///         <br>
///         Returns NULL if this object was never assigend an allocator.
Rtt_Allocator* AndroidImageData::GetAllocator() const
{
	return fAllocatorPointer;
}

/// Gets the pixel width of the image.
/// @return Returns the image's pixel width.
///         <br>
///         Returns zero if the width has never been assigned via the SetWidth() function.
U32 AndroidImageData::GetWidth() const
{
	return fWidth;
}

/// Sets the pixel width of the image.
/// @param value The image's pixel width.
void AndroidImageData::SetWidth(U32 value)
{
	DestroyImageByteBuffer();
	fWidth = value;
}

/// Gets the pixel height of the image.
/// @return Returns the image's pixel height.
///         <br>
///         Returns zero if the height has never been assigned via the SetHeight() function.
U32 AndroidImageData::GetHeight() const
{
	return fHeight;
}

/// Sets the pixel height of the image.
/// @param value The image's pixel height.
void AndroidImageData::SetHeight(U32 value)
{
	DestroyImageByteBuffer();
	fHeight = value;
}

/// Gets the scale of this image compared to the original image file that was loaded.
/// <br>
/// scale = loadedImageWidth / originalImageWidth
/// <br>
/// This is needed in case the image had to be downscaled/downsampled, in which case,
/// the rendering system will need to upscale the image's rectangle back to the original
/// image size.
/// <br>
/// Note that this ImageData object's width and height represents the load image's dimensions
/// after it was downscaled/downsampled if applicable. This scale settings is only informational.
/// @return Returns 1.0 if this image object's byte buffer was not scaled and matches the
///         dimensions of the source image that was loaded.
///         <br>
///         Returns a value less than zero if the loaded image was downscaled or downsampled
///         compared to the original image. This is commonly done if the original image is
///         larger than the max texture size in OpenGL.
Rtt_Real AndroidImageData::GetScale() const
{
	return fScale;
}

/// Sets the scale of the loaded image compared to the original file.
/// <br>
/// scale = loadedImageWidth / originalImageWidth
/// <br>
/// This is expected to be set if the image loader had to downscale/downsample the image
/// so that the rendering system can increase the size of the image's rectangle back to
/// the original image file's size.
/// <br>
/// Note that this ImageData object's width and height is expected to match the loaded
/// image's dimensions, after it was downscaled if applicable. This scale setting is
/// only informational.
/// @param value Set to 1.0 if the loaded image was not downscaled.
///              <br>
///              Set to (loadedImageWidth / originalImageWidth) if the loaded image
///              was downscaled or downsampled.
void AndroidImageData::SetScale(Rtt_Real value)
{
	fScale = value;
}

/// Sets the image pixel format to 32-bit RGBA.
void AndroidImageData::SetPixelFormatToRGBA()
{
	DestroyImageByteBuffer();
	fIsPixelFormatGrayscale = false;
}

/// Sets the image pixel format to 8-bit alpha.
void AndroidImageData::SetPixelFormatToGrayscale()
{
	DestroyImageByteBuffer();
	fIsPixelFormatGrayscale = true;
}

/// Determines if this image's byte buffer contains 32-bit RGBA pixels.
/// @return Returns true if the GetImageByteBuffer() function returns a byte array containing
///         32-bit RGBA pixels. This means that the byte buffer size is (4 * width * height).
///         <br>
///         Returns false if the byte array contains 8-bit pixels. This means that the byte buffer
///         size is (width * height).
bool AndroidImageData::IsPixelFormatRGBA() const
{
	return !fIsPixelFormatGrayscale;
}

/// Determines if this image's byte buffer contains 8-bit alpha pixels.
/// @return Returns true if the GetImageByteBuffer() function returns a byte array containing
///         8-bit alpha pixels. This means that the byte buffer size is (width * height).
///         <br>
///         Returns false if the byte array contains 32-bit RGBA pixels. This means that the byte buffer
///         size is (4 * width * height).
bool AndroidImageData::IsPixelFormatGrayscale() const
{
	return fIsPixelFormatGrayscale;
}

/// Gets the number of bytes that one pixel uses in this image.
/// @return Returns 1 if this image is using an 8-bit grayscale pixel format.
///         <br>
///         Returns 4 if this image is using a 32-bit RGBA pixel format.
U32 AndroidImageData::GetPixelSizeInBytes() const
{
	return fIsPixelFormatGrayscale ? 1 : 4;
}

/// Creates a byte buffer to be used to copy an image's pixels to.
/// This byte buffer can then be rerieved via the GetImageByteBuffer() function.
/// <br>
/// You are expected to call this object's SetWidth(), SetHeight(), and SetPixelFormat*() functions
/// first since they define the size of this byte buffer.
/// <br>
/// If an existing byte buffer has already been created, then calling this function again
/// will destroy the previous byte buffer and then create a new one.
/// @return Returns true if the byte buffer was successfully created.
///         <br>
///         Returns false if unable to create the byte buffer, which can happen if this object
///         has been assigned invalid informationm, such as a width and height set to zero.
///
bool AndroidImageData::CreateImageByteBuffer()
{
	// Destroy the last image buffer, if allocated.
	DestroyImageByteBuffer();

	// Do not continue if this image has invalid configuration.
	if (!fAllocatorPointer || (fWidth <= 0) || (fHeight <= 0) || (fScale <= Rtt_REAL_0))
	{
		return false;
	}

	// Create the byte buffer.
	U32 byteCount = (fWidth * fHeight) * GetPixelSizeInBytes();
	fImageByteBufferPointer = (U8*)Rtt_MALLOC(fAllocatorPointer, (size_t)byteCount);
	return fImageByteBufferPointer ? true : false;
}

/// Destroys the byte buffer that was created by the CreateImageByteBuffer() function.
/// <br>
/// If no byte buffer exists, then this function will do nothing.
/// <br>
/// Note that this object's destructor will automatically destroy its byte buffer.
void AndroidImageData::DestroyImageByteBuffer()
{
	if (fImageByteBufferPointer)
	{
		Rtt_FREE((void*)fImageByteBufferPointer);
		fImageByteBufferPointer = NULL;
	}
}

/// Gets a pointer to the byte buffer created by the CreateImageByteBuffer() function.
/// <br>
/// You are expected to copy an image's pixels to this byte buffer.
/// @return Returns a pointer to the byte buffer.
///         <br>
///         Returns NULL if a byte buffer has not been created by the CreateImageByteBuffer() function.
U8* AndroidImageData::GetImageByteBuffer() const
{
	return fImageByteBufferPointer;
}
