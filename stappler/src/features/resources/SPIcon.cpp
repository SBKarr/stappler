/*
 * SPIcon.cpp
 *
 *  Created on: 07 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPIcon.h"
#include "SPImage.h"
#include "SPFilesystem.h"
#include "SPResource.h"
#include "SPThread.h"
#include "SPDrawCanvasCairo.h"
#include "SPDrawPathNode.h"
#include "SPTextureCache.h"
#include "SPDevice.h"
#include "renderer/CCTexture2D.h"

NS_SP_EXT_BEGIN(resource)

class IconSetGenerator;
static IconSetGenerator *s_iconGeneratorInstance = nullptr;

class IconSetRenderer {
public:
	IconSetRenderer(uint16_t canvasWidth, uint16_t canvasHeight, uint16_t originalWidth, uint16_t originalHeight) {
		_canvasWidth = canvasWidth;
		_canvasHeight = canvasHeight;

		_originalWidth = originalWidth;
		_originalHeight = originalHeight;

		_canvas.set(Rc<draw::CanvasCairo>::create());
	}

	void renderIcon(const String &data, uint16_t column, uint16_t row) {
		uint32_t xOffset = column * _canvasWidth;
		uint32_t yOffset = row * _canvasHeight;

		_canvas->save();
		_canvas->translate(xOffset / ((float)_canvasWidth / (float)_originalWidth), yOffset / ((float)_canvasHeight / (float)_originalHeight));

		Rc<draw::Path> path;
		if (data.compare(0, 7, "path://") == 0) {
			path = Rc<draw::Path>::create(data.substr(7));
		} else {
			path = Rc<draw::Path>::create(FilePath(data));
		}
		if (!path) {
			return;
		}

		path->setFillColor(Color4B(255, 255, 255, 255));
		path->drawOn(_canvas);

		_canvas->restore();
	}

	void renderIcons(cocos2d::Texture2D *tex, const Map<String, String> *source, uint16_t numCols, uint16_t numRows, Map<String, Icon> &icons) {
		_canvas->begin(tex, Color4B(0, 0, 0, 0));
		_canvas->scale((float)_canvasWidth / (float)_originalWidth, (float)_canvasHeight / (float)_originalHeight);

		uint16_t c, r, i = 0;
		for (auto &it : (*source)) {
			c = i % numCols;
			r = i / numCols;

			icons.emplace(it.first, Icon(i, c * _canvasWidth, r * _canvasHeight, _canvasWidth, _canvasHeight, _density, tex));
			renderIcon(it.second, c, r);

			i++;
		}

		_canvas->end();
	}

protected:
	float _density = stappler::screen::density();

	uint32_t _originalWidth = 0;
	uint32_t _originalHeight = 0;

	uint32_t _canvasWidth = 0;
	uint32_t _canvasHeight = 0;

	Rc<draw::Canvas> _canvas;
};

class IconSetGenerator {
public:
	static constexpr uint16_t EngineVersion() { return 1; }

	static IconSetGenerator *getInstance() {
		if (!s_iconGeneratorInstance) {
			s_iconGeneratorInstance = new IconSetGenerator();
		}
		return s_iconGeneratorInstance;
	}

	IconSet *generateFromSVGIcons(IconSet::Config &&cfg) {
		size_t count = cfg.data->size();
		size_t globalSquare = count * (cfg.iconWidth * cfg.iconHeight);

		size_t sq2 = 1; uint16_t h = 1; uint16_t w = 1;
		for (bool s = true ;sq2 < globalSquare; sq2 *= 2, s = !s) {
			if (s) { w *= 2; } else { h *= 2; }
		}

		uint32_t cols = w / cfg.iconWidth;
		uint32_t rows = (uint32_t)(count / cols + 1);

		while (rows * cols < count) {
			rows ++;
		}

		uint32_t texWidth = w;
		uint32_t texHeight = rows * cfg.iconHeight;

		Map<String, Icon> icons;
		Rc<cocos2d::Texture2D> tex = Rc<cocos2d::Texture2D>::create(cocos2d::Texture2D::PixelFormat::A8, texWidth, texHeight);
		tex->setAliasTexParameters();

		IconSetRenderer renderer(cfg.iconWidth, cfg.iconHeight, cfg.originalWidth, cfg.originalHeight);
		renderer.renderIcons(tex, cfg.data, cols, rows, icons);

		/*const cocos2d::Texture2D::PixelFormatInfo& info = cocos2d::Texture2D::_pixelFormatInfoTables.at(tex->getPixelFormat());
		Bytes buf; buf.resize(texWidth * texHeight);
		glBindTexture(GL_TEXTURE_2D, tex->getName());
		glGetTexImage(GL_TEXTURE_2D, 0, info.format, info.type, buf.data());
		glBindTexture(GL_TEXTURE_2D, 0);
		Bitmap::savePng(toString(Time::now().toMicroseconds(), ".png"), buf.data(), texWidth, texHeight, Bitmap::Format::A8);*/

		return new IconSet(std::move(cfg), std::move(icons), tex);
	}

protected:
	IconSetGenerator() { }

	String _cachePath;
};

IconSet *generateFromSVGIcons(IconSet::Config &&cfg) {
	return IconSetGenerator::getInstance()->generateFromSVGIcons(std::move(cfg));
}

void generateIconSet(IconSet::Config &&cfg, const IconSet::Callback &callback) {
	auto &thread = TextureCache::thread();

	IconSet::Config *cfgPtr = new (IconSet::Config) (std::move(cfg));
	auto newSet = new Rc<IconSet>(nullptr);
	thread.perform([newSet, cfgPtr] (cocos2d::Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			*newSet = generateFromSVGIcons(std::move(*cfgPtr));
		});
		return true;
	}, [newSet, cfgPtr, callback] (cocos2d::Ref *, bool) {
		if (*newSet) {
			IconSet * set = *newSet;
			set->onEvent(Device::onAndroidReset, [set] (const Event *ev) {
				set->reload();
			});
			if (callback) {
				callback(*newSet);
			}
		}
		delete newSet;
		delete cfgPtr;
	});
}

NS_SP_EXT_END(resource)

NS_SP_BEGIN

Icon::Icon() : _id(0), _x(0), _y(0), _width(0), _height(0), _density(0.0f) { }

Icon::Icon(uint16_t id, uint16_t x, uint16_t y, uint16_t width, uint16_t height, float density, cocos2d::Texture2D *image)
: _id(id), _x(x), _y(y), _width(width), _height(height), _density(density) {
	_image = image;
}

Icon::~Icon() { }

Rect Icon::getTextureRect() const {
	return Rect(_x, _y, _width, _height);
}

void IconSet::generate(Config &&cfg, const std::function<void(IconSet *)> &callback) {
	resource::generateIconSet(std::move(cfg), callback);
}

IconSet::IconSet(Config &&cfg, Map<String, Icon> &&icons, cocos2d::Texture2D *image)
: _config(std::move(cfg)), _image(image), _icons(std::move(icons)) {
	_texWidth = _image->getPixelsWide();
	_texHeight = _image->getPixelsHigh();
}

IconSet::~IconSet() { }


void IconSet::reload() {
	size_t count = _config.data->size();
	size_t globalSquare = count * (_config.iconWidth * _config.iconHeight);

	size_t sq2 = 1; uint16_t h = 1; uint16_t w = 1;
	for (bool s = true ;sq2 < globalSquare; sq2 *= 2, s = !s) {
		if (s) { w *= 2; } else { h *= 2; }
	}

	uint32_t cols = w / _config.iconWidth;
	uint32_t rows = (uint32_t)(count / cols + 1);

	while (rows * cols < count) {
		rows ++;
	}

	_image->init(cocos2d::Texture2D::PixelFormat::A8, w, rows * _config.iconHeight);

	resource::IconSetRenderer renderer(_config.iconWidth, _config.iconHeight, _config.originalWidth, _config.originalHeight);
	renderer.renderIcons(_image, _config.data, cols, rows, _icons);
}

Icon IconSet::getIcon(const std::string &key) const {
	auto it = _icons.find(key);
	if (it == _icons.end()) {
		return Icon();
	}
	return it->second;
}

NS_SP_END
