// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPBitmap.h"
#include "SPBuffer.h"

#include "SPFilesystem.h"
#include "SPDataReader.h"
#include "SPString.h"
#include "SPLog.h"

#ifndef NOJPEG
#include "jpeglib.h"
#endif

#ifndef NOPNG
#include "png.h"
#endif

NS_SP_BEGIN

using Color = Bitmap::Format;
using Alpha = Bitmap::Alpha;

template<Color Source, Color Target>
void convertLine(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs);

template<>
void convertLine<Color::RGB888, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	for (size_t i = 0, l = ins - 2; i < l; i += 3) {
		*out++ = in[i];			//R
		*out++ = in[i + 1];		//G
		*out++ = in[i + 2];		//B
		*out++ = 0xFF;			//A
	}
}

template<>
void convertLine<Color::I8, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
    }
}

template<>
void convertLine<Color::IA88, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
    }
}

template<>
void convertLine<Color::I8, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
        *out++ = 0xFF;      //A
    }
}

template<>
void convertLine<Color::IA88, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i];     //R
        *out++ = in[i];     //G
        *out++ = in[i];     //B
        *out++ = in[i + 1]; //A
    }
}

template<>
void convertLine<Color::I8, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = in[i];
		*out++ = 0xFF;
    }
}

template<>
void convertLine<Color::IA88, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 1; i < ins; i += 2) {
        *out++ = in[i]; //A
    }
}

template<>
void convertLine<Color::IA88, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 1; i < l; i += 2) {
        *out++ = in[i]; //R
    }
}

template<>
void convertLine<Color::RGBA8888, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = in[i];         //R
        *out++ = in[i + 1];     //G
        *out++ = in[i + 2];     //B
    }
}

template<>
void convertLine<Color::RGB888, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 2; i < l; i += 3) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
    }
}

template<>
void convertLine<Color::RGBA8888, Color::I8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
    }
}

template<>
void convertLine<Color::RGBA8888, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins -3; i < l; i += 4) {
        *out++ = in[i + 3]; //A
    }
}

template<>
void convertLine<Color::RGB888, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 2; i < l; i += 3) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
        *out++ = 0xFF;
    }
}

template<>
void convertLine<Color::RGBA8888, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0, l = ins - 3; i < l; i += 4) {
        *out++ = (in[i] * 299 + in[i + 1] * 587 + in[i + 2] * 114 + 500) / 1000;  //I =  (R*299 + G*587 + B*114 + 500) / 1000
        *out++ = in[i + 3];
    }
}

template<>
void convertLine<Color::A8, Color::IA88>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = 0xFF;
        *out++ = in[i];
    }
}

template<>
void convertLine<Color::A8, Color::RGB888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	memset(out, 0, outs);
}

template<>
void convertLine<Color::A8, Color::RGBA8888>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
    for (size_t i = 0; i < ins; ++i) {
        *out++ = 0x00;
        *out++ = 0x00;
        *out++ = 0x00;
        *out++ = in[i];
    }
}

template<>
void convertLine<Color::RGB888, Color::A8>(const uint8_t *in, uint8_t *out, uint32_t ins, uint32_t outs) {
	memset(out, 0, outs);
}


template<Color Source, Color Target>
bool convertData(const Bytes &dataVec, Bytes & out, uint32_t inStride, uint32_t outStride) {
	auto dataLen = dataVec.size();
	auto height = dataLen / inStride;
	auto data = dataVec.data();
	out.resize(height * outStride);
	auto outData = out.data();
	for (size_t j = 0; j < height; j ++) {
		convertLine<Source, Target>(data + inStride * j, outData + outStride * j, inStride, outStride);
	}
	return true;
}

template <typename Reader>
static bool Bitmap_getTiffImageSize(StackBuffer<256> &data, const io::Producer &file, size_t &width, size_t &height) {
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

bool Bitmap::getImageSize(const String &path, size_t &width, size_t &height) {
	auto file = filesystem::openForReading(path);
	return getImageSize(file, width, height);
}

bool Bitmap::getImageSize(const io::Producer &file, size_t &width, size_t &height) {
	StackBuffer<256> data;
    if (file.seekAndRead(0, data, 30) < 30) {
    	return false;
    }

	if (isPng(data.data(), data.size()) && data.size() >= 24) {
		auto reader = DataReader<ByteOrder::Network>(data.data() + 16, 8);

		width = reader.readUnsigned32();
		height = reader.readUnsigned32();

	    return true;
	} else if(isJpg(data.data(), data.size())) {
		size_t offset = 2;
		uint16_t len = 0;
		uint8_t marker = 0;

		auto reader = DataReader<ByteOrder::Network>(data.data() + 2, data.size() - 2);
		while (reader.is((uint8_t)0xFF)) {
			++ reader;
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

				while (reader.is((uint8_t)0xFF)) {
					++ reader;
				}

				marker = reader.readUnsigned();
				len = reader.readUnsigned16();
			} else {
				reader.clear();
				break;
			}
		}

		if (reader >= 5 && marker >= 0xC0 && marker <= 0xCF) {
			++ reader;
			height = reader.readUnsigned16();
			width = reader.readUnsigned16();
			return true;
		}

		return false;
	} else if (isGif(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 6, 4);

		width = reader.readUnsigned16();
		height = reader.readUnsigned16();

	    return true;
	} else if (isWebp(data.data(), data.size())) {
		auto reader = DataReader<ByteOrder::Little>(data.data() + 26, 4);

		width = reader.readUnsigned16();
		height = reader.readUnsigned16();

	    return true;
	} else if (isTiff(data.data(), data.size())) {
		if (memcmp(data.data(), "II", 2) == 0) {
			if (Bitmap_getTiffImageSize<DataReader<ByteOrder::Little>>(data, file, width, height)) {
				return true;
			}
		} else {
			if (Bitmap_getTiffImageSize<DataReader<ByteOrder::Big>>(data, file, width, height)) {
				return true;
			}
		}
	}

    return false;
}

bool Bitmap::isGif(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char GIF_SIGNATURE_1[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
    static const unsigned char GIF_SIGNATURE_2[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
    return memcmp(GIF_SIGNATURE_1, data, sizeof(GIF_SIGNATURE_1)) == 0
    		|| memcmp(GIF_SIGNATURE_2, data, sizeof(GIF_SIGNATURE_2)) == 0;
}

bool Bitmap::isPng(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 8) {
        return false;
    }

    static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return memcmp(PNG_SIGNATURE, data, sizeof(PNG_SIGNATURE)) == 0;
}

bool Bitmap::isJpg(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const unsigned char JPG_SOI[] = {0xFF, 0xD8};
    return memcmp(data, JPG_SOI, 2) == 0;
}

bool Bitmap::isTiff(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 4) {
        return false;
    }

    static const char* TIFF_II = "II";
    static const char* TIFF_MM = "MM";

    return (memcmp(data, TIFF_II, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 42 && *(static_cast<const unsigned char*>(data) + 3) == 0) ||
        (memcmp(data, TIFF_MM, 2) == 0 && *(static_cast<const unsigned char*>(data) + 2) == 0 && *(static_cast<const unsigned char*>(data) + 3) == 42);
}

bool Bitmap::isWebp(const uint8_t * data, size_t dataLen) {
    if (dataLen <= 12) {
        return false;
    }

    static const char* WEBP_RIFF = "RIFF";
    static const char* WEBP_WEBP = "WEBP";

    return memcmp(data, WEBP_RIFF, 4) == 0
        && memcmp(static_cast<const unsigned char*>(data) + 8, WEBP_WEBP, 4) == 0;
}

#ifndef NOJPEG
struct Bitmap_JpegError {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */

    static void ErrorExit(j_common_ptr cinfo) {
    	Bitmap_JpegError * myerr = (Bitmap_JpegError *) cinfo->err;
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
	struct Bitmap_JpegError jerr;

	JSAMPROW row_pointer[1] = {0};
	unsigned long location = 0;

	bool ret = false;
	do {
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = &Bitmap_JpegError::ErrorExit;
		if (setjmp(jerr.setjmp_buffer))	{
			jpeg_destroy_decompress(&cinfo);
			break;
		}

		jpeg_create_decompress( &cinfo );

#ifndef CC_TARGET_QT5
		jpeg_mem_src(&cinfo, const_cast<unsigned char*>(inputData), size);
#endif /* CC_TARGET_QT5 */

		/* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
		// libjpeg 0.9 adds stricter types.
		jpeg_read_header(&cinfo, TRUE);
#else
		jpeg_read_header(&cinfo, TRUE);
#endif

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
#endif

#ifndef NOPNG
struct Bitmap_readState {
	const uint8_t *data = nullptr;
	size_t offset = 0;
};

void Bitmap_readDynamicData(png_structp pngPtr, png_bytep data, png_size_t length) {
	auto state = (Bitmap_readState *)png_get_io_ptr(pngPtr);
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

	Bitmap_readState state;
	state.data = inputData;
	state.offset = 0;

	png_set_read_fn(png_ptr,(png_voidp)&state, Bitmap_readDynamicData);
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

struct Bitmap_PngStruct {
	int bit_depth = 8;
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	bool valid = false;

	FILE *fp = nullptr;
	Bytes *vec = nullptr;

	~Bitmap_PngStruct() {
		if (png_ptr != nullptr) {
	        png_destroy_write_struct(&png_ptr, &info_ptr);
		}

		if (fp) {
	        fclose(fp);
		}
	}

	Bitmap_PngStruct() {
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

	Bitmap_PngStruct(Bytes *v) : Bitmap_PngStruct() {
		vec = v;
	    valid = true;
	}

	Bitmap_PngStruct(const String &filename) : Bitmap_PngStruct() {
	    fp = filesystem_native::fopen_fn(filename, "wb");
	    if (!fp) {
	        log::format("Bitmap", "fail to open file %s to write png data", filename.c_str());
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

	bool write(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Bitmap::Format format, bool invert = false) {
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
		case Bitmap::Format::A8:
		case Bitmap::Format::I8:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		case Bitmap::Format::IA88:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case Bitmap::Format::RGB888:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		case Bitmap::Format::RGBA8888:
			color_type = PNG_COLOR_TYPE_RGBA;
			break;
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

void Bitmap::savePng(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Format format, bool invert) {
	Bitmap_PngStruct s(filename);
	s.write(data, width, height, stride, format, invert);
}

Bytes Bitmap::writePng(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Format format, bool invert) {
    Bytes state;
    Bitmap_PngStruct s(&state);
	if (s.write(data, width, height, stride, format, invert)) {
		return state;
	} else {
		return Bytes();
	}
}

#else
void Bitmap::savePng(const String &, const uint8_t *, uint32_t, uint32_t, uint32_t, Format, bool) { }
Bytes Bitmap::writePng(const uint8_t *, uint32_t, uint32_t, uint32_t, Format, bool) { return Bytes(); }
#endif

void Bitmap::savePng(const String &filename, const uint8_t *data, uint32_t width, uint32_t height, Format format, bool invert) {
	savePng(filename, data, width, height, 0, format, invert);
}

Bytes Bitmap::writePng(const uint8_t *data, uint32_t width, uint32_t height, Format format, bool invert) {
	return writePng(data, width, height, 0, format, invert);
}

uint8_t Bitmap::getBytesPerPixel(Format c) {
	switch (c) {
	case Format::A8:
	case Format::I8:
		return 1;
		break;
	case Format::IA88:
		return 2;
		break;
	case Format::RGB888:
		return 3;
		break;
	case Format::RGBA8888:
		return 4;
		break;
	}
	return 0;
}

Bitmap::Bitmap() { }

Bitmap::Bitmap(const Bytes &vec, const StrideFn &strideFn)
: Bitmap(vec.data(), vec.size(), strideFn) { }

Bitmap::Bitmap(const uint8_t *data, size_t size, const StrideFn &strideFn) {
	if (!loadData(data, size, strideFn)) {
		_data.clear();
	}
}

Bitmap::Bitmap(const uint8_t *d, uint32_t width, uint32_t height, Format c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(d, d + _stride * height) { }

Bitmap::Bitmap(const Bytes &d, uint32_t width, uint32_t height, Format c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(d) { }

Bitmap::Bitmap(Bytes &&d, uint32_t width, uint32_t height, Format c, Alpha a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height), _stride(max(stride, width * getBytesPerPixel(c))), _data(std::move(d)) { }

Bitmap::Bitmap(Bitmap &&other)
: _color(other._color), _alpha(other._alpha), _width(other._width), _height(other._height), _stride(other._stride), _data(std::move(other._data)) {
	other._data.clear();
}

Bitmap &Bitmap::operator =(Bitmap &&other) {
	_alpha = other._alpha;
	_color = other._color;
	_width = other._width;
	_height = other._height;
	_stride = other._stride;
	_data = std::move(other._data);
	other._data.clear();
	return *this;
}

bool Bitmap::loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn) {
	bool ret = false;
	if (isPng(data, dataLen)) {
#ifndef NOPNG
		if (loadPng(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
			ret = true;
		}
#endif
	} else if (isJpg(data, dataLen)) {
#ifndef NOJPEG
		if (loadJpg(data, dataLen, _data, _color, _alpha, _width, _height, _stride, strideFn)) {
			ret = true;
		}
#endif
	} /* else if (isWebp(data, dataLen)) {
		if (loadWebp(data, dataLen, _data, _color, _alpha, _width, _height)) {
			ret = true;
		}
	} */
	return ret;
}

bool Bitmap::loadData(const Bytes &d, const StrideFn &strideFn) {
	return loadData(d.data(), d.size(), strideFn);
}

void Bitmap::loadBitmap(const uint8_t *d, uint32_t w, uint32_t h, Format c, Alpha a, uint32_t stride) {
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
	memcpy(_data.data(), d, _data.size());
}

void Bitmap::loadBitmap(const Bytes &d, uint32_t w, uint32_t h, Format c, Alpha a, uint32_t stride) {
	setInfo(w, h, c, a, stride);
	_data = d;
}

void Bitmap::loadBitmap(Bytes &&d, uint32_t w, uint32_t h, Format c, Alpha a, uint32_t stride) {
	setInfo(w, h, c, a, stride);
	_data = std::move(d);
}

void Bitmap::alloc(uint32_t w, uint32_t h, Format c, Alpha a, uint32_t stride) {
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
}

void Bitmap::save(const String &filename) {
	savePng(filename, _data.data(), _width, _height, _stride, _color);
}

void Bitmap::setInfo(uint32_t w, uint32_t h, Format c, Alpha a, uint32_t stride) {
	_width = w;
	_height = h;
	_stride = max(stride, w * getBytesPerPixel(c));
	_color = c;
	_alpha = a;
}

bool Bitmap::convert(Format color, const StrideFn &strideFn) {
	bool ret = false;
	Bytes out;
	uint32_t outStride = (strideFn)?max(strideFn(color, _width), _width*getBytesPerPixel(color)):_width*getBytesPerPixel(color);
	if (_color == color && outStride != _stride) {
		out.resize(_height * outStride);
		size_t minStride = _width * getBytesPerPixel(_color);
		for (size_t j = 0; j < _height; j ++) {
			memcpy(out.data() + j * outStride, _data.data() + j * _stride, minStride);
		}
		_data = std::move(out);
		_stride = outStride;
		return true;
	}
	switch (_color) {
	case Format::A8:
		switch (color) {
		case Format::A8: return true; break;
		case Format::I8: _color = color; return true; break;
		case Format::IA88: ret = convertData<Format::A8, Format::IA88>(_data, out, _stride, outStride); break;
		case Format::RGB888: ret = convertData<Format::A8, Format::RGB888>(_data, out, _stride, outStride); break;
		case Format::RGBA8888: ret = convertData<Format::A8, Format::RGBA8888>(_data, out, _stride, outStride); break;
		}
		break;
	case Format::I8:
		switch (color) {
		case Format::A8: _color = color; return true; break;
		case Format::I8: return true; break;
		case Format::IA88: ret = convertData<Format::I8, Format::IA88>(_data, out, _stride, outStride); break;
		case Format::RGB888: ret = convertData<Format::I8, Format::RGB888>(_data, out, _stride, outStride); break;
		case Format::RGBA8888: ret = convertData<Format::I8, Format::RGBA8888>(_data, out, _stride, outStride); break;
		}
		break;
	case Format::IA88:
		switch (color) {
		case Format::A8: ret = convertData<Format::IA88, Format::A8>(_data, out, _stride, outStride); break;
		case Format::I8: ret = convertData<Format::IA88, Format::I8>(_data, out, _stride, outStride); break;
		case Format::IA88: return true; break;
		case Format::RGB888: ret = convertData<Format::IA88, Format::RGB888>(_data, out, _stride, outStride); break;
		case Format::RGBA8888: ret = convertData<Format::IA88, Format::RGBA8888>(_data, out, _stride, outStride); break;
		}
		break;
	case Format::RGB888:
		switch (color) {
		case Format::A8: ret = convertData<Format::RGB888, Format::A8>(_data, out, _stride, outStride); break;
		case Format::I8: ret = convertData<Format::RGB888, Format::I8>(_data, out, _stride, outStride); break;
		case Format::IA88: ret = convertData<Format::RGB888, Format::IA88>(_data, out, _stride, outStride); break;
		case Format::RGB888: return true; break;
		case Format::RGBA8888: ret = convertData<Format::RGB888, Format::RGBA8888>(_data, out, _stride, outStride); break;
		}
		break;
	case Format::RGBA8888:
		switch (color) {
		case Format::A8: ret = convertData<Format::RGBA8888, Format::A8>(_data, out, _stride, outStride); break;
		case Format::I8: ret = convertData<Format::RGBA8888, Format::I8>(_data, out, _stride, outStride); break;
		case Format::IA88: ret = convertData<Format::RGBA8888, Format::IA88>(_data, out, _stride, outStride); break;
		case Format::RGB888: ret = convertData<Format::RGBA8888, Format::RGB888>(_data, out, _stride, outStride); break;
		case Format::RGBA8888: return true; break;
		}
		break;
	}

	if (ret) {
		_color = color;
		_data = std::move(out);
		_stride = outStride;
	}

	return ret;
}

NS_SP_END
