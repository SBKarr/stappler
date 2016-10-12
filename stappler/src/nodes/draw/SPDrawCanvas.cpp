/*
 * SPDrawPrivate.cpp
 *
 *  Created on: 13 апр. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDrawCanvas.h"
#include "renderer/CCTexture2D.h"
#include "cairo.h"

NS_SP_EXT_BEGIN(draw)

Canvas::Canvas() { }

Canvas::~Canvas() {
	if (_context) {
		cairo_destroy(_context);
		_context = nullptr;
	}
	if (_surface) {
		cairo_surface_finish(_surface);
		_surface = nullptr;
	}
	if (_shouldFreeData && _data) {
		delete [] _data;
		_data = nullptr;
	}
}

bool Canvas::init(uint32_t width, uint32_t height, Format fmt) {
	_width = width;
	_height = height;
	_format = fmt;

	cairo_format_t format;
	switch (_format) {
	case Format::A8: format = CAIRO_FORMAT_A8; break;
	case Format::RGBA8888: format = CAIRO_FORMAT_ARGB32; break;
	}

	_stride = cairo_format_stride_for_width(format, width);
	_dataLen = _stride * height;
	_data = new uint8_t[_dataLen + 1];
	memset(_data, 0, _dataLen + 1);

	_surface = cairo_image_surface_create_for_data(_data, format, (int)width, (int)height, _stride);
	_context = cairo_create(_surface);

	return (_surface != nullptr && _context != nullptr);
}

bool Canvas::init(uint8_t *data, uint32_t width, uint32_t height, uint32_t stride, Format fmt) {
	_shouldFreeData = false;

	_width = width;
	_height = height;
	_format = fmt;
	_stride = stride;

	cairo_format_t format;
	switch (_format) {
	case Format::A8: format = CAIRO_FORMAT_A8; break;
	case Format::RGBA8888: format = CAIRO_FORMAT_ARGB32; break;
	}

	_dataLen = _stride * height;
	_data = data;

	_surface = cairo_image_surface_create_for_data(_data, format, (int)width, (int)height, _stride);
	_context = cairo_create(_surface);

	return (_surface != nullptr && _context != nullptr);
}

cairo_t *Canvas::getContext() const {
	return _context;
}

cairo_t *Canvas::acquireContext() {
	cairo_save(_context);
	clear();
	_canvasStackCount ++;
	return _context;
}

void Canvas::releaseContext() {
	if (_canvasStackCount > 0) {
		cairo_restore(_context);
		_canvasStackCount--;
	}
}

cocos2d::Texture2D * Canvas::updateTexture(cocos2d::Texture2D *tex) {
	cocos2d::Texture2D::PixelFormat fmt = cocos2d::Texture2D::PixelFormat::NONE;
	if (_format == Format::A8) {
		fmt = cocos2d::Texture2D::PixelFormat::A8;
	} else if (_format == Format::RGBA8888) {
		fmt = cocos2d::Texture2D::PixelFormat::RGBA8888;
	}

	if (!tex || !match(tex->getPixelsWide(), tex->getPixelsHigh(), _format) || tex->getPixelFormat() != fmt) {
		tex = new cocos2d::Texture2D();
		tex->initWithData(_data, _dataLen, fmt, _width, _height, _stride);
	} else {
		tex->updateWithData(_data, 0, 0, _width, _height, _stride);
	}
	return tex;
}

cocos2d::Texture2D * Canvas::generateTexture(cocos2d::Texture2D *tex) {
	tex = updateTexture(tex);
	return tex;
}

bool Canvas::match(uint32_t w, uint32_t h, Format fmt) const {
	return w == _width && h == _height && _format == fmt;
}

void Canvas::clear() {
	if (_data) {
		memset(_data, 0, _dataLen + 1);
	}
}

void Canvas::clear(const cocos2d::Color4B &color) {
	cairo_save(_context);
	cairo_set_source_rgba(_context, color.b / 255.0, color.g / 255.0, color.r / 255.0, color.a / 255.0);
	cairo_set_operator(_context, CAIRO_OPERATOR_SOURCE);
	cairo_paint(_context);
	cairo_restore(_context);
}

NS_SP_EXT_END(draw)
