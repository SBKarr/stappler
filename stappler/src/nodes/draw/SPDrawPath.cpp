/*
 * SPDrawPath.cpp
 *
 *  Created on: 10 мая 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDrawPath.h"
#include "SPDrawPathNode.h"
#include "SPFilesystem.h"

#include "cairo.h"

NS_SP_EXT_BEGIN(draw)

#define SP_PATH_LOG(...)
//#define SP_PATH_LOG(...) stappler::logTag("Path Debug", __VA_ARGS__)

class SVGPathReader : public ReaderClassBase<CharReaderBase> {
public:
	using Vec2 = cocos2d::Vec2;
	using Mat4 = cocos2d::Mat4;

	static bool readFile(Path *p, const std::string &str) {
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

	static bool readPath(Path *p, const std::string &str) {
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
			bool &_largeArc, bool &_sweep, float &_x, float &_y) {
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
		_x = x;
		_y = y;

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

bool Path::init() {
	return true;
}

bool Path::init(const std::string &str) {
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

void Path::moveTo(float x, float y) {
	_commands.emplace_back(Command::MakeMoveTo(x, y));
	setDirty();
}

void Path::lineTo(float x, float y) {
	_commands.emplace_back(Command::MakeLineTo(x, y));
	setDirty();
}

void Path::quadTo(float x1, float y1, float x2, float y2) {
	_commands.emplace_back(Command::MakeQuadTo(x1, y1, x2, y2));
	setDirty();
}

void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	_commands.emplace_back(Command::MakeCubicTo(x1, y1, x2, y2, x3, y3));
	setDirty();
}

void Path::arcTo(float rx, float ry, float angle, bool largeFlag, bool sweepFlag, float x, float y) {
	_commands.emplace_back(Command::MakeArcTo(rx, ry, angle, largeFlag, sweepFlag, x, y));
	setDirty();
}
void Path::arcTo(float xc, float yc, float rx, float ry, float angle1, float angle2, float rotation) {
	_commands.emplace_back(Command::MakeArcTo(xc, yc, rx, ry, angle1, angle2, rotation));
	setDirty();
}

void Path::beginPath() {
	_commands.emplace_back(Command::MakeBeginPath());
	setDirty();
}

void Path::closePath() {
	_commands.emplace_back(Command::MakeClosePath());
	setDirty();
}

void Path::addRect(const cocos2d::Rect& rect) {
	moveTo(rect.origin.x, rect.origin.y);
	lineTo(rect.origin.x + rect.size.width, rect.origin.y);
	lineTo(rect.origin.x + rect.size.width, rect.origin.y + rect.size.height);
	lineTo(rect.origin.x, rect.origin.y + rect.size.height);
	closePath();
}
void Path::addOval(const cocos2d::Rect& oval) {
	moveTo(oval.getMaxX(), oval.getMidY());
	arcTo(oval.getMidX(), oval.getMidY(), oval.size.width/2, oval.size.height/2, 0.0f, 360.0_to_rad, 0.0f);
	closePath();
}
void Path::addCircle(float x, float y, float radius) {
	moveTo(x + radius, y);
	arcTo(x, y, radius, 0.0f, 360.0f * (M_PI/180.0f));
	closePath();
}
void Path::addArc(const cocos2d::Rect& oval, float startAngle, float sweepAngle) {
	beginPath();
	arcTo(oval.getMidX(), oval.getMidY(), oval.size.width/2, oval.size.height/2, startAngle, sweepAngle, 0.0f);
}
/*
void Path::addRoundRect(const cocos2d::Rect& rect, float rx, float ry) {

}
void Path::addPoly(const cocos2d::Vec2 pts[], int count, bool close) {

}
*/
void Path::setFillColor(const cocos2d::Color4B &color) {
	_fillColor = color;
	setDirty();
}
const cocos2d::Color4B &Path::getFillColor() const {
	return _fillColor;
}

void Path::setStrokeColor(const cocos2d::Color4B &color) {
	_strokeColor = color;
	setDirty();
}
const cocos2d::Color4B &Path::getStrokeColor() const {
	return _strokeColor;
}

void Path::setFillOpacity(uint8_t value) {
	_fillColor.a = value;
	setDirty();
}
uint8_t Path::getFillOpacity() const {
	return _fillColor.a;
}

void Path::setStrokeOpacity(uint8_t value) {
	_strokeColor.a = value;
	setDirty();
}
uint8_t Path::getStrokeOpacity() const {
	return _strokeColor.a;
}

void Path::setAntialiased(bool value) {
	if (_isAntialiased != value) {
		_isAntialiased = value;
		setDirty();
	}
}
bool Path::isAntialiased() const {
	return _isAntialiased;
}

void Path::setStrokeWidth(float width) {
	if (width != _strokeWidth) {
		_strokeWidth = width;
		setDirty();
	}
}

float Path::getStrokeWidth() const {
	return _strokeWidth;
}

void Path::setStyle(Style s) {
	if (s != _style) {
		_style = s;
		setDirty();
	}
}

Path::Style Path::getStyle() const {
	return _style;
}

void Path::clear() {
	if (!empty()) {
		_commands.clear();
		setDirty();
	}
}

bool Path::empty() const {
	return _commands.empty();
}

void helper_quadratic_to (cairo_t *cr, float x1, float y1, float x2, float y2) {
	double x0, y0;
	cairo_get_current_point (cr, &x0, &y0);
	cairo_curve_to (cr,
		2.0 / 3.0 * x1 + 1.0 / 3.0 * x0,
		2.0 / 3.0 * y1 + 1.0 / 3.0 * y0,
		2.0 / 3.0 * x1 + 1.0 / 3.0 * x2,
		2.0 / 3.0 * y1 + 1.0 / 3.0 * y2,
		x2, y2);
}

void helper_arc_to (cairo_t *cr, float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation) {
	cairo_matrix_t save_matrix;
	cairo_get_matrix(cr, &save_matrix);
	cairo_translate(cr, xc, yc);
	if (rx > ry) {
		cairo_scale(cr, ry / rx, 1.0);
	} else if (rx < ry) {
		cairo_scale(cr, 1.0, rx / ry);
	}
	if (rotation != 0.0f) {
		cairo_rotate(cr, rotation);
	}
	if (sweepAngle >= 0.0f) {
		cairo_arc(cr, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle + sweepAngle);
	} else {
		cairo_arc_negative(cr, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle + sweepAngle);
	}
	cairo_set_matrix(cr, &save_matrix);
}

void helper_alt_arc_to (cairo_t *cr, float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y) {
	double _x, _y;
	cairo_get_current_point (cr, &_x, &_y);

	rx = fabsf(rx); ry = fabsf(ry);

	cocos2d::Mat4 transform(cocos2d::Mat4::IDENTITY); Mat4::createRotationZ(angle, &transform);
	cocos2d::Mat4 inverse = transform; inverse.transpose();

	Vec2 vDash(cocos2d::PointApplyTransform(Vec2((_x - x) / 2, (_y - y) / 2), inverse));
	float lambda = (vDash.x * vDash.x) / (rx * rx) + (vDash.y * vDash.y) / (ry * ry);
	if (lambda > 1.0f) {
		rx = sqrtf(lambda) * rx; ry = sqrtf(lambda) * ry;
		vDash = Vec2(cocos2d::PointApplyTransform(Vec2((_x - x) / 2, (_y - y) / 2), inverse));
	}

	float rx_y1_ = (rx * rx * vDash.y * vDash.y);
	float ry_x1_ = (ry * ry * vDash.x * vDash.x);
	float cst = sqrtf(((rx * rx * ry * ry) - rx_y1_ - ry_x1_) / (rx_y1_ + ry_x1_));

	Vec2 cDash((largeArc != sweep ? 1.0f : -1.0f) * cst * rx * vDash.y / ry,
			(largeArc != sweep ? 1.0f : -1.0f) * - cst * ry * vDash.x / rx);

	float cx = cDash.x + (_x + x) / 2;
	float cy = cDash.y + (_y + y) / 2;

	float startAngle = Vec2::angle(Vec2(1.0f, 0.0f), Vec2((vDash.x - cDash.x) / rx, (vDash.y - cDash.y) / ry));
	float sweepAngle = Vec2::angle(Vec2((vDash.x - cDash.x) / rx, (vDash.y - cDash.y) / ry),
			Vec2((-vDash.x - cDash.x) / rx, (-vDash.y - cDash.y) / ry));

	helper_arc_to(cr, cx, cy, rx, ry, startAngle, sweepAngle, angle);
}

void Path::drawOn(cairo_t *ctx) const {
	cairo_save(ctx);
	cairo_set_antialias(ctx, (_isAntialiased?CAIRO_ANTIALIAS_GOOD:CAIRO_ANTIALIAS_NONE));
	cairo_set_line_width(ctx, _strokeWidth);

	for (auto &it : _commands) {
		switch (it.type) {
		case Command::MoveTo:
			cairo_move_to(ctx, it.moveTo.x, it.moveTo.y);
			break;
		case Command::LineTo:
			cairo_line_to(ctx, it.lineTo.x, it.lineTo.y);
			break;
		case Command::QuadTo:
			helper_quadratic_to(ctx, it.quadTo.x1, it.quadTo.y1, it.quadTo.x2, it.quadTo.y2);
			break;
		case Command::CubicTo:
			cairo_curve_to(ctx, it.cubicTo.x1, it.cubicTo.y1, it.cubicTo.x2, it.cubicTo.y2, it.cubicTo.x3, it.cubicTo.y3);
			break;
		case Command::ArcTo:
			helper_arc_to(ctx, it.arcTo.xc, it.arcTo.yc, it.arcTo.rx, it.arcTo.ry, it.arcTo.startAngle, it.arcTo.sweepAngle, it.arcTo.rotation);
			break;
		case Command::AltArcTo:
			helper_alt_arc_to(ctx, it.altArcTo.rx, it.altArcTo.ry, it.altArcTo.rotation, it.altArcTo.largeFlag, it.altArcTo.sweepFlag, it.altArcTo.x, it.altArcTo.y);
			break;
		case Command::ClosePath:
			cairo_close_path(ctx);
			break;
		case Command::BeginPath:
			cairo_new_sub_path(ctx);
			break;
		default: break;
		}
	}

	switch (_style) {
	case Style::Fill:
		cairo_set_source_rgba(ctx, _fillColor.r / 255.0, _fillColor.g / 255.0, _fillColor.b / 255.0, _fillColor.a / 255.0);
		cairo_fill(ctx);
		break;
	case Style::Stroke:
		cairo_set_source_rgba(ctx, _strokeColor.r / 255.0, _strokeColor.g / 255.0, _strokeColor.b / 255.0, _strokeColor.a / 255.0);
		cairo_stroke(ctx);
		break;
	case Style::FillAndStroke:
		cairo_set_source_rgba(ctx, _fillColor.r / 255.0, _fillColor.g / 255.0, _fillColor.b / 255.0, _fillColor.a / 255.0);
		cairo_fill_preserve(ctx);
		cairo_set_source_rgba(ctx, _strokeColor.r / 255.0, _strokeColor.g / 255.0, _strokeColor.b / 255.0, _strokeColor.a / 255.0);
		cairo_stroke(ctx);
		break;
	}

	cairo_restore(ctx);
}

void Path::remove() {
	auto nodes = _nodes;
	for (auto node : nodes) {
		node->removePath(this);
	}
}

void Path::addNode(PathNode *node) {
	_nodes.insert(node);
	node->_pathsDirty = true;
}

void Path::removeNode(PathNode *node) {
	auto it = _nodes.find(node);
	if (it != _nodes.end()) {
		_nodes.erase(node);
		node->_pathsDirty = true;
	}
}

void Path::setDirty() {
	if (!_dirty) {
		for (auto node : _nodes) {
			node->_pathsDirty = true;
		}
		_dirty = true;
	}
}

NS_SP_EXT_END(draw)
