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

#ifndef MATERIAL_RESOURCES_MATERIALICONSTORAGE_H_
#define MATERIAL_RESOURCES_MATERIALICONSTORAGE_H_

#include "MaterialIconNames.h"
#include "renderer/CCTexture2D.h"
#include "SPEventHandler.h"
#include "SLPath.h"

NS_MD_BEGIN

class IconStorage : public Ref, EventHandler {
public:
	using Path = layout::Path;

	struct Icon {
		Icon() : _name(IconName::None), _x(0), _y(0), _width(0), _height(0), _density(0.0f) { }
		Icon(IconName id, uint16_t x, uint16_t y, uint16_t width, uint16_t height, float density, Path && p)
		: _name(id), _x(x), _y(y), _width(width), _height(height), _density(density), _path(std::move(p)) { }

		IconName getId() const { return _name; }
		uint16_t getWidth() const { return _width; }
		uint16_t getHeight() const { return _height; }
		float getDensity() const { return _density; }
		const Path &getPath() const { return _path; }

		Rect getTextureRect() const {
			return Rect(_x, _y, _width, _height);
		}

		bool operator == (const Icon &v) {
			return v._name == _name && v._x == _x && v._y == _y && v._width == _width && v._height == _height
					&& v._density == _density;
		}

		bool operator != (const Icon &v) {
			return !(*this == v);
		}

		IconName _name;
		uint16_t _x;
		uint16_t _y;
		uint16_t _width;
		uint16_t _height;
		float _density;
		Path _path;
	};

	static EventHeader onUpdate;

	static Path getIconPath(IconName);

	IconStorage();
	~IconStorage();

	bool init(float d, uint16_t = 24, int16_t = 24);

	void addIcon(IconName);

	const Icon * getIcon(IconName) const;
	cocos2d::Texture2D *getTexture() const;

	void update(float dt);
	void reload();

	bool matchSize(const Size &) const;

protected:
	void schedule();
	void unschedule();

	void onUpdateTexture(cocos2d::Texture2D *, Vector<Icon> &&);

	uint16_t _width = 24;
	uint16_t _height = 24;

	bool _dirty = false;
	bool _scheduled = false;
	bool _inUpdate = false;
	float _density = 1.0f;
	Vector<IconName> _names;
	Vector<Icon> _icons;
	Rc<cocos2d::Texture2D> _texture;
};

NS_MD_END

#endif /* MATERIAL_RESOURCES_MATERIALICONSTORAGE_H_ */
