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

#ifndef LAYOUT_FONT_SLFONTLIBRARY_H_
#define LAYOUT_FONT_SLFONTLIBRARY_H_

#include "SLFont.h"

typedef struct FT_FaceRec_ * FT_Face;
typedef struct FT_LibraryRec_ * FT_Library;

NS_LAYOUT_BEGIN

struct FontStruct {
	String file;
	Bytes data;
};

using FontStructMap = Map<String, FontStruct>;
using FontFaceMap = Map<String, FT_Face>;
using FontFacePriority = Vector<Pair<String, uint16_t>>;
using FontTextureMap = Map<String, Vector<CharTexture>>;
using FontTextureLayout = Pair<uint32_t, FontTextureMap>;

class FreeTypeInterface : public AllocBase {
public: /* common functions */

	FreeTypeInterface(const String &);
	~FreeTypeInterface();

	/**
	 * Drop all cached data
	 */
	void clear();

public: /* fonts interface */

	/**
	 * Request metrics data for specific font size and sources
	 */
	Metrics requestMetrics(const FontSource *, const Vector<String> &, uint16_t size, const ReceiptCallback &cb);

	/**
	 * Reqeust data for new chars in layout
	 */
	Arc<FontLayout::Data> requestLayoutUpgrade(const FontSource *, const Vector<String> &,
			const Arc<FontLayout::Data> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb);


	struct FontTextureInterface {
		Function<size_t(uint16_t, uint16_t)> emplaceTexture;
		Function<bool(size_t, const void *data, uint16_t offsetX, uint16_t offsetY, uint16_t width, uint16_t height)> draw;
	};

	FontTextureMap updateTextureWithSource(uint32_t v, FontSource *source, const Map<String, Vector<char16_t>> &l, const FontTextureInterface &);

protected:

	/**
	 * Open file by name and read callback
	 * Returns iterator, that points to cached file data
	 */
	FontStructMap::iterator openFile(const FontSource *source, const ReceiptCallback &cb, const String &name, const String &file);

	/**
	 * Open Freetype face from data in memory
	 * Returns iterator, that points to cached file data
	 */
	FT_Face openFontFace(const Bytes &data, const String &font, uint16_t fontSize);

	/**
	 * Open FreeType face for specific filename and font size
	 */
	FT_Face getFace(const FontSource *source, const ReceiptCallback &cb, const String &file, uint16_t size);

	/**
	 * Update layout data for specific font and char
	 * Also, updates provided FreeType face cache
	 */
	void requestCharUpdate(const FontSource *source, const ReceiptCallback &cb, const Vector<String> &srcs, uint16_t size,
			Vector<FT_Face> &faces, Vector<CharLayout> &layout, char16_t theChar);

	/**
	 * Get kerning amount for two chars
	 */
	bool getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value);

protected:
	String fallbackFont;
	Set<String> files;
	FontStructMap openedFiles;
	FontFaceMap openedFaces;
	FT_Library FTlibrary = nullptr;
};

/*class FontLibrary;
class FontLibraryCache {
public:
	FontLibraryCache(FontLibrary *);
	~FontLibraryCache();

	void clear();
	void update();
	Time getTimer() const;

protected:
	FT_Face openFontFace(const Bytes &data, const String &font, uint16_t fontSize);
	FontStructMap::iterator processFile(Bytes && data, const String &file);
	FontStructMap::iterator openFile(const Source *source, const ReceiptCallback &cb, const String &name, const String &file);
	FT_Face getFace(const Source *source, const ReceiptCallback &cb, const String &file, uint16_t size);

	Metrics getMetrics(FT_Face face, uint16_t);

	void requestCharUpdate(const Source *, const ReceiptCallback &cb, const Vector<String> &, uint16_t size, Vector<FT_Face> &, Vector<CharLayout> &, char16_t);
	bool getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value);

	void drawCharOnBitmap(cocos2d::Texture2D *bmp, const URect &, FT_Bitmap *bitmap);

	FontLibrary *library;
	Time timer;
};

using FontTextureLayout = Pair<uint32_t, Map<String, Vector<CharTexture>>>;

struct FontLayoutData {
	Arc<FontLayout> layout;
	Vector<CharTexture> *chars;
	Vector<FT_Face> faces;
};

class FontLibrary {
public:
	static FontLibrary *getInstance();
	FontLibrary();
	~FontLibrary();
	void update(float dt);

	Arc<FontLibraryCache> getCache();

	Vector<size_t> getQuadsCount(const FormatSpec *format, const Map<String, Vector<CharTexture>> &layouts, size_t texSize);

	bool writeTextureQuads(uint32_t v, FontSource *, const FormatSpec *, const Vector<Rc<cocos2d::Texture2D>> &,
			Vector<Rc<DynamicQuadArray>> &, Vector<Vector<bool>> &);

	bool writeTextureRects(uint32_t v, FontSource *, const FormatSpec *, float scale, Vector<Rect> &);

	bool isSourceRequestValid(FontSource *, uint32_t);

	void cleanupSource(FontSource *);
	Arc<FontTextureLayout> getSourceLayout(FontSource *);
	void setSourceLayout(FontSource *, FontTextureLayout &&);

protected:
	void writeTextureQuad(const FormatSpec *format, const font::Metrics &m, const font::CharSpec &c, const font::CharLayout &l, const font::CharTexture &t,
			const font::RangeSpec &range, const font::LineSpec &line, Vector<bool> &cMap, const cocos2d::Texture2D *tex, DynamicQuadArray *quad);

	std::mutex _mutex;
	Time _timer = 0;
	Map<uint64_t, Arc<FontLibraryCache>> _cache;

	std::mutex _sourcesMutex;
	Map<Source *, Arc<FontTextureLayout>> _sources;

	static FontLibrary *s_instance;
	static std::mutex s_mutex;
};
*/

NS_LAYOUT_END

#endif /* LAYOUT_FONT_SLFONTLIBRARY_H_ */
