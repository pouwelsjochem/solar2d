//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Types.h"
#include "Rtt_BitmapUtils.h"
#include <png.h>
#include <cstring>		// for memcpy

#ifndef Rtt_LINUX_ENV
#include <SDL.h>
#endif

uint8_t* bitmapUtil::loadPNG(FILE* fp, int& w, int& h)
{
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png == NULL)
	{
		return NULL;
	}

	png_infop info = png_create_info_struct(png);
	if (info == NULL)
	{
		return NULL;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		return NULL;
	}

	png_init_io(png, fp);
	png_read_info(png, info);

	w = png_get_image_width(png, info);
	h = png_get_image_height(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth = png_get_bit_depth(png, info);

	if (bit_depth == 16)
		png_set_strip_16(png);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * h);
	for (int y = 0; y < h; y++)
	{
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png, info));
	}
	png_read_image(png, row_pointers);

	int pitch = png_get_rowbytes(png, info);
	uint8_t* im = (uint8_t*) malloc(pitch * h);
	uint8_t* dst = im;
  for(int y = 0; y < h; y++) 
	{
    png_bytep row = row_pointers[y];
		memcpy(dst, row, pitch);
		dst += pitch;
    free(row_pointers[y]);
  }
  free(row_pointers);
	png_destroy_read_struct(&png, &info, NULL);

	return im;
}

void pngWriteFunc(png_structp png_ptr, png_bytep data, png_size_t length)
{
	#ifndef Rtt_LINUX_ENV
		fwrite(data, length, 1, (FILE*)png_ptr->io_ptr);
	#endif
}

bool	bitmapUtil::savePNG(const char* filename, uint8_t* data, int width, int height, Rtt::PlatformBitmap::Format format)
// Writes a 24 or 32-bit color image in .png format, to the
// given output stream.  Data should be in [RGB or RGBA...] byte order.
{
	int bpp = Rtt::PlatformBitmap::BytesPerPixel(format);
	if (bpp != 3 && bpp != 4)
	{
//		printf("png writer: bpp must be 3 or 4\n");
		return false;
	}

	FILE* out = fopen(filename, "wb");
	if (out == NULL)
	{
//		printf("png writer: can't create %s\n", filename);
		return false;
	}

	png_structp	png_ptr;
	png_infop	info_ptr;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		// @@ log error here!
		fclose(out);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		// @@ log error here!
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(out);
		return false;
	}

	png_init_io(png_ptr, out);
	png_set_write_fn(png_ptr, (png_voidp)out, pngWriteFunc, NULL);
	png_set_IHDR(png_ptr,	info_ptr,	width, height, 8, bpp == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	if (format == Rtt::PlatformBitmap::Format::kBGRA)
	{
		png_set_swap_alpha(png_ptr);
	}
	//	 png_set_bgr(png_ptr);

	png_write_info(png_ptr, info_ptr);
	for (int y = 0; y < height; y++)
	{
		png_write_row(png_ptr, data + (width * bpp) * y);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(out);
	return true;
}

// get pixel value from SDL surface
#ifndef Rtt_LINUX_ENV
Uint32 getSurfacePixel(SDL_Surface *surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;

	// Here p is the address to the pixel we want to retrieve
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	switch (bpp)
	{
	case 1:
		return *p;
	case 2:
		return *(Uint16 *)p;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
	case 4:
		return (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3] : p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
	default:
		return 0;       // shouldn't happen, but avoids warnings
	}
}
#endif