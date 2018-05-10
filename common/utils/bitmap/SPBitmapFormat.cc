// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPCommonPlatform.h"
#include <setjmp.h>

#include "SPBitmap.h"
#include "SPDataReader.h"
#include "SPFilesystem.h"
#include "SPLog.h"
#include "SPHtmlParser.h"
#include "jpeglib.h"
#include "png.h"

#include "webp/decode.h"
#include "webp/encode.h"

NS_SP_EXT_BEGIN(jpeg)

using Color = Bitmap::PixelFormat;
using Alpha = Bitmap::Alpha;

struct JpegError {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */

    static void ErrorExit(j_common_ptr cinfo) {
    	JpegError * myerr = (JpegError *) cinfo->err;
	    char buffer[JMSG_LENGTH_MAX];
	    (*cinfo->err->format_message) (cinfo, buffer);
	    log::format("JPEG", "jpeg error: %s", buffer);
	    longjmp(myerr->setjmp_buffer, 1);
	}
};

static bool loadJpg(const uint8_t *inputData, size_t size,
		Bytes &outputData, Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
		uint32_t &stride, const Bitmap::StrideFn &strideFn) {
	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo;
	struct JpegError jerr;

	JSAMPROW row_pointer[1] = {0};
	unsigned long location = 0;

	bool ret = false;
	do {
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = &JpegError::ErrorExit;
		if (setjmp(jerr.setjmp_buffer))	{
			jpeg_destroy_decompress(&cinfo);
			break;
		}

		jpeg_create_decompress( &cinfo );
		jpeg_mem_src(&cinfo, const_cast<unsigned char*>(inputData), size);

		/* reading the image header which contains image information */
		jpeg_read_header(&cinfo, TRUE);

		// we only support RGB or grayscale
		if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
			color = (color == Color::A8?Color::A8:Color::I8);
		} else {
			cinfo.out_color_space = JCS_RGB;
			color = Color::RGB888;
		}

		/* Start decompression jpeg here */
		jpeg_start_decompress( &cinfo );

		/* init image info */
		width  = cinfo.output_width;
		height = cinfo.output_height;

		if (color == Color::I8 || color == Color::RGB888) {
			alpha = Alpha::Opaque;
		} else {
			alpha = Alpha::Unpremultiplied;
		}

		if (strideFn) {
			stride = max(strideFn(color, width), cinfo.output_width*cinfo.output_components);
		} else {
			stride = cinfo.output_width*cinfo.output_components;
		}

		auto dataLen = height * stride;
		outputData.resize(dataLen);

		/* now actually read the jpeg into the raw buffer */
		/* read one scan line at a time */
		while (cinfo.output_scanline < cinfo.output_height) {
			row_pointer[0] = outputData.data() + location;
			location += stride;
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
		}

		jpeg_destroy_decompress( &cinfo );
		ret = true;
	} while (0);

	return ret;
}

struct JpegStruct {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	bool valid = false;

	FILE *fp = nullptr;
	Bytes *vec = nullptr;

	unsigned char *mem = nullptr;
	unsigned long memSize = 0;

	~JpegStruct() {
		jpeg_destroy_compress( &cinfo );

		if (fp) {
			fclose(fp);
		}
    	if (mem) {
    		free(mem);
    	}
	}

	JpegStruct() {
		cinfo.err = jpeg_std_error( &jerr );
		jpeg_create_compress(&cinfo);
	}

	JpegStruct(Bytes *v) : JpegStruct() {
		vec = v;
		jpeg_mem_dest(&cinfo, &mem, &memSize);
		valid = true;
	}

	JpegStruct(const String &filename) : JpegStruct() {
		fp = filesystem_native::fopen_fn(filename, "wb");
		if (!fp) {
			log::format("Bitmap", "fail to open file '%s' to write jpeg data", filename.c_str());
			valid = false;
			return;
		}

		jpeg_stdio_dest(&cinfo, fp);
		valid = true;
	}

	bool write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Bitmap::PixelFormat format, bool invert = false) {
		if (!valid) {
			return false;
		}

		/* this is a pointer to one row of image data */
		JSAMPROW row_pointer[1];

		/* Setting the parameters of the output file here */
		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = Bitmap::getBytesPerPixel(format);

	    switch (format) {
	    case Color::A8:
	    case Color::I8:
	    	cinfo.input_components = 1;
	    	cinfo.in_color_space = JCS_GRAYSCALE;
	    	break;
	    case Color::RGB888:
	    	cinfo.input_components = 3;
	    	cinfo.in_color_space = JCS_RGB;
	    	break;
	    default:
			log::format("JPEG", "Color format is not supported by JPEG!");
	    	return false;
	    	break;
	    }

	    /* default compression parameters, we shouldn't be worried about these */
	    jpeg_set_defaults( &cinfo );
	    jpeg_set_quality( &cinfo, 90, TRUE );
	    /* Now do the compression .. */
	    jpeg_start_compress( &cinfo, TRUE );
	    /* like reading a file, this time write one row at a time */
	    while( cinfo.next_scanline < cinfo.image_height ) {
	        row_pointer[0] = (JSAMPROW) &data[ (invert ? (height - 1 - cinfo.next_scanline) : cinfo.next_scanline) * stride ];
	        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	    }
	    /* similar to read file, clean up after we're done compressing */
	    jpeg_finish_compress( &cinfo );

	    if (vec) {
	    	if (memSize) {
		    	vec->assign(mem, mem + memSize);
	    	} else {
	    		return false;
	    	}
	    }

	    return true;
	}
};

static bool saveJpeg(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	JpegStruct s(filename);
	return s.write(data, width, height, stride, format, invert);
}

static Bytes writeJpeg(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
    Bytes state;
    JpegStruct s(&state);
	if (s.write(data, width, height, stride, format, invert)) {
		return state;
	} else {
		return Bytes();
	}
}

NS_SP_EXT_END(jpeg)


NS_SP_EXT_BEGIN(png)

using Color = Bitmap::PixelFormat;
using Alpha = Bitmap::Alpha;

struct ReadState {
	const uint8_t *data = nullptr;
	size_t offset = 0;
};

static void readDynamicData(png_structp pngPtr, png_bytep data, png_size_t length) {
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


NS_SP_EXT_BEGIN(webp)

static bool loadWebp(const uint8_t *inputData, size_t size,
		Bytes &outputData, Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
		uint32_t &stride, const Bitmap::StrideFn &strideFn) {

	WebPDecoderConfig config;
	if (WebPInitDecoderConfig(&config) == 0) return false;
	if (WebPGetFeatures(inputData, size, &config.input) != VP8_STATUS_OK) return false;
	if (config.input.width == 0 || config.input.height == 0) return false;

	config.output.colorspace = config.input.has_alpha ? MODE_RGBA : MODE_RGB;
	color = config.input.has_alpha ? BitmapFormat::Color::RGBA8888 : BitmapFormat::Color::RGB888;
	width = config.input.width;
	height = config.input.height;

	alpha = (config.input.has_alpha != 0) ? Bitmap::Alpha::Unpremultiplied : Bitmap::Alpha::Opaque;

	stride = strideFn ? strideFn(color, width) : width * Bitmap::getBytesPerPixel(color);
	outputData.resize(stride * height);

	config.output.u.RGBA.rgba = outputData.data();
	config.output.u.RGBA.stride = stride;
	config.output.u.RGBA.size = outputData.size();
	config.output.is_external_memory = 1;

	if (WebPDecode(inputData, size, &config) != VP8_STATUS_OK) {
		outputData.clear();
		return false;
	}

	return true;
}


struct WebpStruct {
	static bool isWebpSupported(Color format) {
		switch (format) {
		case Color::A8:
		case Color::I8:
		case Color::IA88:
		case Color::Auto:
			log::text("Bitmap", "Webp supports only RGB888 and RGBA8888");
			return false;
		default:
			break;
		}
		return true;
	}

	static int FileWriter(const uint8_t* data, size_t data_size, const WebPPicture* const pic) {
		FILE* const out = (FILE *)pic->custom_ptr;
		return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
	}

	FILE *fp = nullptr;
	Bytes *vec = nullptr;

	WebPConfig config;
	WebPPicture pic;
	WebPMemoryWriter writer;

	bool pictureInit = false;
	bool memoryInit = false;
	bool valid = true;

	~WebpStruct() {
		if (pictureInit) {
			WebPPictureFree(&pic);
			pictureInit = false;
		}

		if (memoryInit) {
			WebPMemoryWriterClear(&writer);
			memoryInit = false;
		}

		if (fp) {
	        fclose(fp);
		}
	}

	WebpStruct(Color format, bool lossless) {
		if (!isWebpSupported(format)) {
			valid = false;
		}

		if (!WebPPictureInit(&pic)) {
			valid = false;
		}


		if (lossless) {
			if (!WebPConfigPreset(&config, WEBP_PRESET_ICON, 100.0f)) {
				valid = false;
			}

			config.lossless = 1;
			config.method = 6;
		} else {
			if (!WebPConfigPreset(&config, WEBP_PRESET_PICTURE, 90.0f)) {
				valid = false;
			}

			config.lossless = 0;
			config.method = 6;
		}

		if (!WebPValidateConfig(&config)) {
			valid = false;
		}
	}

	WebpStruct(Bytes *v, Color format, bool lossless) : WebpStruct(format, lossless) {
		vec = v;
	}

	WebpStruct(const String &filename, Color format, bool lossless) : WebpStruct(format, lossless) {
	    fp = filesystem_native::fopen_fn(filename, "wb");
	    if (!fp) {
	        log::format("Bitmap", "fail to open file '%s' to write png data", filename.c_str());
		    valid = false;
			return;
	    }
	}

	operator bool () const {
		return valid;
	}

	bool write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Bitmap::PixelFormat format) {
		if (!valid) {
			return false;
		}

		if (!fp && !vec) {
			return false;
		}

		WebPPicture pic;

		pic.use_argb = 1;
		pic.width = width;
		pic.height = height;

		Bitmap tmp;

		switch (format) {
		case Color::A8:
		case Color::I8:
		case Color::IA88:
		case Color::Auto:
			return false;
		case Color::RGB888:
			WebPPictureImportRGB(&pic, data, stride);

			break;
		case Color::RGBA8888:
			WebPPictureImportRGBA(&pic, data, stride);
			break;
		}
		pictureInit = true;

		if (fp) {
			pic.writer = FileWriter;
			pic.custom_ptr = fp;
		} else {
			WebPMemoryWriterInit(&writer);
			pic.writer = WebPMemoryWrite;
			pic.custom_ptr = &writer;
			memoryInit = true;
		}

		if (!WebPEncode(&config, &pic)) {
			return false;
		}

		if (vec) {
			vec->resize(writer.size);
			memcpy(vec->data(), writer.mem, writer.size);
		}

	    return true;
	}
};


static bool saveWebpLossless(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (invert) {
		log::format("Bitmap", "Inverted output is not supported for webp");
		return false;
	}

	WebpStruct coder(filename, format, true);
	if (coder) {
		return coder.write(data, width, height, stride, format);
	}

	return false;
}

static Bytes writeWebpLossless(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (invert) {
		log::format("Bitmap", "Inverted output is not supported for webp");
		return Bytes();
	}

	Bytes state;
	WebpStruct coder(&state, format, true);
	if (coder.write(data, width, height, stride, format)) {
		return state;
	}
	return Bytes();
}

static bool saveWebpLossy(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (invert) {
		log::format("Bitmap", "Inverted output is not supported for webp");
		return false;
	}

	WebpStruct coder(filename, format, false);
	if (coder) {
		return coder.write(data, width, height, stride, format);
	}

	return false;
}

static Bytes writeWebpLossy(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (invert) {
		log::format("Bitmap", "Inverted output is not supported for webp");
		return Bytes();
	}

	Bytes state;
	WebpStruct coder(&state, format, false);
	if (coder.write(data, width, height, stride, format)) {
		return state;
	}
	return Bytes();
}

NS_SP_EXT_END(webp)


NS_SP_BEGIN

static bool BitmapFormat_isPng(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return memcmp(PNG_SIGNATURE, data, sizeof(PNG_SIGNATURE)) == 0;
}

static bool BitmapFormat_getPngImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isPng(data.data(), data.size()) && data.size() >= 24) {
		auto reader = DataReader<ByteOrder::Network>(data.data() + 16, 8);

		width = reader.readUnsigned32();
		height = reader.readUnsigned32();

		return true;
	}
	return false;
}

static bool BitmapFormat_isJpg(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const unsigned char JPG_SOI[] = {0xFF, 0xD8};
    return memcmp(data, JPG_SOI, 2) == 0;
}

static bool BitmapFormat_getJpegImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isJpg(data.data(), data.size())) {
		size_t offset = 2;
		uint16_t len = 0;
		uint8_t marker = 0;

		auto reader = DataReader<ByteOrder::Network>(data.data() + 2, data.size() - 2);
		while (reader.is((uint8_t) 0xFF)) {
			++reader;
		}

		marker = reader.readUnsigned();
		len = reader.readUnsigned16();

		while (marker < 0xC0 || marker > 0xCF) {
			offset += 2 + len;
			data.clear();

			if (file.seekAndRead(offset, data, 12) != 12) {
				return false;
			}

			if (data.size() >= 12) {
				reader = data.get<DataReader<ByteOrder::Network>>();

				while (reader.is((uint8_t) 0xFF)) {
					++reader;
				}

				marker = reader.readUnsigned();
				len = reader.readUnsigned16();
			} else {
				reader.clear();
				break;
			}
		}

		if (reader >= 5 && marker >= 0xC0 && marker <= 0xCF) {
			++reader;
			height = reader.readUnsigned16();
			width = reader.readUnsigned16();
			return true;
		}

		return false;
	}
	return false;
}

static bool BitmapFormat_isWebpLossless(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 12) {
        return false;
    }

    static const char* WEBP_RIFF = "RIFF";
    static const char* WEBP_WEBP = "WEBPVP8L";

    return memcmp(data, WEBP_RIFF, 4) == 0
    		&& memcmp(static_cast<const unsigned char*>(data) + 8, WEBP_WEBP, 8) == 0;
}

static bool BitmapFormat_getWebpLosslessImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isWebpLossless(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Big>(data.data() + 21, 4);

		auto b0 = reader.readUnsigned();
		auto b1 = reader.readUnsigned();
		auto b2 = reader.readUnsigned();
		auto b3 = reader.readUnsigned();

		// first 14 bits - width, last 14 bits - height

		width = (b0 | ((b1 & 0x3F) << 8)) + 1;
		height = (((b3 & 0xF) << 10) | (b2 << 2) | ((b1 & 0xC0) >> 6)) + 1;

		return true;
	}
	return false;
}

static bool BitmapFormat_isWebp(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 12) {
        return false;
    }

    static const char* WEBP_RIFF = "RIFF";
    static const char* WEBP_WEBP = "WEBP";

    return memcmp(data, WEBP_RIFF, 4) == 0
        && memcmp(static_cast<const unsigned char*>(data) + 8, WEBP_WEBP, 4) == 0;
}

static bool BitmapFormat_getWebpImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isWebp(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 24, 6);

		auto b0 = reader.readUnsigned();
		auto b1 = reader.readUnsigned();
		auto b2 = reader.readUnsigned();
		auto b3 = reader.readUnsigned();
		auto b4 = reader.readUnsigned();
		auto b5 = reader.readUnsigned();

		// first 14 bits - width, last 14 bits - height

		width = (b0 | (b1 << 8) | (b2 << 8)) + 1;
		height = (b3 | (b4 << 8) | (b5 << 8)) + 1;

		return true;
	}
	return false;
}

size_t BitmapFormat_detectSvgSize(const StringView &value) {
	StringView str(value);
	auto fRes = str.readFloat();
	if (!fRes.valid()) {
		return 0;
	}

	auto fvalue = fRes.get();
	if (fvalue == 0.0f) {
		return 0;
	}

	str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (str == "px" || str.empty()) {
		// do nothing
	} else if (str == "pt") {
		fvalue = fvalue * 4.0f / 3.0f;
	} else if (str == "pc") {
		fvalue = fvalue * 15.0f;
	} else if (str == "mm") {
		fvalue = fvalue * 3.543307f;
	} else if (str == "cm") {
		fvalue = fvalue * 35.43307f;
	} else {
		log::format("Bitmap", "Invalid size metric in svg: %s", str.data());
		return 0;
	}

	return size_t(ceilf(fvalue));
}

static bool BitmapFormat_detectSvg(const StringView &buf, size_t &w, size_t &h) {
	StringView str(buf);
	str.skipUntilString("<svg", false);
	if (!str.empty() && str.is<StringView::CharGroup<CharGroupId::WhiteSpace>>()) {
		bool found = false;
		bool isSvg = false;
		uint32_t width = 0;
		uint32_t height = 0;
		while (!found && !str.empty()) {
			str.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto key = html::Tag_readAttrName(str, true);
			auto value = html::Tag_readAttrValue(str, true);
			if (!key.empty() && !value.empty()) {
				if (key == "xmlns") {
					if (value.is("http://www.w3.org/2000/svg")) {
						isSvg = true;
					}
				} else if (key == "width") {
					width = BitmapFormat_detectSvgSize(value);
				} else if (key == "height") {
					height = BitmapFormat_detectSvgSize(value);
				}
				if (isSvg && width && height) {
					found = true;
				}
			}
		}
		if (isSvg) {
			w = width;
			h = height;
		}
		return isSvg;
	}

	return false;
}

static bool BitmapFormat_detectSvg(const StringView &buf) {
	size_t w = 0, h = 0;
	return BitmapFormat_detectSvg(buf, w, h);
}

static bool BitmapFormat_isSvg(const uint8_t * data, size_t dataLen) {
	if (dataLen <= 127) {
		return false;
	}

	return BitmapFormat_detectSvg(StringView((const char *)data, dataLen));
}

static bool BitmapFormat_getSvgImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_detectSvg(StringView((const char *)data.data(), data.size()), width, height)) {
		return true;
	}

	return false;
}

static bool BitmapFormat_isGif(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char GIF_SIGNATURE_1[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
    static const unsigned char GIF_SIGNATURE_2[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
    return memcmp(GIF_SIGNATURE_1, data, sizeof(GIF_SIGNATURE_1)) == 0
    		|| memcmp(GIF_SIGNATURE_2, data, sizeof(GIF_SIGNATURE_2)) == 0;
}

static bool BitmapFormat_getGifImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isGif(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 6, 4);

		width = reader.readUnsigned16();
		height = reader.readUnsigned16();

		return true;
	}
	return false;
}

static bool BitmapFormat_isTiff(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const char* TIFF_II = "II";
    static const char* TIFF_MM = "MM";

    return (memcmp(data, TIFF_II, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 42 && *(static_cast<const unsigned char*>(data) + 3) == 0) ||
        (memcmp(data, TIFF_MM, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 0 && *(static_cast<const unsigned char*>(data) + 3) == 42);
}

template <typename Reader>
static bool BitmapFormat_getTiffImageSizeImpl(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	auto reader = Reader(data.data() + 4, 4);
	auto offset = reader.readUnsigned32();

	data.clear();
	if (file.seekAndRead(offset, data, 2) != 2) {
		return false;
	}
	auto size = Reader(data.data(), 2).readUnsigned16();
	auto dictSize = size * 12;
	offset += 2;
	while (dictSize > 0) {
		data.clear();
		size_t blockSize = min(12 * 21, dictSize);
		if (file.read(data, blockSize) != blockSize) {
			return false;
		}

		auto blocks = blockSize / 12;
		reader = Reader(data.data(), blockSize);

		for (uint16_t i = 0; i < blocks; ++i) {
			auto tagid = reader.readUnsigned16();
			auto type = reader.readUnsigned16();
			auto count = reader.readUnsigned32();
			if (tagid == 256 && count == 1) {
				if (type == 3) {
					width = reader.readUnsigned16();
					reader.offset(2);
				} else if (type == 4) {
					width = reader.readUnsigned32();
				} else {
					reader.offset(4);
				}
			} else if (tagid == 257 && count == 1) {
				if (type == 3) {
					height = reader.readUnsigned16();
					reader.offset(2);
				} else if (type == 4) {
					height = reader.readUnsigned32();
				} else {
					reader.offset(4);
				}
				return true;
			} else {
				if (tagid > 257) {
					return false;
				}
				reader.offset(4);
			}
		}
	}
	return false;
}

static bool BitmapFormat_getTiffImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (BitmapFormat_isTiff(data.data(), data.size())) {
		if (memcmp(data.data(), "II", 2) == 0) {
			if (BitmapFormat_getTiffImageSizeImpl<DataReader<ByteOrder::Little>>(file, data, width, height)) {
				return true;
			}
		} else {
			if (BitmapFormat_getTiffImageSizeImpl<DataReader<ByteOrder::Big>>(file, data, width, height)) {
				return true;
			}
		}
	}
	return false;
}


static BitmapFormat s_defaultFormats[toInt(Bitmap::FileFormat::Custom)] = {
	BitmapFormat(Bitmap::FileFormat::Png, &BitmapFormat_isPng, &BitmapFormat_getPngImageSize
			, &png::loadPng, &png::writePng, &png::savePng
	),
	BitmapFormat(Bitmap::FileFormat::Jpeg, &BitmapFormat_isJpg, &BitmapFormat_getJpegImageSize
			, &jpeg::loadJpg, &jpeg::writeJpeg, &jpeg::saveJpeg
	),
	BitmapFormat(Bitmap::FileFormat::WebpLossless, &BitmapFormat_isWebpLossless, &BitmapFormat_getWebpLosslessImageSize
			, &webp::loadWebp, &webp::writeWebpLossless, &webp::saveWebpLossless
	),
	BitmapFormat(Bitmap::FileFormat::WebpLossy, &BitmapFormat_isWebp, &BitmapFormat_getWebpImageSize
			, &webp::loadWebp, &webp::writeWebpLossy, &webp::saveWebpLossy
	),
	BitmapFormat(Bitmap::FileFormat::Svg, &BitmapFormat_isSvg, &BitmapFormat_getSvgImageSize),
	BitmapFormat(Bitmap::FileFormat::Gif, &BitmapFormat_isGif, &BitmapFormat_getGifImageSize),
	BitmapFormat(Bitmap::FileFormat::Tiff, &BitmapFormat_isTiff, &BitmapFormat_getTiffImageSize),
};

static std::mutex s_formatListMutex;
static std::vector<BitmapFormat> s_formatList;

void BitmapFormat::add(const BitmapFormat &fmt) {
	s_formatListMutex.lock();
	s_formatList.emplace_back(fmt);
	s_formatListMutex.unlock();
}

BitmapFormat::BitmapFormat(FileFormat f, const check_fn &c, const size_fn &s, const load_fn &l, const write_fn &wr, const save_fn &sv)
: check_ptr(c), size_ptr(s), load_ptr(l), write_ptr(wr), save_ptr(sv), format(f) {
	assert(f != FileFormat::Custom);
	if (check_ptr && size_ptr) {
		flags |= Recognizable;
	}
	if (load_ptr) {
		flags |= Readable;
	}
	if (save_ptr || write_ptr) {
		flags |= Writable;
	}

	switch (format) {
	case FileFormat::Png: name = "PNG"; break;
	case FileFormat::Jpeg: name = "JPEG"; break;
	case FileFormat::WebpLossless: name = "WebP-lossless"; break;
	case FileFormat::WebpLossy: name = "WebP-lossy"; break;
	case FileFormat::Svg: name = "SVG"; break;
	case FileFormat::Gif: name = "GIF"; break;
	case FileFormat::Tiff: name = "TIFF"; break;
	default: break;
	}
}

BitmapFormat::BitmapFormat(const String &n, const check_fn &c, const size_fn &s, const load_fn &l, const write_fn &wr, const save_fn &sv)
: check_ptr(c), size_ptr(s), load_ptr(l), write_ptr(wr), save_ptr(sv), format(FileFormat::Custom), name(n) {
	if (check_ptr && size_ptr) {
		flags |= Recognizable;
	}
	if (load_ptr) {
		flags |= Readable;
	}
	if (save_ptr || write_ptr) {
		flags |= Writable;
	}
}

const String &BitmapFormat::getName() const {
	return name;
}

bool BitmapFormat::isRecognizable() const {
	return (flags & Recognizable) != None;
}
bool BitmapFormat::isReadable() const {
	return (flags & Readable) != None;
}
bool BitmapFormat::isWritable() const {
	return (flags & Writable) != None;
}

BitmapFormat::Flags BitmapFormat::getFlags() const {
	return flags;
}
BitmapFormat::FileFormat BitmapFormat::getFormat() const {
	return format;
}

bool BitmapFormat::is(const uint8_t * data, size_t dataLen) const {
	if (check_ptr) {
		return check_ptr(data, dataLen);
	}
	return false;
}
bool BitmapFormat::getSize(const io::Producer &file, StackBuffer<512> &buf, size_t &width, size_t &height) const {
	if (size_ptr) {
		return size_ptr(file, buf, width, height);
	}
	return false;
}

bool BitmapFormat::load(const uint8_t *data, size_t size, Bytes &out,
		Color &color, Alpha &alpha, uint32_t &width, uint32_t &height,
		uint32_t &stride, const Bitmap::StrideFn &strideFn) {
	if (load_ptr) {
		return load_ptr(data, size, out, color, alpha, width, height, stride, strideFn);
	}
	return false;
}

Bytes BitmapFormat::write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (write_ptr) {
		return write_ptr(data, width, height, stride, format, invert);
	}
	return Bytes();
}

bool BitmapFormat::save(const String &path, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Color format, bool invert) {
	if (save_ptr) {
		return save_ptr(path, data, width, height, stride, format, invert);
	}
	return false;
}

BitmapFormat::check_fn BitmapFormat::getCheckFn() const { return check_ptr; }
BitmapFormat::size_fn BitmapFormat::getSizeFn() const { return size_ptr; }
BitmapFormat::load_fn BitmapFormat::getLoadFn() const { return load_ptr; }
BitmapFormat::write_fn BitmapFormat::getWriteFn() const { return write_ptr; }
BitmapFormat::save_fn BitmapFormat::getSaveFn() const { return save_ptr; }


bool Bitmap::getImageSize(const String &path, size_t &width, size_t &height) {
	auto file = filesystem::openForReading(path);
	return getImageSize(file, width, height);
}

bool Bitmap::getImageSize(const io::Producer &file, size_t &width, size_t &height) {
	StackBuffer<512> data;
	if (file.seekAndRead(0, data, 512) < 32) {
		return false;
	}

	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		if (s_defaultFormats[i].isRecognizable() && s_defaultFormats[i].getSize(file, data, width, height)) {
			return true;
		}
	}


	Vector<BitmapFormat::size_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable()) {
			fns.emplace_back(it.getSizeFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(file, data, width, height)) {
			return true;
		}
	}

	return false;
}

bool Bitmap::isImage(const String &path, bool readable) {
	auto file = filesystem::openForReading(path);
	return isImage(file, readable);
}
bool Bitmap::isImage(const io::Producer &file, bool readable) {
	StackBuffer<512> data;
	if (file.seekAndRead(0, data, 512) < 32) {
		return false;
	}

	return isImage(data.data(), data.size(), readable);
}

bool Bitmap::isImage(const uint8_t * data, size_t dataLen, bool readable) {
	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		if (s_defaultFormats[i].isRecognizable() && (!readable || s_defaultFormats[i].isReadable())
				&& s_defaultFormats[i].is(data, dataLen)) {
			return true;
		}
	}

	Vector<BitmapFormat::check_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable() && (!readable || it.isReadable())) {
			fns.emplace_back(it.getCheckFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(data, dataLen)) {
			return true;
		}
	}

	return false;
}

bool Bitmap::check(FileFormat fmt, const uint8_t * data, size_t dataLen) {
	assert(fmt != FileFormat::Custom);
	return s_defaultFormats[toInt(fmt)].is(data, dataLen);
}

bool Bitmap::check(const String &name, const uint8_t * data, size_t dataLen) {
	Vector<BitmapFormat::check_fn> fns;

	s_formatListMutex.lock();
	fns.reserve(s_formatList.size());

	for (auto &it : s_formatList) {
		if (it.isRecognizable() && it.getName() == name) {
			fns.emplace_back(it.getCheckFn());
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it(data, dataLen)) {
			return true;
		}
	}

	return false;
}

bool Bitmap::save(const String &path, bool invert) {
	FileFormat fmt = FileFormat::Png;
	auto ext = filepath::lastExtension(path);
	if (ext == "png") {
		fmt = FileFormat::Png;
	} else if (ext == "jpeg" || ext == "jpg") {
		fmt = FileFormat::Jpeg;
	} else if (ext == "webp") {
		fmt = FileFormat::WebpLossless;
	}
	return save(fmt, path, invert);
}
bool Bitmap::save(FileFormat fmt, const String &path, bool invert) {
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.save(path, _data.data(), _width, _height, _stride, _color, invert);
	}
	return false;
}
bool Bitmap::save(const String &name, const String &path, bool invert) {
	BitmapFormat::save_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getSaveFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		return fn(path, _data.data(), _width, _height, _stride, _color, invert);
	}
	return false;
}

Bytes Bitmap::write(FileFormat fmt, bool invert) {
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.write(_data.data(), _width, _height, _stride, _color, invert);
	}
	return Bytes();
}
Bytes Bitmap::write(const String &name, bool invert) {
	BitmapFormat::write_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getWriteFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		return fn(_data.data(), _width, _height, _stride, _color, invert);
	}
	return Bytes();
}

bool Bitmap::loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn) {
	for (int i = 0; i < toInt(Bitmap::FileFormat::Custom); ++i) {
		auto &fmt = s_defaultFormats[i];
		if (fmt.is(data, dataLen) && fmt.isReadable()) {
			if (fmt.load(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
				_originalFormat = FileFormat(i);
				_originalFormatName = fmt.getName();
				return true;
			}
		}
	}

	Vector<BitmapFormat> fns;

	s_formatListMutex.lock();

	fns.reserve(s_formatList.size());
	for (auto &it : s_formatList) {
		if (it.isReadable()) {
			fns.emplace_back(it);
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it.is(data, dataLen) && it.load(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
			_originalFormat = FileFormat::Custom;
			_originalFormatName = it.getName();
			return true;
		}
	}

	return false;
}

bool Bitmap::loadData(const Bytes &d, const StrideFn &strideFn) {
	return loadData(d.data(), d.size(), strideFn);
}

NS_SP_END
