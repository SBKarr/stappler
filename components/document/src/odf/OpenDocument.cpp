
#include "SPCommon.h"
#include "OpenDocument.h"
#include "SPNetworkHandle.h"

namespace opendocument {

File File::makeText(StringView name, StringView type, StringView data) {
	return File{Text, name.str<Interface>(), type.str<Interface>(), Bytes((uint8_t *)data.data(), (uint8_t *)data.data() + data.size())};
}
File File::makeBinary(StringView name, StringView type, Bytes &&data) {
	return File{Binary, name.str<Interface>(), type.str<Interface>(), std::move(data)};
}
File File::makeFilesystem(StringView name, StringView type, StringView path) {
	return File{Filesystem, name.str<Interface>(), type.str<Interface>(), Bytes((uint8_t *)path.data(), (uint8_t *)path.data() + path.size() + 1)};
}
File File::makeNetwork(StringView name, StringView url) {
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
File File::makeFunctional(StringView name, StringView type, FileReaderCallback &&fn) {
	return File{Functional, name.str<Interface>(), type.str<Interface>(), Bytes(), move(fn)};
}

String formatMetaDouble(double val) {
	return toString(std::scientific, val);
}

String formatMetaDateTime(Time val) {
	return val.toIso8601<Interface>();
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
