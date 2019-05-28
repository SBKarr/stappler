/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDBStorageFile.h"

#include "SPFilesystem.h"
#include "SPBitmap.h"
#include "SPDBStorageField.h"
#include "SPDBStorageScheme.h"
#include "SPDBStorageAdapter.h"
#include "SPDBStorageTransaction.h"
#include "SPDBStorageWorker.h"
#include "SPDBInputFile.h"

#include "SPIO.h"

NS_DB_BEGIN

mem::String File::getFilesystemPath(uint64_t oid) {
	return mem::toString(internals::getDocuemntRoot(), "/uploads/", oid);
}

static bool File_isImage(const mem::StringView &type) {
	return type == "image/gif"
			|| type == "image/jpeg"
			|| type == "image/pjpeg"
			|| type == "image/png"
			|| type == "image/tiff"
			|| type == "image/webp"
			|| type == "image/svg+xml";
}

static bool File_validateFileField(const Field &field, size_t writeSize, const mem::StringView &type) {
	auto ffield = static_cast<const FieldFile *>(field.getSlot());
	// check size
	if (writeSize > ffield->maxSize) {
		messages::error("Storage", "File is larger then max file size in field", mem::Value {
			std::make_pair("field", mem::Value(field.getName())),
			std::make_pair("max", mem::Value((int64_t)ffield->maxSize)),
			std::make_pair("size", mem::Value((int64_t)writeSize))
		});
		return false;
	}

	// check type
	auto &types = ffield->allowedTypes;
	if (!types.empty()) {
		bool ret = false;
		for (auto &it : types) {
			if (type == it) {
				ret = true;
				break;
			}
		}
		if (!ret) {
			messages::error("Storage", "Invalid file type for field", mem::Value {
				std::make_pair("field", mem::Value(field.getName())),
				std::make_pair("type", mem::Value(type))
			});
			return false;
		}
	}
	return true;
}

static bool File_validateImageField(const Field &field, size_t writeSize, mem::StringView type, stappler::io::Producer file) {
	auto ffield = static_cast<const FieldImage *>(field.getSlot());

	// check size
	if (writeSize > ffield->maxSize) {
		messages::error("Storage", "File is larger then max file size in field", mem::Value{
			std::make_pair("field", mem::Value(field.getName())),
			std::make_pair("max", mem::Value((int64_t)ffield->maxSize)),
			std::make_pair("size", mem::Value((int64_t)writeSize))
		});
		return false;
	}

	if (!File_isImage(type)) {
		messages::error("Storage", "Unknown image type for field", mem::Value {
			std::make_pair("field", mem::Value(field.getName())),
			std::make_pair("type", mem::Value(type))
		});
		return false;
	}

	// check type
	auto &types = ffield->allowedTypes;
	if (!types.empty()) {
		bool ret = false;
		for (auto &it : types) {
			if (type == it) {
				ret = true;
				break;
			}
		}
		if (!ret) {
			messages::error("Storage", "Invalid file type for field", mem::Value{
				std::make_pair("field", mem::Value(field.getName())),
				std::make_pair("type", mem::Value(type))
			});
			return false;
		}
	}

	size_t width = 0, height = 0;
	if (!stappler::Bitmap::getImageSize(file, width, height) && width > 0 && height > 0) {
		messages::error("Storage", "Fail to detect file size with");
		return false;
	}

	if (ffield->minImageSize.policy == ImagePolicy::Reject) {
		if (ffield->minImageSize.width > width || ffield->minImageSize.height > height) {
			messages::error("Storage", "Image is to small, rejected by policy rule", mem::Value{
				std::make_pair("min", mem::Value {
					std::make_pair("width", mem::Value(ffield->minImageSize.width)),
					std::make_pair("height", mem::Value(ffield->minImageSize.height))
				}),
				std::make_pair("current", mem::Value{
					std::make_pair("width", mem::Value(width)),
					std::make_pair("height", mem::Value(height))
				})
			});
			return false;
		}
	}

	if (ffield->maxImageSize.policy == ImagePolicy::Reject) {
		if (ffield->maxImageSize.width < width || ffield->maxImageSize.height < height) {
			messages::error("Storage", "Image is to large, rejected by policy rule", mem::Value{
				std::make_pair("max", mem::Value {
					std::make_pair("width", mem::Value(ffield->maxImageSize.width)),
					std::make_pair("height", mem::Value(ffield->maxImageSize.height))
				}),
				std::make_pair("current", mem::Value{
					std::make_pair("width", mem::Value(width)),
					std::make_pair("height", mem::Value(height))
				})
			});
			return false;
		}
	}
	return true;
}

bool File::validateFileField(const Field &field, const InputFile &file) {
	if (field.getType() == db::Type::File) {
		return File_validateFileField(field, file.writeSize, file.type);
	} else if (field.getType() == db::Type::Image) {
		return File_validateImageField(field, file.writeSize, file.type, file.file);
	}
	return true;
}

bool File::validateFileField(const Field &field, const mem::StringView &type, const mem::Bytes &data) {
	if (field.getType() == db::Type::File) {
		return File_validateFileField(field, data.size(), type);
	} else if (field.getType() == db::Type::Image) {
		stappler::CoderSource source(data);
		return File_validateImageField(field, data.size(), type, source);
	}
	return true;
}

mem::Value File::createFile(const Transaction &t, const Field &f, InputFile &file) {
	auto scheme = internals::getFileScheme();
	Value fileData;
	fileData.setString(file.type, "type");
	fileData.setInteger(file.writeSize, "size");

	if (f.getType() == db::Type::Image || File_isImage(file.type)) {
		size_t width = 0, height = 0;
		if (stappler::Bitmap::getImageSize(file.file, width, height)) {
			auto &val = fileData.emplace("image");
			val.setInteger(width, "width");
			val.setInteger(height, "height");
		}
	}

	fileData = Worker(*scheme, t).create(fileData, true);
	if (fileData && fileData.isInteger("__oid")) {
		auto id = fileData.getInteger("__oid");
		if (file.save(File::getFilesystemPath(id))) {
			return Value(id);
		}
	}

	file.close();
	return Value();
}

mem::Value File::createFile(const Transaction &t, const mem::StringView &type, const mem::StringView &path, int64_t mtime) {
	auto scheme = internals::getFileScheme();
	auto size = stappler::filesystem::size(path);

	mem::Value fileData;
	fileData.setString(type, "type");
	fileData.setInteger(size, "size");
	if (mtime) {
		fileData.setInteger(mtime, "mtime");
	}

	size_t width = 0, height = 0;
	if (stappler::Bitmap::getImageSize(mem::StringView(path), width, height)) {
		auto &val = fileData.emplace("image");
		val.setInteger(width, "width");
		val.setInteger(height, "height");
	}

	fileData = Worker(*scheme, t).create(fileData, true);
	if (fileData && fileData.isInteger("__oid")) {
		auto id = fileData.getInteger("__oid");
		if (stappler::filesystem::move(path, File::getFilesystemPath(id))) {
			return Value(id);
		} else {
			Worker(*scheme, t).remove(fileData.getInteger("__oid"));
		}
	}

	stappler::filesystem::remove(path);
	return Value();
}

mem::Value File::createFile(const Transaction &t, const mem::StringView &type, const mem::Bytes &data, int64_t mtime) {
	auto scheme = internals::getFileScheme();
	auto size = data.size();

	Value fileData;
	fileData.setString(type, "type");
	fileData.setInteger(size, "size");
	if (mtime) {
		fileData.setInteger(mtime, "mtime");
	}

	size_t width = 0, height = 0;
	stappler::CoderSource source(data);
	if (stappler::Bitmap::getImageSize(source, width, height)) {
		auto &val = fileData.emplace("image");
		val.setInteger(width, "width");
		val.setInteger(height, "height");
	}

	fileData = Worker(*scheme, t).create(fileData, true);
	if (fileData && fileData.isInteger("__oid")) {
		auto id = fileData.getInteger("__oid");
		if (stappler::filesystem::write(File::getFilesystemPath(id), data)) {
			return Value(id);
		} else {
			Worker(*scheme, t).remove(fileData.getInteger("__oid"));
		}
	}

	return Value();
}

static bool getTargetImageSize(size_t W, size_t H, const MinImageSize &min, const MaxImageSize &max
		, size_t &tW, size_t &tH) {

	if (min.width > W || min.height > H) {
		float scale = 0.0f;
		if (min.width == 0) {
			scale = (float)min.height / (float)H;
		} else if (min.height == 0) {
			scale = (float)min.width / (float)W;
		} else {
			scale = std::min((float)min.width / (float)W, (float)min.height / (float)H);
		}
		tW = W * scale; tH = H * scale;
		return true;
	}

	if ((max.width != 0 && max.width < W) || (max.height != 0 && max.height < H)) {
		float scale = 0.0f;
		if (max.width == 0) {
			scale = (float)max.height / (float)H;
		} else if (max.height == 0) {
			scale = (float)max.width / (float)W;
		} else {
			scale = std::min((float)max.width / (float)W, (float)max.height / (float)H);
		}
		tW = (size_t)W * scale; tH = (size_t)H * scale;
		return true;
	}

	tW = W; tH = H;
	return false;
}

static mem::String saveImage(stappler::Bitmap &bmp) {
	file_t file = file_t::open_tmp(config::getUploadTmpImagePrefix(), false);
	mem::String path(file.path());
	file.close();

	if (!path.empty()) {
		bool ret = false;
		auto fmt = bmp.getOriginalFormat();
		if (fmt == stappler::Bitmap::FileFormat::Custom) {
			ret = bmp.save(bmp.getOriginalFormatName(), path);
		} else {
			ret = bmp.save(bmp.getOriginalFormat(), path);
		}

		if (ret) {
			return mem::String(path);
		}
	}

    return mem::String();
}

static mem::String resizeImage(stappler::Bitmap &bmp, size_t width, size_t height) {
	auto newImage = bmp.resample(width, height);
    if (newImage) {
    	return saveImage(newImage);
    }

    return mem::String();
}

static mem::Map<mem::String, mem::String> writeImages(const Field &f, InputFile &file) {
	auto field = static_cast<const FieldImage *>(f.getSlot());

	size_t width = 0, height = 0;
	size_t targetWidth, targetHeight;
	if (!stappler::Bitmap::getImageSize(file.file, width, height)) {
		return mem::Map<mem::String, mem::String>();
	}

	mem::Map<mem::String, mem::String> ret;

	bool needResize = getTargetImageSize(width, height, field->minImageSize, field->maxImageSize, targetWidth, targetHeight);
	if (needResize || field->thumbnails.size() > 0) {
		stappler::Buffer data(file.writeSize);
		stappler::io::Producer prod(file.file);
		prod.seek(0, stappler::io::Seek::Set);
		prod.read(data, file.writeSize);

		stappler::Bitmap bmp(data.data(), data.size());
	    if (!bmp) {
	    	messages::error("Storage", "Fail to open image");
	    } else {
		    if (needResize) {
		    	auto fpath = resizeImage(bmp, targetWidth, targetHeight);
		    	if (!fpath.empty()) {
		    		ret.emplace(f.getName().str<mem::Interface>(), std::move(fpath));
		    	}
		    } else {
		    	ret.emplace(f.getName().str<mem::Interface>(), file.file.path());
		    }

		    if (field->thumbnails.size() > 0) {
		    	for (auto &it : field->thumbnails) {
			    	getTargetImageSize(width, height, MinImageSize(), MaxImageSize(it.width, it.height),
			    			targetWidth, targetHeight);

			    	auto fpath = resizeImage(bmp, targetWidth, targetHeight);
			    	if (!fpath.empty()) {
			    		ret.emplace(it.name, std::move(fpath));
			    	}
		    	}
		    }
	    }
	} else {
		ret.emplace(f.getName().str<mem::Interface>(), file.path);
	}

	return ret;
}

static mem::Map<mem::String, mem::String> writeImages(const Field &f, const mem::StringView &type, const mem::Bytes &data) {
	auto field = static_cast<const FieldImage *>(f.getSlot());

	size_t width = 0, height = 0;
	size_t targetWidth, targetHeight;
	stappler::CoderSource source(data);
	if (!stappler::Bitmap::getImageSize(source, width, height)) {
		return mem::Map<mem::String, mem::String>();
	}

	mem::Map<mem::String, mem::String> ret;

	bool needResize = getTargetImageSize(width, height, field->minImageSize, field->maxImageSize, targetWidth, targetHeight);
	if (needResize || field->thumbnails.size() > 0) {
		stappler::Bitmap bmp(data);
	    if (!bmp) {
	    	messages::error("Storage", "Fail to open image");
	    } else {
		    if (needResize) {
		    	auto fpath = resizeImage(bmp, targetWidth, targetHeight);
		    	if (!fpath.empty()) {
		    		ret.emplace(f.getName().str<mem::Interface>(), std::move(fpath));
		    	}
		    } else {
		    	auto fpath = saveImage(bmp);
		    	if (!fpath.empty()) {
		    		ret.emplace(f.getName().str<mem::Interface>(), std::move(fpath));
		    	}
		    }

		    if (field->thumbnails.size() > 0) {
		    	for (auto &it : field->thumbnails) {
			    	getTargetImageSize(width, height, MinImageSize(), MaxImageSize(it.width, it.height),
			    			targetWidth, targetHeight);

			    	auto fpath = resizeImage(bmp, targetWidth, targetHeight);
			    	if (!fpath.empty()) {
			    		ret.emplace(it.name, std::move(fpath));
			    	}
		    	}
		    }
	    }
	} else {
		file_t file = file_t::open_tmp(config::getUploadTmpImagePrefix(), false);
		file.xsputn((const char *)data.data(), data.size());
		ret.emplace(f.getName().str<mem::Interface>(), file.path());
		file.close();
	}

	return ret;
}

mem::Value File::createImage(const Transaction &t, const Field &f, InputFile &file) {
	mem::Value ret;

	auto files = writeImages(f, file);
	for (auto & it : files) {
		if (it.first == f.getName() && it.second == file.path) {
			auto val = createFile(t, f, file);
			if (val.isInteger()) {
				ret.setValue(std::move(val), it.first);
			}
		} else {
			auto &field = it.first;
			auto &filePath = it.second;

			auto val = createFile(t, file.type, filePath);
			if (val.isInteger()) {
				ret.setValue(std::move(val), field);
			}
		}
	}
	return ret;
}

mem::Value File::createImage(const Transaction &t, const Field &f, const mem::StringView &type, const mem::Bytes &data, int64_t mtime) {
	mem::Value ret;

	auto files = writeImages(f, type, data);
	for (auto & it : files) {
		auto &field = it.first;
		auto &filePath = it.second;

		auto val = createFile(t, type, filePath, mtime);
		if (val.isInteger()) {
			ret.setValue(std::move(val), field);
		}
	}
	return ret;
}

bool File::removeFile(const mem::Value &val) {
	int64_t id = 0;
	if (val.isInteger()) {
		id = val.asInteger();
	} else if (val.isInteger("__oid")) {
		id = val.getInteger("__oid");
	}

	return removeFile(id);
}
bool File::removeFile(int64_t id) {
	if (id) {
		stappler::filesystem::remove(File::getFilesystemPath(id));
		return true;
	}

	return false;
}

bool File::purgeFile(const Transaction &t, const mem::Value &val) {
	int64_t id = 0;
	if (val.isInteger()) {
		id = val.asInteger();
	} else if (val.isInteger("__oid")) {
		id = val.getInteger("__oid");
	}
	return purgeFile(t, id);
}

bool File::purgeFile(const Transaction &t, int64_t id) {
	if (id) {
		if (auto scheme = internals::getFileScheme()) {
			Worker(*scheme, t).remove(id);
			stappler::filesystem::remove(File::getFilesystemPath(id));
			return true;
		}
	}
	return false;
}

const Scheme * File::getScheme() {
	return internals::getFileScheme();
}

mem::Value File::getData(const Transaction &t, uint64_t id) {
	if (auto scheme = getScheme()) {
		return Worker(*scheme, t).get(id);
	}
	return Value();
}

void File::setData(const Transaction &t, uint64_t id, const mem::Value &val) {
	if (auto scheme = getScheme()) {
		Worker(*scheme, t).update(id, val);
	}
}

NS_DB_END
