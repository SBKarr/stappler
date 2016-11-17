/*
 * SPIcon.h
 *
 *  Created on: 07 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_
#define LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_

#include "SPImage.h"
#include "base/CCMap.h"

NS_SP_BEGIN

class Icon {
public:
	Icon();
	Icon(uint16_t id, uint16_t x, uint16_t y, uint16_t width, uint16_t height, float density, cocos2d::Texture2D *tex);

	virtual ~Icon();

	inline uint16_t getId() const { return _id; }
	inline uint16_t getWidth() const { return _width; }
	inline uint16_t getHeight() const { return _height; }
	inline float getDensity() const { return _density; }

	cocos2d::Texture2D *getTexture() const { return _image; }
	Rect getTextureRect() const;

	operator bool() const {
		return bool(_image);
	}

	bool operator == (const Icon &v) {
		return v._id == _id && v._x == _x && v._y == _y && v._width == _width && v._height == _height
				&& v._density == _density && v._image == _image;
	}

	bool operator != (const Icon &v) {
		return !(*this == v);
	}

private:
	uint16_t _id;
	uint16_t _x;
	uint16_t _y;
	uint16_t _width;
	uint16_t _height;
	float _density;
	Rc<cocos2d::Texture2D> _image;
};

class IconSet : public cocos2d::Ref {
public:
	using Callback = std::function<void(IconSet *)>;
	struct Config {
		std::string name;
		uint16_t version;
		const Map<String, String> * data;
		uint16_t originalWidth;
		uint16_t originalHeight;
		uint16_t iconWidth;
		uint16_t iconHeight;
	};

	static void generate(Config &&, const Callback &callback);

	Icon getIcon(const std::string &) const;

	IconSet(Config &&, Map<String, Icon> &&icons, cocos2d::Texture2D *image);
	~IconSet();

	const Config &getConfig() const { return _config; }

	uint16_t getOriginalWidth() const { return _config.originalWidth; }
	uint16_t getOriginalHeight() const { return _config.originalHeight; }
	uint16_t getIconHeight() const { return _config.iconHeight; }
	uint16_t getIconWidth() const { return _config.iconWidth; }

	const std::string &getName() const { return _config.name; }
	uint16_t getVersion() const { return _config.version; }

	cocos2d::Texture2D *getTexture() const { return _image; }

protected:
	Config _config;
	uint16_t _texWidth = 0;
	uint16_t _texHeight = 0;

	Rc<cocos2d::Texture2D> _image;
	Map<String, Icon> _icons;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_ICONS_SPICON_H_ */
