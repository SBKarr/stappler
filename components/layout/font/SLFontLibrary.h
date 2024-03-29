/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLFontSource.h"

#ifndef SP_DISABLE_FONT_LAYOUT

typedef struct FT_FaceRec_ * FT_Face;
typedef struct FT_LibraryRec_ * FT_Library;

NS_LAYOUT_BEGIN

struct FontStruct {
	String file;
	Bytes data;
	BytesView view;

	FontStruct(const String &f, Bytes &&d) : file(f), data(move(d)), view(data) { }
	FontStruct(const BytesView &d) : file(toString("bytes://", (void *)d.data())), view(d) { }
};

using FontStructMap = Map<String, FontStruct>;
using FontFaceMap = Map<String, FT_Face>;
using FontFacePriority = Vector<Pair<String, uint16_t>>;

struct FontTextureLayout final : Ref {
	bool init(uint32_t, FontTextureMap &&);

	uint32_t index;
	FontTextureMap map;
};

class FreeTypeInterface : public Ref {
public: /* common functions */
	using FontTextureInterface = layout::FontTextureInterface;

	virtual bool init(const String &);
	virtual bool init(const BytesView &);
	virtual ~FreeTypeInterface();

	/**
	 * Drop all cached data
	 */
	void clear();

public: /* fonts interface */

	/**
	 * Request metrics data for specific font size and sources
	 */
	Metrics requestMetrics(const FontSource *, const Vector<FontFace::FontFaceSource> &, FontSize size, const ReceiptCallback &cb);

	/**
	 * Reqeust data for new chars in layout
	 */
	Rc<FontData> requestLayoutUpgrade(const FontSource *, const Vector<FontFace::FontFaceSource> &,
			const Rc<FontData> &data, const Vector<char16_t> &chars, const ReceiptCallback &cb);

	FontTextureMap updateTextureWithSource(uint32_t v, FontSource *source, const Map<String, Vector<char16_t>> &l, const FontTextureInterface &);

protected:

	/**
	 * Open file by name and read callback
	 * Returns iterator, that points to cached file data
	 */
	FontStructMap::iterator openFile(const FontSource *source, const ReceiptCallback &cb, const String &name, const String &file);

	/**
	 * Open file by name and read callback
	 * Returns iterator, that points to cached file data
	 */
	FontStructMap::iterator openFile(const FontSource *source, const String &name, const BytesView &file);

	/**
	 * Open Freetype face from data in memory
	 * Returns iterator, that points to cached file data
	 */
	FT_Face openFontFace(const BytesView &data, const String &font, FontSize fontSize);

	/**
	 * Open FreeType face for specific filename and font size
	 */
	FT_Face getFace(const FontSource *source, const ReceiptCallback &cb, const FontFace::FontFaceSource &file, FontSize size);

	/**
	 * Update layout data for specific font and char
	 * Also, updates provided FreeType face cache
	 */
	void requestCharUpdate(const FontSource *source, const ReceiptCallback &cb, const Vector<FontFace::FontFaceSource> &srcs, FontSize size,
			Vector<FT_Face> &faces, Vector<CharLayout> &layout, char16_t theChar);

	/**
	 * Get kerning amount for two chars
	 */
	bool getKerning(const Vector<FT_Face> &faces, char16_t first, char16_t second, int16_t &value);

protected:
	FontFace::FontFaceSource fallbackFont;
	Set<String> files;
	FontStructMap openedFiles;
	FontFaceMap openedFaces;
	FT_Library FTlibrary = nullptr;
};

NS_LAYOUT_END

#endif

#endif /* LAYOUT_FONT_SLFONTLIBRARY_H_ */
