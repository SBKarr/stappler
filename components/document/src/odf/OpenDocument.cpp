
#include "SPCommon.h"
#include "OpenDocument.h"
#include "SPNetworkHandle.h"

#include "ODZip.cc"

namespace opendocument {

static void *fopen_mem_func(void * opaque, const void* filename, int mode) {
	return opaque;
}

static unsigned long fread_mem_func (void *opaque, void *stream, void* buf, unsigned long size) {
	Buffer *mem = (Buffer *)stream;

	auto r = mem->pop<BytesView>(size);

	memcpy(buf, (char*)r.data(), r.size());
	return r.size();
}

static unsigned long fwrite_mem_func (void *opaque, void *stream, const void* buf, unsigned long size) {
	Buffer *mem = (Buffer *)stream;

	mem->put((const char *)buf, size);
	return size;
}

static long long unsigned ftell_mem_func (void *opaque, void *stream) {
	Buffer *mem = (Buffer *)stream;

	return mem->size();
}

static long fseek_mem_func (void *opaque, void *stream, unsigned long long offset, int origin) {
	Buffer *mem = (Buffer *)stream;

	unsigned long new_pos = 0;
	switch (origin) {
		case ZLIB_FILEFUNC_SEEK_CUR : {
			new_pos = mem->size() + offset;
			break;
		}
		case ZLIB_FILEFUNC_SEEK_END : {
			new_pos = mem->input() + offset;
			break;
		}
		case ZLIB_FILEFUNC_SEEK_SET : {
			new_pos = offset;
			break;
		}
		default: {
			return -1;
		}
	}

	if (mem->seek(new_pos)) {
		return 0;
	}
	return -1;
}

static int fclose_mem_func (void *opaque, void *stream) {
	return 0;
}

static int ferror_mem_func (void *opaque, void *stream) {
	/* We never return errors */
	return 0;
}

static zlib_filefunc64_32_def FileContainer_makeDefs(Buffer *buf) {
	zlib_filefunc64_32_def ret;
	ret.zfile_func64.zopen64_file = fopen_mem_func;
	ret.zfile_func64.zread_file = fread_mem_func;
	ret.zfile_func64.zwrite_file = fwrite_mem_func;
	ret.zfile_func64.ztell64_file = ftell_mem_func;
	ret.zfile_func64.zseek64_file = fseek_mem_func;
	ret.zfile_func64.zclose_file = fclose_mem_func;
	ret.zfile_func64.zerror_file = ferror_mem_func;
	ret.zfile_func64.opaque = (void *)buf;
	ret.zopen32_file = nullptr;
	ret.ztell32_file = nullptr;
	ret.zseek32_file = nullptr;
	return ret;
}


File File::makeText(const StringView &name, const StringView &type, const StringView &data) {
	return File{Text, name.str<Interface>(), type.str<Interface>(), Bytes((uint8_t *)data.data(), (uint8_t *)data.data() + data.size())};
}
File File::makeBinary(const StringView &name, const StringView &type, Bytes &&data) {
	return File{Binary, name.str<Interface>(), type.str<Interface>(), std::move(data)};
}
File File::makeFilesystem(const StringView &name, const StringView &type, const StringView &path) {
	return File{Filesystem, name.str<Interface>(), type.str<Interface>(), Bytes((uint8_t *)path.data(), (uint8_t *)path.data() + path.size() + 1)};
}
File File::makeNetwork(const StringView &name, const StringView &url) {
	String type;
	Bytes data;

	stappler::NetworkHandle h;
	h.init(stappler::NetworkHandle::Method::Get, url);

	Buffer stream;
	h.setReceiveCallback([&] (char *data, size_t size) -> size_t {
		stream.put(data, size);
		return size;
	});
	if (h.perform()) {
		auto r = stream.get();
		data = Bytes(r.data(), r.data() + r.size());
		type = h.getContentType().str<Interface>();
	}

	return makeBinary(name, type, std::move(data));
}
File File::makeFunctional(const StringView &name, const StringView &type, FileReaderCallback &&fn) {
	return File{Functional, name.str<Interface>(), type.str<Interface>(), Bytes(), move(fn)};
}

String formatMetaDouble(double val) {
	return toString(std::scientific, val);
}

String formatMetaDateTime(Time val) {
	return val.toIso8601();
}

String formatMetaBoolean(bool val) {
	return val ? "true" : "false";
}

void writeEscaped(const WriteCallback &cb, StringView r) {
	while (!r.empty()) {
		auto tmp = r.readUntil<StringView::Chars<'"', '\'', '<', '>', '&'>>();
		if (!tmp.empty()) {
			cb(tmp);
		}
		if (r.empty()) {
			return;
		} else if (r.is('"')) {
			cb("&quot;");
			++ r;
		} else if (r.is('\'')) {
			cb("&apos;");
			++ r;
		} else if (r.is('<')) {
			cb("&lt;");
			++ r;
		} else if (r.is('>')) {
			cb("&gt;");
			++ r;
		} else if (r.is('&')) {
			if (r.is("&amp;")) {
				cb("&amp;"); r += "&amp;"_len;
			} else if (r.is("&quot;")) {
				cb("&quot;"); r += "&quot;"_len;
			} else if (r.is("&quot;")) {
				cb("&quot;"); r += "&quot;"_len;
			} else if (r.is("&apos;")) {
				cb("&apos;"); r += "&apos;"_len;
			} else if (r.is("&lt;")) {
				cb("&lt;"); r += "&lt;"_len;
			} else if (r.is("&gt;")) {
				cb("&gt;"); r += "&gt;"_len;
			} else if (r.is("&#")) {
				cb("&#"); r += 2;
				cb(r.readUntil<StringView::Chars<';'>>());
				if (r.is(';')) {
					cb(";");
					++ r;
				}
			} else {
				cb("&amp;");
				++ r;
			}
		}
	}
}

const WriteCallback &operator<<(const WriteCallback &cb, const char *str) {
	cb(StringView(str, strlen(str)));
	return cb;
}

const WriteCallback &operator<<(const WriteCallback &cb, const StringView &str) {
	cb(str);
	return cb;
}

const WriteCallback &operator<<(const WriteCallback &cb, const Escaped &str) {
	writeEscaped(cb, str.get());
	return cb;
}

const WriteCallback &operator<<(const WriteCallback &cb, const PercentValue &val) {
	char b[20] = {0};
	auto ret = snprintf(b, 19, "%g%%", val.get() * 100.0f);
	cb(StringView(&b[0], ret));
	return cb;
}

}

#include "ODStyle.cc"
#include "ODContent.cc"

#include "ODDocument.cc"
#include "ODTextDocument.cc"
#include "ODMMDProcessor.cc"
