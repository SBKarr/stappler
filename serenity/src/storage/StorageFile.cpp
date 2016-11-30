// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "StorageFile.h"
#include "StorageField.h"
#include "StorageScheme.h"
#include "InputFilter.h"

#include "SPBitmap.h"
#include "SPFilesystem.h"

NS_SA_EXT_BEGIN(storage)

String File::getFilesystemPath(uint64_t oid) {
	return toString(Server(AllocStack::get().server()).getDocumentRoot(), "/uploads/", oid);
}

bool File::validateFileField(const Field &field, const InputFile &file) {
	if (field.getType() == Type::File) {
		auto ffield = static_cast<const FieldFile *>(field.getSlot());
		// check size
		if (file.writeSize > ffield->maxSize) {
			messages::error("Storage", "File is larger then max file size in field", data::Value {
				std::make_pair("field", data::Value(field.getName())),
				std::make_pair("max", data::Value((int64_t)ffield->maxSize)),
				std::make_pair("size", data::Value((int64_t)file.writeSize))
			});
			return false;
		}

		// check type
		auto &types = ffield->allowedTypes;
		if (!types.empty()) {
			bool ret = false;
			for (auto &it : types) {
				if (file.type == it) {
					ret = true;
					break;
				}
			}
			if (!ret) {
				messages::error("Storage", "Invalid file type for field", data::Value{
					std::make_pair("field", data::Value(field.getName())),
					std::make_pair("type", data::Value(file.type))
				});
				return false;
			}
		}

	} else if (field.getType() == Type::Image) {
		auto ffield = static_cast<const FieldImage *>(field.getSlot());

		// check size
		if (file.writeSize > ffield->maxSize) {
			messages::error("Storage", "File is larger then max file size in field", data::Value{
				std::make_pair("field", data::Value(field.getName())),
				std::make_pair("max", data::Value((int64_t)ffield->maxSize)),
				std::make_pair("size", data::Value((int64_t)file.writeSize))
			});
			return false;
		}

		if (file.type != "image/gif"
				&& file.type != "image/jpeg"
				&& file.type != "image/pjpeg"
				&& file.type != "image/png"
				&& file.type != "image/tiff"
				&& file.type != "image/webp") {

			messages::error("Storage", "Unknown image type for field", data::Value {
				std::make_pair("field", data::Value(field.getName())),
				std::make_pair("type", data::Value(file.type))
			});
			return false;
		}

		// check type
		auto &types = ffield->allowedTypes;
		if (!types.empty()) {
			bool ret = false;
			for (auto &it : types) {
				if (file.type == it) {
					ret = true;
					break;
				}
			}
			if (!ret) {
				messages::error("Storage", "Invalid file type for field", data::Value{
					std::make_pair("field", data::Value(field.getName())),
					std::make_pair("type", data::Value(file.type))
				});
				return false;
			}
		}

		size_t width = 0, height = 0;
		if (!Bitmap::getImageSize(file.file, width, height) && width > 0 && height > 0) {
			messages::error("Storage", "Fail to detect file size with");
			return false;
		}

		if (ffield->minImageSize.policy == ImagePolicy::Reject) {
			if (ffield->minImageSize.width > width || ffield->minImageSize.height > height) {
				messages::error("Storage", "Image is to small, rejected by policy rule", data::Value{
					std::make_pair("min", data::Value {
						std::make_pair("width", data::Value(ffield->minImageSize.width)),
						std::make_pair("height", data::Value(ffield->minImageSize.height))
					}),
					std::make_pair("current", data::Value{
						std::make_pair("width", data::Value(width)),
						std::make_pair("height", data::Value(height))
					})
				});
				return false;
			}
		}

		if (ffield->maxImageSize.policy == ImagePolicy::Reject) {
			if (ffield->maxImageSize.width < width || ffield->maxImageSize.height < height) {
				messages::error("Storage", "Image is to large, rejected by policy rule", data::Value{
					std::make_pair("max", data::Value {
						std::make_pair("width", data::Value(ffield->maxImageSize.width)),
						std::make_pair("height", data::Value(ffield->maxImageSize.height))
					}),
					std::make_pair("current", data::Value{
						std::make_pair("width", data::Value(width)),
						std::make_pair("height", data::Value(height))
					})
				});
				return false;
			}
		}
	}
	return true;
}

data::Value File::createFile(Adapter *adapter, const Field &f, InputFile &file) {
	auto scheme = Server(AllocStack::get().server()).getFileScheme();
	data::Value fileData;
	fileData.setString(file.type, "type");
	fileData.setInteger(file.writeSize, "size");

	if (f.getType() == Type::Image) {
		size_t width = 0, height = 0;
		if (Bitmap::getImageSize(file.file, width, height)) {
			auto &val = fileData.emplace("image");
			val.setInteger(width, "width");
			val.setInteger(height, "height");
		}
	}

	fileData = scheme->create(adapter, fileData, true);
	if (fileData && fileData.isInteger("__oid")) {
		auto id = fileData.getInteger("__oid");
		if (file.save(File::getFilesystemPath(id))) {
			return data::Value(id);
		}
	}

	file.close();
	return data::Value();
}

data::Value File::createFile(Adapter *adapter, const String &type, const String &path) {
	auto scheme = Server(AllocStack::get().server()).getFileScheme();
	auto size = filesystem::size(path);

	data::Value fileData;
	fileData.setString(type, "type");
	fileData.setInteger(size, "size");

	size_t width = 0, height = 0;
	if (Bitmap::getImageSize(path, width, height)) {
		auto &val = fileData.emplace("image");
		val.setInteger(width, "width");
		val.setInteger(height, "height");
	}

	fileData = scheme->create(adapter, fileData, true);
	if (fileData && fileData.isInteger("__oid")) {
		auto id = fileData.getInteger("__oid");
		if (filesystem::move(path, File::getFilesystemPath(id))) {
			return data::Value(id);
		}
	}

	filesystem::remove(path);
	return data::Value();
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

String resizeImage(Map<String, String> &ret, ImageInfo * info, Image *image, size_t width, size_t height, ExceptionInfo * ex) {
	auto newImage = ResizeImage(image, width, height, LanczosFilter, 1.0, ex);
    if (ex->severity > ErrorException) {
    	messages::error("Storage", "Fail to resize image", data::Value(ex->description));
    }

    if (newImage) {
    	apr::file file; file.open_tmp(config::getUploadTmpImagePrefix(), APR_FOPEN_CREATE | APR_FOPEN_READ | APR_FOPEN_WRITE | APR_FOPEN_EXCL);

    	FILE *filePtr = fdopen(dup(file.fd()), "w");
    	if (filePtr) {
    	    SetImageInfoFile(info, filePtr);
    	    WriteImage(info, newImage);
    	    fclose(filePtr);
    	}

	    DestroyImage(newImage);
	    if (filePtr) {
		    return String(file.path());
	    }
    }

    return String();
}

Map<String, String> writeImages(const Field &f, InputFile &file) {
	auto field = static_cast<const FieldImage *>(f.getSlot());

	size_t width = 0, height = 0;
	size_t targetWidth, targetHeight;
	if (!Bitmap::getImageSize(file.file, width, height)) {
		return Map<String, String>();
	}

	FILE *filePtr = fdopen(dup(file.file.fd()), "r");
	if (!filePtr) {
		return Map<String, String>();
	}

	fseek(filePtr, 0, SEEK_SET);

	Map<String, String> ret;

	bool needResize = getTargetImageSize(width, height, field->minImageSize, field->maxImageSize, targetWidth, targetHeight);
	if (needResize || field->thumbnails.size() > 0) {
		ExceptionInfo * ex = AcquireExceptionInfo();
		GetExceptionInfo(ex);

		ImageInfo * info = AcquireImageInfo();
		GetImageInfo(info);
	    SetImageInfoFile(info, filePtr);

	    Image * image = ReadImage(info, ex);

	    if (ex->severity > ErrorException) {
	    	messages::error("Storage", "Fail to open image", data::Value(ex->description));
	    } else {
		    if (needResize) {
		    	auto fpath = resizeImage(ret, info, image, targetWidth, targetHeight, ex);
		    	if (!fpath.empty()) {
		    		ret.emplace(f.getName(), std::move(fpath));
		    	}
		    } else {
		    	ret.emplace(f.getName(), file.file.path());
		    }

		    if (field->thumbnails.size() > 0) {
		    	for (auto &it : field->thumbnails) {
			    	getTargetImageSize(width, height, MinImageSize(), MaxImageSize(it.width, it.height),
			    			targetWidth, targetHeight);

			    	auto fpath = resizeImage(ret, info, image, targetWidth, targetHeight, ex);
			    	if (!fpath.empty()) {
			    		ret.emplace(it.name, std::move(fpath));
			    	}
		    	}
		    }
	    }

		if (image) { DestroyImage(image); }
		if (info) { DestroyImageInfo(info); }
		if (ex) { DestroyExceptionInfo(ex); }
	} else {
		ret.emplace(f.getName(), file.path);
	}

	fclose(filePtr);
	return ret;
}

data::Value File::createImage(Adapter *adapter, const Field &f, InputFile &file) {
	data::Value ret;
	auto files = writeImages(f, file);
	for (auto & it : files) {
		if (it.first == f.getName() && it.second == file.path) {
			auto val = createFile(adapter, f, file);
			if (val.isInteger()) {
				ret.setValue(std::move(val), it.first);
			}
		} else {
			auto &field = it.first;
			auto &filePath = it.second;

			auto val = createFile(adapter, file.type, filePath);
			if (val.isInteger()) {
				ret.setValue(std::move(val), field);
			}
		}
	}
	return ret;
}

bool File::removeFile(Adapter *adapter, const Field &f, const data::Value &val) {
	int64_t id = 0;
	if (val.isInteger()) {
		id = val.asInteger();
	} else if (val.isInteger("__oid")) {
		id = val.getInteger("__oid");
	}

	if (id) {
		filesystem::remove(File::getFilesystemPath(id));
		return true;
	}

	return false;
}

bool File::purgeFile(Adapter *adapter, const Field &f, const data::Value &val) {
	int64_t id = 0;
	if (val.isInteger()) {
		id = val.asInteger();
	} else if (val.isInteger("__oid")) {
		id = val.getInteger("__oid");
	}

	if (id) {
		auto scheme = Server(AllocStack::get().server()).getFileScheme();
		if (adapter->removeObject(scheme, id)) {
			filesystem::remove(File::getFilesystemPath(id));
		}
		return true;
	}

	return false;
}
Scheme * File::getScheme() {
	auto serv = AllocStack::get().server();
	if (serv) {
		return Server(serv).getFileScheme();
	}
	return nullptr;
}

data::Value File::getData(Adapter *adapter, uint64_t id) {
	auto scheme = getScheme();
	if (scheme) {
		return scheme->get(adapter, id);
	}
	return data::Value();
}

void File::setData(Adapter *adapter, uint64_t id, const data::Value &val) {
	auto scheme = getScheme();
	if (scheme) {
		scheme->update(adapter, id, val);
	}
}

NS_SA_EXT_END(storage)
