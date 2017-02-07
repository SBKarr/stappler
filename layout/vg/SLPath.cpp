// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPLayout.h"
#include "SLPath.h"
#include "SPFilesystem.h"

NS_LAYOUT_BEGIN

#define SP_PATH_LOG(...)
//#define SP_PATH_LOG(...) stappler::logTag("Path Debug", __VA_ARGS__)

class SVGPathReader : public ReaderClassBase<char> {
public:
	static bool readFile(Path *p, const String &str) {
		if (!str.empty()) {
			auto content = filesystem::readTextFile(str);
			CharReaderBase r(content);
			r.skipUntilString("<path ");
			if (!r.is("<path ")) {
				return false;
			}

			r.skipString("<path ");
			CharReaderBase pathContent = r.readUntil<Chars<'>'>>();
			pathContent.skipUntilString("d=\"");
			if (r.is("d=\"")) {
				r.skipString("d=\"");
				return readPath(p, pathContent.readUntil<Chars<'"'>>());
			}
		}
		return false;
	}

	static bool readPath(Path *p, const String &str) {
		return readPath(p, CharReaderBase(str));
	}

	static bool readPath(Path *p, const CharReaderBase &r) {
		if (r.size() > 0) {
			SVGPathReader reader(p, r);
			return reader.parse();
		}
		return false;
	}

protected:
	bool parse() {
		while (!reader.empty()) {
			if (!readCmdGroup()) {
				return false;
			}
		}

		return true;
	}

	bool readCmdGroup() {
		readWhitespace();
		while (!reader.empty()) {
			if (!readCmd()) {
				return false;
			}
		}
		return true;
	}

	bool readCmd() {
		if (!readMoveTo()) {
			return false;
		}
		readWhitespace();

		bool readNext = true;
		while(readNext) {
			readNext = readDrawTo();
			if (readNext) {
				readWhitespace();
			}
		}

		return true;
	}

	bool readMoveTo() {
		if (reader >= 1) {
			readWhitespace();
			bool relative = true;
			if (reader.is('M')) {
				relative = false; ++ reader;
			} else if (reader.is('m')) {
				relative = true; ++ reader;
			} else {
				return false;
			}

			readWhitespace();
			return readMoveToArgs(relative);
		}
		return false;
	}

	bool readDrawTo() {
		if (reader >= 1) {
			auto c = reader[0];
			++ reader;

			readWhitespace();
			if (c == 'M' || c == 'm') {
				return readMoveToArgs(c == 'm');
			} else if (c == 'Z' || c == 'z') {
				SP_PATH_LOG("Z");
				if (_pathStarted) {
					_x = _sx;
					_y = _sy;
					_pathStarted = false;
				}
				path->closePath();
				return true;
			} else if (c == 'L' || c == 'l') {
				return readLineToArgs(c == 'l');
			} else if (c == 'H' || c == 'h') {
				return readHorizontalLineTo(c == 'h');
			} else if (c == 'V' || c == 'v') {
				return readVerticalLineTo(c == 'v');
			} else if (c == 'C' || c == 'c') {
				return readCubicBezier(c == 'c');
			} else if (c == 'S' || c == 's') {
				return readCubicBezierShort(c == 's');
			} else if (c == 'Q' || c == 'q') {
				return readQuadraticBezier(c == 'q');
			} else if (c == 'T' || c == 't') {
				return readQuadraticBezierShort(c == 't');
			} else if (c == 'A' || c == 'a') {
				return readEllipticalArc(c == 'a');
			}
		}
		return false;
	}

	bool readLineToArgs(bool relative) {
		float x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readCoordPair(x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { x = _x + x; y = _y + y; }
				_x = x; _y = y; _b = false;
				SP_PATH_LOG("L %f %f", _x, _y);
				path->lineTo(x, y);
			}
		}
		return ret;
	}

	bool readHorizontalLineTo(bool relative) {
		float x;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readNumber(x);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { x = _x + x; }
				_x = x; _b = false;

				SP_PATH_LOG("H %f", _x);
				path->lineTo(x, _y);
			}
		}
		return ret;
	}

	bool readVerticalLineTo(bool relative) {
		float y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readNumber(y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { y = _y + y; }
				_y = y; _b = false;
				SP_PATH_LOG("V %f", _y);
				path->lineTo(_x, y);
			}
		}
		return ret;
	}

	bool readCubicBezier(bool relative) {
		float x1, y1, x2, y2, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readCurveToArg(x1, y1, x2, y2, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x1 = _x + x1; y1 = _y + y1;
					x2 = _x + x2; y2 = _y + y2;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x2, _by = y2; _b = true;
				SP_PATH_LOG("C %f %f %f %f %f %f", x1, y1, x2, y2, x, y);
				path->cubicTo(x1, y1, x2, y2, x, y);
			}
		}
		return ret;
	}

	bool readCubicBezierShort(bool relative) {
		float x1, y1, x2, y2, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readSmoothCurveToArg(x2, y2, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }

				getNewBezierParams(x1, y1);
				if (relative) {
					x2 = _x + x2; y2 = _y + y2;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x2, _by = y2; _b = true;
				SP_PATH_LOG("S (%f %f) %f %f %f %f", x1, y1, x2, y2, x, y);
				path->cubicTo(x1, y1, x2, y2, x, y);
			}
		}
		return ret;
	}

	bool readQuadraticBezier(bool relative) {
		float x1, y1, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readQuadraticCurveToArg(x1, y1, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x1 = _x + x1; y1 = _y + y1;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x1, _by = y1; _b = true;
				path->quadTo(x1, y1, x, y);
			}
		}
		return ret;
	}

	bool readQuadraticBezierShort(bool relative) {
		float x1, y1, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readSmoothQuadraticCurveToArg(x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				getNewBezierParams(x1, y1);
				if (relative) {
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x1, _by = y1; _b = true;
				path->quadTo(x1, y1, x, y);
			}
		}
		return ret;
	}

	bool readEllipticalArc(bool relative) {
		float rx, ry, xAxisRotation, x, y;
		bool largeArc, sweep;

		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readEllipticalArcArg(rx, ry, xAxisRotation, largeArc, sweep, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x = _x + x; y = _y + y;
				}

				if (rx == 0 || ry == 0) {
					_x = x; _y = y; _b = false;
					path->lineTo(x, y);
				} else {
					_x = x; _y = y; _b = false;
					path->arcTo(rx, ry, xAxisRotation, largeArc, sweep, x, y);
				}
			}
		}
		return ret;
	}

	bool readMoveToArgs(bool relative) {
		float x = 0.0f, y = 0.0f;
		if (!readCoordPair(x, y)) {
			return false;
		}

		if (relative) {
			x = _x + x;
			y = _y + y;
		}

		_b = false;
		_x = x;
		_y = y;
		if (!_pathStarted) {
			_sx = _x;
			_sy = _y;
			_pathStarted = true;
		}

		SP_PATH_LOG("M %f %f", _x, _y);
		path->moveTo(x, y);
		readCommaWhitespace();
		readLineToArgs(relative);

		return true;
	}

	bool readCurveToArg(float &x1, float &y1, float &x2, float &y2, float &x, float &y) {
		if (!readCoordPair(x1, y1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x2, y2)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}
		return true;
	}

	bool readSmoothCurveToArg(float &x2, float &y2, float &x, float &y) {
		return readQuadraticCurveToArg(x2, y2, x, y);
	}

	bool readQuadraticCurveToArg(float &x1, float &y1, float &x, float &y) {
		if (!readCoordPair(x1, y1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}
		return true;
	}

	bool readEllipticalArcArg(float &_rx, float &_ry, float &_xAxisRotation,
			bool &_largeArc, bool &_sweep, float &_dx, float &_dy) {
		float rx, ry, xAxisRotation, x, y;
		bool largeArc, sweep;

		if (!readCoordPair(rx, ry)) {
			return false;
		}
		readCommaWhitespace();
		if (!readNumber(xAxisRotation)) {
			return false;
		}

		if (!readCommaWhitespace()) {
			return false;
		}

		if (!readFlag(largeArc)) {
			return false;
		}

		readCommaWhitespace();
		if (!readFlag(sweep)) {
			return false;
		}

		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}

		_rx = rx;
		_ry = ry;
		_xAxisRotation = xAxisRotation;
		_largeArc = largeArc;
		_sweep = sweep;
		_dx = x;
		_dy = y;

		return true;
	}

	bool readSmoothQuadraticCurveToArg(float &x, float &y) {
		return readCoordPair(x, y);
	}

	bool readCoordPair(float &x, float &y) {
		float value1 = 0.0f, value2 = 0.0f;
		if (!readNumber(value1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readNumber(value2)) {
			return false;
		}

		x = value1;
		y = value2;
		return true;
	}

	bool readWhitespace() { return reader.readChars<Group<GroupId::WhiteSpace>>().size() != 0; }

	bool readCommaWhitespace() {
		if (reader >= 1) {
			bool ws = readWhitespace();
			if (reader.is(',')) {
				++ reader;
			} else {
				return ws;
			}
			readWhitespace();
			return true;
		}
		return false;
	}

	bool readNumber(float &val) {
		if (!reader.empty()) {
			auto tmp = reader.readFloat();
			if (IsErrorValue(tmp)) {
				return false;
			}
			val = tmp;
			return true;
		}
		return false;
	}

	bool readFlag(bool &flag) {
		if (reader >= 1) {
			if (reader.is('0') || reader.is('1')) {
				flag = (reader.is('1'));
				++ reader;
				return true;
			}
		}
		return false;
	}

	void getNewBezierParams(float &bx, float &by) {
		if (_b) {
			bx = _x * 2 - _bx; by = _y * 2 - _by;
		} else {
			bx = _x; by = _y;
		}
	}

	SVGPathReader(Path *p, const CharReaderBase &r)
	: path(p), reader(r) { }

	float _x = 0.0f, _y = 0.0f;

	bool _b = false;
	float _bx = 0.0f, _by = 0.0f;

	float _sx = 0.0f; float _sy = 0.0f;
	bool _pathStarted = false;
	Path *path = nullptr;
	CharReaderBase reader;
};

Path::Path() { }
Path::Path(size_t count) {
	_commands.reserve(count);
}

bool Path::init() {
	return true;
}

bool Path::init(const String &str) {
	if (!SVGPathReader::readPath(this, str)) {
		return false;
	}
	return true;
}

bool Path::init(const CharReaderBase &str) {
	if (!SVGPathReader::readPath(this, str)) {
		return false;
	}
	return true;
}

bool Path::init(FilePath &&str) {
	if (!SVGPathReader::readFile(this, str.get())) {
		return false;
	}
	return true;
}

size_t Path::count() const {
	return _commands.size();
}

Path & Path::moveTo(float x, float y) {
	_commands.emplace_back(Command::MakeMoveTo(x, y));
	return *this;
}

Path & Path::lineTo(float x, float y) {
	_commands.emplace_back(Command::MakeLineTo(x, y));
	return *this;
}

Path & Path::quadTo(float x1, float y1, float x2, float y2) {
	_commands.emplace_back(Command::MakeQuadTo(x1, y1, x2, y2));
	return *this;
}

Path & Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	_commands.emplace_back(Command::MakeCubicTo(x1, y1, x2, y2, x3, y3));
	return *this;
}

Path & Path::arcTo(float rx, float ry, float angle, bool largeFlag, bool sweepFlag, float x, float y) {
	_commands.emplace_back(Command::MakeArcTo(rx, ry, angle, largeFlag, sweepFlag, x, y));
	return *this;
}
Path & Path::closePath() {
	_commands.emplace_back(Command::MakeClosePath());
	return *this;
}

Path & Path::addRect(const Rect& rect) {
	return addRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}
Path & Path::addRect(float x, float y, float width, float height) {
	moveTo(x, y);
	lineTo(x + width, y);
	lineTo(x + width, y + height);
	lineTo(x, y + height);
	closePath();
	return *this;
}
Path & Path::addOval(const Rect& oval) {
	Vec2 r(oval.size.width / 2.0f, oval.size.height / 2.0f);
	moveTo(oval.getMaxX(), oval.getMidY());
	arcTo(r.x, r.y, 0, true, false, oval.getMinX(), oval.getMidY());
	arcTo(r.x, r.y, 0, true, false, oval.getMaxX(), oval.getMidY());
	closePath();
	return *this;
}
Path & Path::addCircle(float x, float y, float radius) {
	moveTo(x + radius, y);
	arcTo(radius, radius, 0, true, false, x - radius, y);
	arcTo(radius, radius, 0, true, false, x + radius, y);
	closePath();
	return *this;
}

Path & Path::addEllipse(float x, float y, float rx, float ry) {
	moveTo(x + rx, y);
	arcTo(rx, ry, 0, true, false, x - rx, y);
	arcTo(rx, ry, 0, true, false, x + rx, y);
	closePath();
	return *this;
}

Path & Path::addArc(const Rect& oval, float startAngle, float sweepAngle) {
	const auto rx = oval.size.width / 2;
	const auto ry = oval.size.height / 2;

	const auto x = rx * cosf(startAngle);
	const auto y = ry * sinf(startAngle);

	const auto sx = rx * cosf(startAngle + sweepAngle);
	const auto sy = ry * sinf(startAngle + sweepAngle);

	moveTo(oval.origin.x + rx + x, oval.origin.y + ry + y);
	arcTo(rx, ry, 0.0f, (sweepAngle > M_PI)?true:false, true, oval.origin.x + rx + sx, oval.origin.y + ry + sy);
	return *this;
}

Path & Path::addRect(float x, float y, float width, float height, float rx, float ry) {
	if (isnan(rx)) {
		rx = 0.0f;
	}
	if (isnan(ry)) {
		ry = 0.0f;
	}

	if (rx == 0.0f && ry == 0.0f) {
		return addRect(x, y, width, height);
	} else if (rx == 0.0f) {
		rx = ry;
	} else if (ry == 0.0f) {
		ry = rx;
	}

	rx = std::min(width / 2.0f, rx);
	ry = std::min(height / 2.0f, ry);

	moveTo(x + width - rx, y);
	arcTo(rx, ry, 0, false, false, x + width, y + ry);
	lineTo(x + width, y + height - ry);
	arcTo(rx, ry, 0, false, false, x + width - rx, y + height);
	lineTo(x + rx, y + height);
	arcTo(rx, ry, 0, false, false, x, y + height - ry);
	lineTo(x, y + ry);
	arcTo(rx, ry, 0, false, false, x + rx, y);
	closePath();

	return *this;
}
Path & Path::setFillColor(const Color4B &color) {
	_params.fillColor = color;
	return *this;
}
Path & Path::setFillColor(const Color3B &color, bool preserveOpacity) {
	_params.fillColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
Path & Path::setFillColor(const Color &color, bool preserveOpacity) {
	_params.fillColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
const Color4B &Path::getFillColor() const {
	return _params.fillColor;
}

Path & Path::setStrokeColor(const Color4B &color) {
	_params.strokeColor = color;
	return *this;
}
Path & Path::setStrokeColor(const Color3B &color, bool preserveOpacity) {
	_params.strokeColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
Path & Path::setStrokeColor(const Color &color, bool preserveOpacity) {
	_params.strokeColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
const Color4B &Path::getStrokeColor() const {
	return _params.strokeColor;
}

Path & Path::setFillOpacity(uint8_t value) {
	_params.fillColor.a = value;
	return *this;
}
uint8_t Path::getFillOpacity() const {
	return _params.fillColor.a;
}

Path & Path::setStrokeOpacity(uint8_t value) {
	_params.strokeColor.a = value;
	return *this;
}
uint8_t Path::getStrokeOpacity() const {
	return _params.strokeColor.a;
}

Path & Path::setStrokeWidth(float width) {
	_params.strokeWidth = width;
	return *this;
}

float Path::getStrokeWidth() const {
	return _params.strokeWidth;
}

Path &Path::setWindingRule(Winding value) {
	_params.winding = value;
	return *this;
}
Path::Winding Path::getWindingRule() const {
	return _params.winding;
}

Path &Path::setLineCup(LineCup value) {
	_params.lineCup = value;
	return *this;
}
Path::LineCup Path::getLineCup() const {
	return _params.lineCup;
}

Path &Path::setLineJoin(LineJoin value) {
	_params.lineJoin = value;
	return *this;
}
Path::LineJoin Path::getLineJoin() const {
	return _params.lineJoin;
}

Path &Path::setMiterLimit(float value) {
	_params.miterLimit = value;
	return *this;
}
float Path::getMiterLimit() const {
	return _params.miterLimit;
}

Path & Path::setStyle(Style s) {
	_params.style = s;
	return *this;
}

Path::Style Path::getStyle() const {
	return _params.style;
}

Path & Path::setTransform(const Mat4 &t) {
	_params.transform = t;
	return *this;
}
Path & Path::applyTransform(const Mat4 &t) {
	_params.transform *= t;
	return *this;
}
const Mat4 &Path::getTransform() const {
	return _params.transform;
}

Path & Path::clear() {
	if (!empty()) {
		_commands.clear();
	}
	return *this;
}

Path & Path::setTag(uint32_t tag) {
	_tag = tag;
	return *this;
}
uint32_t Path::getTag() const {
	return _tag;
}

Path & Path::setParams(const Params &p) {
	_params = p;
	return *this;
}

Path::Params Path::getParams() const {
	return _params;
}

bool Path::empty() const {
	return _commands.empty();
}

const Vector<Path::Command> &Path::getCommands() const {
	return _commands;
}

NS_LAYOUT_END
