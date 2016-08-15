/*
 * SPResourceIcon.cpp
 *
 *  Created on: 10 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPResource.h"
#include "SPDrawPathNode.h"
#include "SPIcon.h"
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPString.h"

#include "SPDrawCanvas.h"
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

		path->setFillColor(Color4B(255, 255, 255, 255));

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

	void renderIcons(const std::map<std::string, std::string> &icons, uint16_t numCols, uint16_t numRows, data::Value &arr) {
		uint16_t c, r, i = 0;
		for (auto &it : icons) {
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

	std::string getIconSetFilename(const IconSet::Config &cfg) {
		return toString(_cachePath, "/", cfg.name, ".", cfg.originalWidth, "x", cfg.originalWidth,
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
		Image *img = Image::createWithFile(texFile, Image::Format::A8, true);
		if (!img) {
			return nullptr;
		}

		cocos2d::Map<std::string, Icon *> icons;

		float density = stappler::screen::density();

		auto &arr = data.getValue("icons");
		for (auto &it : arr.asArray()) {
			uint16_t x = it.getInteger("x");
			uint16_t y = it.getInteger("y");
			uint16_t id = it.getInteger("id");
			std::string name = it.getString("name");

			Icon *icon = new Icon(id, x, y, cfg.iconWidth, cfg.iconHeight, density, img);
			icons.insert(name, icon);
			icon->release();
		}

		auto set = new IconSet(std::move(cfg), std::move(icons), img);
		img->release();
		return set;
	}

	IconSet *generateFromSVGIcons(IconSet::Config &&cfg) {
		std::string filename = getIconSetFilename(cfg);

		std::string texFile = filename + ".png";
		std::string cacheFile = filename + ".cache.json";

		IconSet *ret = nullptr;

		if (auto data = validateCache(cacheFile, texFile, cfg.version, cfg.data.size())) {
			ret = readIconSet(texFile, data, std::move(cfg));
			if (ret) {
				return ret;
			}
		}

		// generate icons texture

		size_t count = cfg.data.size();
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
		data.setInteger(cfg.data.size(), "count");
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

NS_SP_EXT_END(resource)
