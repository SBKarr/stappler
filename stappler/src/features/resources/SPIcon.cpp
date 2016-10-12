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
#include "SPDrawCanvas.h"
#include "SPDrawPathNode.h"
#include "SPTextureCache.h"
#include "renderer/CCTexture2D.h"
#include "cairo.h"

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

		_canvas = Rc<draw::Canvas>::create(_canvasWidth, _canvasHeight, draw::Format::A8);

		auto cr = _canvas->getContext();
		cairo_scale(cr, (float)_canvasWidth / (float)_originalWidth, (float)_canvasHeight / (float)_originalHeight);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	}

	void renderIcon(const std::string &data, uint16_t column, uint16_t row) {
		Rc<draw::Path> path;
		if (data.compare(0, 7, "path://") == 0) {
			path = Rc<draw::Path>::create(data.substr(7));
		} else {
			path = Rc<draw::Path>::create(FilePath(data));
		}
		if (!path) {
			return;
		}

		path->setFillColor(Color4B(0, 0, 0, 0));

		cairo_t * cr = nullptr;
		if (_canvas) {
			cr = _canvas->getContext();
		}

		if (cr) {
			path->drawOn(cr);

			auto data = _canvas->data();
			uint32_t xOffset = column * _canvasWidth;
			uint32_t yOffset = row * _canvasHeight;

			for (uint32_t i = 0; i < _canvasWidth; i++) {
				for (uint32_t j = 0; j < _canvasHeight; j++) {
					uint32_t pos = j * _canvasWidth + i;
					int bufferPos = ((yOffset + j) * _bufferWidth + i + xOffset);
					_buffer[bufferPos] = data[pos];
				}
			}

			_canvas->clear();
		}
	}

	void renderIcons(const Map<String, String> *icons, uint16_t numCols, uint16_t numRows, data::Value &arr) {
		uint16_t c, r, i = 0;
		for (auto &it : (*icons)) {
			c = i % numCols;
			r = i / numCols;

			data::Value val;
			val.setInteger(i, "id");
			val.setInteger(c * _canvasWidth, "x");
			val.setInteger(r * _canvasHeight, "y");
			val.setString(it.first, "name");

			arr.addValue(std::move(val));

			renderIcon(it.second, c, r);

			i++;
		}
	}

	void setBuffer(uint8_t *buffer, size_t bufferLength, uint32_t width, uint32_t height) {
		_buffer = buffer;
		_bufferWidth = width;
		_bufferHeight = height;
		_bufferLength = bufferLength;
	}

protected:
	uint8_t *_buffer = nullptr;
	size_t _bufferLength = 0;

	uint32_t _bufferWidth = 0;
	uint32_t _bufferHeight = 0;

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

	static String getIconSetFilename(const IconSet::Config &cfg) {
		return toString(filesystem::writablePath("iconsets_cache"), "/", cfg.name, ".", cfg.originalWidth, "x", cfg.originalWidth,
				".", cfg.iconWidth, "x", cfg.iconHeight);
	}

	data::Value validateCache(const std::string &cacheFile, const std::string &texFile, uint16_t version, size_t count) {
		if (!filesystem::exists(texFile) || !filesystem::exists(cacheFile)) {
			return data::Value();
		}
		data::Value data = data::readFile(cacheFile);
		if (!data.isDictionary()
				|| (uint16_t)data.getInteger("engineVersion") != IconSetGenerator::EngineVersion()
				|| (uint16_t)data.getInteger("version") != version
				|| (size_t)data.getInteger("count") != count) {

			filesystem::remove(cacheFile);
			filesystem::remove(texFile);

			return data::Value();
		}

		auto &arr = data.getValue("icons");
		if (!arr.isArray() || arr.size() != count) {
			filesystem::remove(cacheFile);
			filesystem::remove(texFile);

			return data::Value();
		}

		return data;
	}

	IconSet *readIconSet(const std::string &texFile, const data::Value &data, IconSet::Config &&cfg) {
		Bitmap bitmap(filesystem::readFile(texFile));
		if (!bitmap) {
			return nullptr;
		}

		cocos2d::Texture2D *tex = new cocos2d::Texture2D();
		tex->initWithDataThreadSafe(bitmap.dataPtr(), bitmap.size(), cocos2d::Texture2D::PixelFormat::A8, bitmap.width(), bitmap.height(), 0);
		tex->setAliasTexParameters();

		Map<String, Icon> icons;

		float density = stappler::screen::density();

		auto &arr = data.getValue("icons");
		for (auto &it : arr.asArray()) {
			uint16_t x = it.getInteger("x");
			uint16_t y = it.getInteger("y");
			uint16_t id = it.getInteger("id");
			std::string name = it.getString("name");

			icons.emplace(name, Icon(id, x, y, cfg.iconWidth, cfg.iconHeight, density, tex));
		}

		auto set = new IconSet(std::move(cfg), std::move(icons), tex);
		tex->release();
		return set;
	}

	IconSet *generateFromSVGIcons(IconSet::Config &&cfg) {
		std::string filename = getIconSetFilename(cfg);

		std::string texFile = filename + ".png";
		std::string cacheFile = filename + ".cache.json";

		IconSet *ret = nullptr;

		if (auto data = validateCache(cacheFile, texFile, cfg.version, cfg.data->size())) {
			ret = readIconSet(texFile, data, std::move(cfg));
			if (ret) {
				return ret;
			}
		}

		// generate icons texture

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

		data::Value iconsArray(data::Value::Type::ARRAY);
		iconsArray.getArray().reserve(cfg.data->size());

		uint32_t texWidth = w;
		uint32_t texHeight = rows * cfg.iconHeight;

		std::vector<uint8_t> buffer; buffer.resize(texWidth * texHeight);
		IconSetRenderer renderer(cfg.iconWidth, cfg.iconHeight, cfg.originalWidth, cfg.originalHeight);
		renderer.setBuffer(buffer.data(), texWidth * texHeight, texWidth, texHeight);
		renderer.renderIcons(cfg.data, cols, rows, iconsArray);

		data::Value data;
		data.setValue(std::move(iconsArray), "icons");
		data.setInteger(IconSetGenerator::EngineVersion(), "engineVersion");
		data.setInteger(cfg.version, "version");
		data.setInteger(cfg.data->size(), "count");
		data.setString(cfg.name, "name");
		data::save(data, cacheFile);

		Bitmap::savePng(texFile, buffer.data(), texWidth, texHeight, Image::Format::A8);

		return readIconSet(texFile, data, std::move(cfg));
	}

protected:
	IconSetGenerator() {
		_cachePath = filesystem::writablePath("iconsets_cache");
		filesystem::mkdir(_cachePath);

	}

	std::string _cachePath;
};

IconSet *generateFromSVGIcons(IconSet::Config &&cfg) {
	return IconSetGenerator::getInstance()->generateFromSVGIcons(std::move(cfg));
}

void generateIconSet(IconSet::Config &&cfg, const IconSet::Callback &callback) {
	auto &thread = TextureCache::thread();

	IconSet::Config *cfgPtr = new (IconSet::Config) (std::move(cfg));
	IconSet **newSet = new (IconSet *)(nullptr);
	thread.perform([newSet, cfgPtr] (cocos2d::Ref *) -> bool {
		TextureCache::getInstance()->performWithGL([&] {
			*newSet = generateFromSVGIcons(std::move(*cfgPtr));
		});
		return true;
	}, [newSet, cfgPtr, callback] (cocos2d::Ref *, bool) {
		if (*newSet) {
			if (callback) {
				auto filename = IconSetGenerator::getIconSetFilename(*cfgPtr) + ".png";
				TextureCache::getInstance()->addLoadedTexture(filename, (*newSet)->getTexture());
				callback(*newSet);
			}
			(*newSet)->release();
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

IconSet::~IconSet() {
	auto filename = resource::IconSetGenerator::getIconSetFilename(_config) + ".png";
	TextureCache::getInstance()->removeLoadedTexture(filename);
}

Icon IconSet::getIcon(const std::string &key) const {
	auto it = _icons.find(key);
	if (it == _icons.end()) {
		return Icon();
	}
	return it->second;
}

NS_SP_END
