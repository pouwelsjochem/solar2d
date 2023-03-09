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
#include "Rtt_Math.h"
#include <png.h>
#include <cstring>		// for memcpy
#include <memory>
#include <SDL.h>

namespace bitmapUtil
{
	uint8_t* loadPNG(FILE* fp, int& w, int& h)
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

		png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * h);
		for (int y = 0; y < h; y++)
		{
			row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
		}
		png_read_image(png, row_pointers);

		int pitch = png_get_rowbytes(png, info);
		uint8_t* im = (uint8_t*)malloc(pitch * h);
		uint8_t* dst = im;
		for (int y = 0; y < h; y++)
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

	struct png_buffer_t
	{
		char* buffer;
		size_t size;
	};

	void pngWriteFunc(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		// with libpng15 next line causes pointer deference error; use libpng12 
		struct png_buffer_t* p = (struct png_buffer_t*)png_get_io_ptr(png_ptr); // was png_ptr->io_ptr

		size_t nsize = p->size + length;
		p->buffer = (char*)(p->buffer ? realloc(p->buffer, nsize) : malloc(nsize));
		if (p->buffer)
		{
			memcpy(p->buffer + p->size, data, length);
			p->size += length;
			return;
		}
		Rtt_LogException("png writer: no memory\n");
	}

	char* savePNG(size_t &length, uint8_t* data, int width, int height, Rtt::PlatformBitmap::Format format)
		// Writes a 24 or 32-bit color image in .png format, to the
		// given output stream.  Data should be in [RGB or RGBA...] byte order.
	{
		int bpp = Rtt::PlatformBitmap::BytesPerPixel(format);
		if (bpp != 3 && bpp != 4)
		{
			Rtt_LogException("png writer: bpp must be 3 or 4\n");
			length = 0;
			return nullptr;
		}

		png_structp	png_ptr;
		png_infop	info_ptr;

		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == NULL)
		{
			Rtt_LogException("png writer: png_create_write_struct failed\n");
			length = 0;
			return nullptr;
		}

		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL)
		{
			png_destroy_write_struct(&png_ptr, NULL);
			Rtt_LogException("png writer: png_create_info_struct failed\n");
			length = 0;
			return nullptr;
		}

		png_buffer_t png_buffer = {};
		png_set_write_fn(png_ptr, &png_buffer, pngWriteFunc, NULL);

		png_set_IHDR(png_ptr, info_ptr, width, height, 8, bpp == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);

		bool free_data = false;
		if (format == Rtt::PlatformBitmap::Format::kBGRA)
		{
			// BGRA ==> RGBA
			U8* rgba = (U8*)malloc(width * height * 4);
			U8* src = data;
			U8* dst = rgba;
			for (int i = 0; i < width * height; i++)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst[3] = src[3];
				dst += 4;
				src += 4;
			}
			data = rgba;
			free_data = true;
		}

		for (int y = 0; y < height; y++)
		{
			png_write_row(png_ptr, data + (width * bpp) * y);
		}

		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);

		if (free_data)
		{
			free(data);
		}

		length = png_buffer.size;
		return png_buffer.buffer;
	}
}

