/*
 * SPFontFIle.cpp
 *
 *  Created on: 10 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPResource.h"
#include "SPFilesystem.h"

//#define SP_RESOURCE_FILE_LOG(...) logTag("FontFile", __VA_ARGS__)
#define SP_RESOURCE_FILE_LOG(...)

NS_SP_EXT_BEGIN(resource)

static std::string s_cachePath;

static std::string getFontCachePath(const std::string &name) {
	if (s_cachePath.empty()) {
		s_cachePath = filesystem::writablePath("fonts_cache");
		filesystem::mkdir(s_cachePath);
	}

	return s_cachePath + "/font." + name + ".sbf";
}

static uint8_t getEngineVersion() {
	return 2;
}

struct FontFileData {
	Font::Metrics metrics;
	Font::CharSet chars;
	Font::KerningMap kerning;
};

struct FontFileInfo {
	std::string imageFile;

	std::map<uint16_t, std::string> fontIds;
	std::map<std::string, FontFileData> fontData;
};

struct FontFileMainHeader {
	uint8_t code[3];
	uint8_t engineVersion;
	uint16_t headerSize;
};

struct FontFileSubHeader {
	uint16_t fileVersion;
	uint16_t fontsCount;
	uint16_t imageNameSize;
	char name[0];
};

struct FontFileFontHeader {
	uint16_t id;
	uint64_t receiptHash;
	uint16_t nameSize;
	char name[0];
};

struct FontFileMetrics {
	uint16_t id;
	uint16_t fontSize;
	uint16_t fontHeight;
	int16_t ascender;
	int16_t descender;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint16_t charsSize;
	uint16_t kerningSize;
};

struct FontFileChar {
	uint16_t id;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	int16_t xOffset;
	int16_t yOffset;
	uint16_t xAdvance;
};

struct FontFileKerning {
	uint32_t chars;
	int16_t value;
};

uint16_t readMainHeader(FontFileInfo &info, std::ifstream &stream) {
	FontFileMainHeader header;
	stream.read((char *)&header, sizeof(FontFileMainHeader));

	if ((stream.rdstate() & std::ifstream::eofbit ) != 0
			|| memcmp(header.code, "SBF", 3) != 0
			|| header.engineVersion != getEngineVersion()) {
		return 0;
	}

	SP_RESOURCE_FILE_LOG("read main header: code:%c%c%c engine:%d size:%d", header.code[0], header.code[1], header.code[2],
			header.engineVersion, header.headerSize);

	return header.headerSize;
}

void writeMainHeader(std::ofstream &stream, uint16_t headerSize) {
	FontFileMainHeader header;
	header.code[0] = 'S'; header.code[1] = 'B'; header.code[2] = 'F'; header.engineVersion = getEngineVersion(); header.headerSize = headerSize;
	stream.write((const char *)&header, sizeof(FontFileMainHeader));
	SP_RESOURCE_FILE_LOG("write main header: code:%c%c%c engine:%d size:%d", header.code[0], header.code[1], header.code[2],
			header.engineVersion, header.headerSize);
}

uint16_t readSubHeader(FontFileInfo &info, std::ifstream &stream, uint16_t headerSize, size_t fontsCount, uint16_t version) {
	FontFileSubHeader *header = (FontFileSubHeader *)malloc(headerSize + 1); memset(header, 0, headerSize + 1);
	stream.read((char *)header, headerSize);

	if ((stream.rdstate() & std::ifstream::eofbit ) != 0
			|| header->fontsCount != fontsCount || header->fileVersion != version) {
		return 0;
	}

	info.imageFile = std::string(header->name, header->imageNameSize);
	SP_RESOURCE_FILE_LOG("read sub header: version:%d count:%d imageFile:%s %d %d", header->fileVersion, header->fontsCount,
			info.imageFile.c_str(), *(((uint8_t *)header) + headerSize - 1), *(((uint8_t *)header) + headerSize));

	if (info.imageFile.empty()) {
		return 0;
	}

	auto fc = header->fontsCount;
	free(header);
	return fc;
}

void writeSubHeader(std::ofstream &stream, uint16_t version, uint16_t fontsCount, const std::string &image) {
	FontFileSubHeader header;
	header.fileVersion = version;
	header.fontsCount = fontsCount;
	header.imageNameSize = image.size();

	stream.write((const char *)&header, sizeof(FontFileSubHeader));
	stream.write(image.c_str(), image.size());

	SP_RESOURCE_FILE_LOG("write sub header: version:%d count:%d imageFile:%s", header.fileVersion, header.fontsCount,
			image.c_str());
}

std::string readFontHeader(FontFileInfo &info, std::ifstream &stream, FontFileFontHeader &header) {
	uint16_t headerSize = 0;
	stream.read((char *)(&headerSize), sizeof(uint16_t));
	if ((stream.rdstate() & std::ifstream::eofbit ) != 0 || headerSize == 0) {
		return "";
	}

	uint8_t *buf = new uint8_t[headerSize];
	stream.read((char *)buf, headerSize);
	if ((stream.rdstate() & std::ifstream::eofbit ) != 0 || ((FontFileFontHeader *)buf)->nameSize == 0) {
		return "";
	}

	memcpy(&header, buf, sizeof(FontFileFontHeader));

	std::string name((char *)(buf + sizeof(FontFileFontHeader)), header.nameSize);
	SP_RESOURCE_FILE_LOG("read font header: headerSize:%d id:%d hash:%lu name:%s", headerSize, header.id,
			header.receiptHash, name.c_str());
	info.fontIds.insert(std::make_pair(header.id, name));
	delete [] buf;
	return name;
}

void writeFontHeader(std::ofstream &stream, uint16_t &id, uint64_t receiptHash, const std::string &name) {
	uint16_t size = sizeof(FontFileFontHeader) + name.size();
	stream.write((const char *)&size, sizeof(uint16_t));

	FontFileFontHeader header;
	header.id = id;
	header.receiptHash = receiptHash;
	header.nameSize = name.size();

	stream.write((const char *)&header, sizeof(FontFileFontHeader));
	stream.write(name.c_str(), name.size());

	SP_RESOURCE_FILE_LOG("write font header: headerSize:%d id:%d hash:%lu name:%s", size, id,
			receiptHash, name.c_str());

	id ++;
}

bool readFonts(FontFileInfo &info, std::ifstream &stream, uint16_t count) {
	FontFileMetrics metrics;
	FontFileChar fchar;
	FontFileKerning kerning;
	FontFileData data;
	for (uint16_t i = 0; i < count; i++) {
		data.chars.clear();
		data.kerning.clear();

		stream.read((char *)&metrics, sizeof(FontFileMetrics));
		if ((stream.rdstate() & std::ifstream::eofbit ) != 0) {
			return false;
		}

		auto nameIt = info.fontIds.find(metrics.id);
		if (nameIt == info.fontIds.end()) {
			return false;
		}

		auto &name = nameIt->second;

		data.metrics = Font::Metrics{ metrics.fontSize, metrics.fontHeight, metrics.ascender, metrics.descender,
			metrics.underlinePosition, metrics.underlineThickness };

		for (uint16_t j = 0; j < metrics.charsSize; j++) {
			stream.read((char *)&fchar, sizeof(FontFileChar));
			if ((stream.rdstate() & std::ifstream::eofbit ) != 0) {
				return false;
			}

			data.chars.insert(Font::Char(fchar.id, cocos2d::Rect(fchar.x, fchar.y, fchar.w, fchar.h),
					fchar.xOffset, fchar.yOffset, fchar.xAdvance));
		}

		for (uint16_t j = 0; j < metrics.kerningSize; j++) {
			stream.read((char *)&kerning, sizeof(FontFileKerning));
			if ((stream.rdstate() & std::ifstream::eofbit ) != 0) {
				return false;
			}

			data.kerning.insert(std::make_pair(kerning.chars, kerning.value));
		}

		SP_RESOURCE_FILE_LOG("read font: id:%d size:%d height:%d asc:%d desc:%d upos:%d ut:%d chars:%d kernpairs:%d",
				metrics.id, metrics.fontSize, metrics.fontHeight, metrics.ascender, metrics.descender,
				metrics.underlinePosition, metrics.underlineThickness, metrics.charsSize, metrics.kerningSize);

		info.fontData.insert(std::make_pair(name, std::move(data)));
	}

	return true;
}

void writeFont(std::ofstream &stream, uint16_t id, const Font::Data &data) {
	FontFileMetrics metrics;
	FontFileChar fchar;
	FontFileKerning kerning;

	metrics.id = id;
	metrics.fontSize = data.metrics.size;
	metrics.fontHeight = data.metrics.height;
	metrics.ascender = data.metrics.ascender;
	metrics.descender = data.metrics.descender;
	metrics.underlinePosition = data.metrics.underlinePosition;
	metrics.underlineThickness = data.metrics.underlineThickness;
	metrics.charsSize = data.chars.size();
	metrics.kerningSize = data.kerning.size();

	stream.write((const char *)&metrics, sizeof(FontFileMetrics));

	for (auto &it : data.chars) {
		fchar.id = it.charID;
		fchar.x = it.x();
		fchar.y = it.y();
		fchar.w = it.width();
		fchar.h = it.height();
		fchar.xOffset = it.xOffset;
		fchar.yOffset = it.yOffset;
		fchar.xAdvance = it.xAdvance;

		stream.write((const char *)&fchar, sizeof(FontFileChar));
	}

	for (auto &it : data.kerning) {
		kerning.chars = it.first;
		kerning.value = it.second;

		stream.write((const char *)&kerning, sizeof(FontFileKerning));
	}

	SP_RESOURCE_FILE_LOG("write font: id:%d size:%d height:%d asc:%d desc:%d upos:%d ut:%d chars:%d kernpairs:%d",
			metrics.id, metrics.fontSize, metrics.fontHeight, metrics.ascender, metrics.descender,
			metrics.underlinePosition, metrics.underlineThickness, metrics.charsSize, metrics.kerningSize);
}

FontSet * readFontSet(FontSet::Config &&cfg, const cocos2d::Map<std::string, Asset *> &assets) {
	auto path = getFontCachePath(cfg.name);

	std::ifstream stream(path, std::ios::in | std::ios::binary);
	if (!stream.is_open()) {
		return nullptr;
	}

	FontFileInfo info;

	auto subHeadSize = readMainHeader(info, stream);
	if (subHeadSize == 0) {
		stream.close();
		return nullptr;
	}

	auto fontsCount = readSubHeader(info, stream, subHeadSize, cfg.requests.size(), cfg.version);
	if (fontsCount == 0) {
		stream.close();
		return nullptr;
	}

	if (!filesystem::exists(info.imageFile)) {
		stream.close();
		return nullptr;
	}

	std::map<std::string, uint64_t> hashes;
	for (auto &it : cfg.requests) {
		auto hash = getReceiptHash(it.receipt, assets);
		SP_RESOURCE_FILE_LOG("hash: %s -> %lu", it.name.c_str(), hash);
		hashes.insert(std::make_pair(it.name, hash));
	}

	FontFileFontHeader fontHeader;
	for (uint16_t i = 0; i < fontsCount; i++) {
		auto name = readFontHeader(info, stream, fontHeader);
		if (name.empty()) {
			stream.close();
			return nullptr;
		} else {
			auto hashIt = hashes.find(name);
			if (hashIt == hashes.end() || hashIt->second != fontHeader.receiptHash) {
				stream.close();
				return nullptr;
			}
		}
	}

	if (!readFonts(info, stream, fontsCount)) {
		stream.close();
		return nullptr;
	}

	stream.close();

	auto image = Image::createWithFile(info.imageFile, Image::Format::A8);
	if (!image) {
		return nullptr;
	}

	cocos2d::Map<std::string, Font *> fontMap;
	for (auto &it : info.fontData) {
		auto font = new Font(it.first, std::move(it.second.metrics), std::move(it.second.chars), std::move(it.second.kerning), image);
		fontMap.insert(it.first, font);
		font->release();
	}

	return new FontSet(std::move(cfg), std::move(fontMap), image);
}

FontSet * writeFontSet(FontSet::Config &&cfg, const cocos2d::Map<std::string, Asset *> &assets,
		std::map<std::string, Font::Data> &fonts, const uint8_t *imageData, uint32_t imageWidth, uint32_t imageHeight) {
	auto path = getFontCachePath(cfg.name);
	auto imagePath = path + ".png";

	filesystem::remove(path);
	filesystem::remove(imagePath);

	Image::savePng(imagePath, imageData, imageWidth, imageHeight, Image::Format::A8);

	std::ofstream stream(path);
	if (!stream.is_open()) {
		return nullptr;
	}

	std::string imageCanonical = filepath::canonical(imagePath);
	uint16_t subheadSize = sizeof(FontFileSubHeader) + imageCanonical.size();
	writeMainHeader(stream, subheadSize);
	writeSubHeader(stream, cfg.version, fonts.size(), imageCanonical);

	// fonts headers
	uint16_t id = 0;
	std::map<std::string, uint16_t> fontIds;
	for (auto &it : cfg.requests) {
		auto fontIt = fonts.find(it.name);
		if (fontIt != fonts.end()) {
			fontIds.insert(std::make_pair(it.name, id));
			writeFontHeader(stream, id, getReceiptHash(it.receipt, assets), it.name);
		}
	}

	// fonts data
	for (auto &it : fonts) {
		auto idIt = fontIds.find(it.first);
		if (idIt != fontIds.end()) {
			writeFont(stream, idIt->second, it.second);
		}
	}

	auto image = Image::createWithFile(imagePath, Image::Format::A8);
	if (!image) {
		return nullptr;
	}

	cocos2d::Map<std::string, Font *> fontMap;
	for (auto &it : fonts) {
		auto font = new Font(it.first, std::move(it.second.metrics), std::move(it.second.chars), std::move(it.second.kerning), image);
		fontMap.insert(it.first, font);
		font->release();
	}

	return new FontSet(std::move(cfg), std::move(fontMap), image);
}

NS_SP_EXT_END(resource)
