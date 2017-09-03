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

#ifndef COMMON_UTILS_BITMAP_SPBITMAPFORMATJPEG_HPP_
#define COMMON_UTILS_BITMAP_SPBITMAPFORMATJPEG_HPP_

#include "SPCommon.h"
#include "SPBitmap.h"
#include "SPLog.h"

#include <setjmp.h>
#include "jpeglib.h"

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

#endif /* COMMON_UTILS_BITMAP_SPBITMAPFORMATJPEG_HPP_ */
