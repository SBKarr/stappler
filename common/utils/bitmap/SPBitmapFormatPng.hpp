/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#ifndef COMMON_UTILS_BITMAP_SPBITMAPFORMATPNG_HPP_
#define COMMON_UTILS_BITMAP_SPBITMAPFORMATPNG_HPP_

#include "SPBitmap.h"

#include "png.h"

NS_SP_EXT_BEGIN(png)

using Color = Bitmap::PixelFormat;
using Alpha = Bitmap::Alpha;

struct ReadState {
	const uint8_t *data = nullptr;
	size_t offset = 0;
};

void readDynamicData(png_structp pngPtr, png_bytep data, png_size_t length) {
	auto state = (ReadState *)png_get_io_ptr(pngPtr);
	memcpy(data, state->data + state->offset, length);
	state->offset += length;
}

static bool loadPng(const uint8_t *inputData, size_t size,
		Bytes &outputData, Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
		uint32_t &stride, const Bitmap::StrideFn &strideFn) {
	auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
    	log::text("libpng", "fail to create read struct");
        return false;
    }

    auto info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
    	log::text("libpng", "fail to create info struct");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return false;
    }

	if (setjmp(png_jmpbuf(png_ptr))) {
		log::text("libpng", "error in processing (setjmp return)");
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    return false;
	}

	ReadState state;
	state.data = inputData;
	state.offset = 0;

#ifdef PNG_ARM_NEON_API_SUPPORTED
#if PNG_ARM_NEON_OPT == 1
	if (platform::proc::_isArmNeonSupported()) {
		png_set_option(png_ptr, PNG_ARM_NEON, PNG_OPTION_ON);
	}
#endif
#endif
	png_set_read_fn(png_ptr,(png_voidp)&state, readDynamicData);
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    png_byte bitdepth = png_get_bit_depth(png_ptr, info_ptr);
    png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bitdepth < 8) {
    	bitdepth = 8;
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }
    if (bitdepth == 16) {
        png_set_strip_16(png_ptr);
    }
    if (bitdepth < 8) {
        png_set_packing(png_ptr);
    }

    png_read_update_info(png_ptr, info_ptr);
    bitdepth = png_get_bit_depth(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
	auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY) {
		color = (color == Color::A8?Color::A8:Color::I8);
	} else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		color = Color::IA88;
	} else if (color_type == PNG_COLOR_TYPE_RGB) {
		color = Color::RGB888;
	} else if (color_type == PNG_COLOR_TYPE_RGBA) {
		color = Color::RGBA8888;
	} else {
		width = 0;
		height = 0;
		stride = 0;
		outputData.clear();
        log::format("Bitmap", "unsupported color type: %u", (unsigned int)color_type);
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    return false;
	}

	if (strideFn) {
		stride = max((uint32_t)strideFn(color, width), (uint32_t)rowbytes);
	} else {
		stride = (uint32_t)rowbytes;
	}

	if (color == Color::I8 || color == Color::RGB888) {
		alpha = Alpha::Opaque;
	} else {
		alpha = Alpha::Unpremultiplied;
	}

    // read png data
    png_bytep* row_pointers = new png_bytep[height];

    auto dataLen = stride * height;
    outputData.resize(dataLen);

    for (unsigned short i = 0; i < height; ++i) {
        row_pointers[i] = outputData.data() + i*stride;
    }

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, nullptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    delete [] row_pointers;

	if (!outputData.empty()) {
		return true;
	}

	return false;
}

struct PngStruct {
	int bit_depth = 8;
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	bool valid = false;

	FILE *fp = nullptr;
	Bytes *vec = nullptr;

	~PngStruct() {
		if (png_ptr != nullptr) {
	        png_destroy_write_struct(&png_ptr, &info_ptr);
		}

		if (fp) {
	        fclose(fp);
		}
	}

	PngStruct() {
	    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	    if (png_ptr == nullptr) {
	        log::text("libpng", "fail to create write struct");
	    	return;
	    }

	    info_ptr = png_create_info_struct (png_ptr);
	    if (info_ptr == nullptr) {
	        log::text("libpng", "fail to create info struct");
	        return;
	    }
	}

	PngStruct(Bytes *v) : PngStruct() {
		vec = v;
	    valid = true;
	}

	PngStruct(const String &filename) : PngStruct() {
	    fp = filesystem_native::fopen_fn(filename, "wb");
	    if (!fp) {
	        log::format("Bitmap", "fail to open file '%s' to write png data", filename.c_str());
		    valid = false;
			return;
	    }

	    valid = true;
	}

	static void writePngFn(png_structp png_ptr, png_bytep data, png_size_t length) {
		auto vec = (Bytes *)png_get_io_ptr(png_ptr);
		size_t offset = vec->size();
		vec->resize(vec->size() + length, 0);
		memcpy(vec->data() + offset, data, length);
	}

	bool write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Bitmap::PixelFormat format, bool invert = false) {
		if (!valid) {
			return false;
		}

		if (!fp && !vec) {
			return false;
		}

	    if (setjmp (png_jmpbuf (png_ptr))) {
	        log::text("libpng", "error in processing (setjmp return)");
	        return false;
	    }

	    if (stride == 0) {
	    	stride = Bitmap::getBytesPerPixel(format) * width;
	    }

	    int color_type = 0;
		switch (format) {
		case Bitmap::PixelFormat::A8:
		case Bitmap::PixelFormat::I8:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		case Bitmap::PixelFormat::IA88:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case Bitmap::PixelFormat::RGB888:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		case Bitmap::PixelFormat::RGBA8888:
			color_type = PNG_COLOR_TYPE_RGBA;
			break;
		default:
			return false;
		}

	    /* Set image attributes. */
	    png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth,
	    		color_type, PNG_INTERLACE_NONE,
	                  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	    /* Initialize rows of PNG. */
	    png_byte **row_pointers = (png_byte **)png_malloc(png_ptr, height * sizeof (png_byte *));
	    for (size_t i = 0; i < height; ++i) {
	        row_pointers[i] = (png_byte *) (
	        		(invert)?(&data[stride * (height - i - 1)]):(&data[stride * i]));
	    }

	    if (fp) {
	        png_init_io (png_ptr, fp);
	    } else {
	        png_set_write_fn(png_ptr, vec, &writePngFn, nullptr);
	    }

	    png_set_rows (png_ptr, info_ptr, row_pointers);
	    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	    png_free(png_ptr, row_pointers);
	    return true;
	}
};

static bool savePng(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	PngStruct s(filename);
	return s.write(data, width, height, stride, format, invert);
}

static Bytes writePng(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
    Bytes state;
    PngStruct s(&state);
	if (s.write(data, width, height, stride, format, invert)) {
		return state;
	} else {
		return Bytes();
	}
}

NS_SP_EXT_END(png)

#endif /* COMMON_UTILS_BITMAP_SPBITMAPFORMATPNG_HPP_ */
