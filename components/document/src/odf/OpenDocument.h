
#ifndef UTILS_SRC_ODT_H_
#define UTILS_SRC_ODT_H_

#include "SPCommon.h"
#include "SPString.h"
#include "SPMemory.h"

namespace opendocument {

using namespace stappler::mem_pool;

using Buffer = stappler::BufferTemplate<Interface>;

class Node;
class MasterPage;

enum class DocumentType {
	Text, // application/vnd.oasis.opendocument.text
};


using WriteCallback = Callback<void(const StringView &)>;
using FileReaderCallback = Function<void(const WriteCallback &)>;

enum FileType {
	Text,
	Binary,
	Filesystem,
	Functional,
};

struct File {
	static File makeText(StringView, StringView, StringView);
	static File makeBinary(StringView , StringView, Bytes &&);
	static File makeFilesystem(StringView, StringView, StringView);
	static File makeNetwork(StringView, StringView);
	static File makeFunctional(StringView, StringView, FileReaderCallback &&);

	FileType fileType;

	String name;
	String type;
	Bytes data;
	FileReaderCallback callback;
};

enum MetaType {
	Generator,
	Title,
	Description,
	Subject,
	Keywords,
	InitialCreator,
	Creator,
	PrintedBy,
	CreationDate,
	Date,
	PrintDate,
	Language
};

enum class MetaFormat {
	String,
	Double,
	DateTime,
	Boolean
};

struct Font {
	enum class Generic {
		None,
		Decorative,
		Modern,
		Roman,
		Script,
		Swiss,
		System
	};

	enum class Pitch {
		None,
		Fixed,
		Variable
	};

	String family;
	Generic generic = Generic::None;
	Pitch pitch = Pitch::None;

	Vector<Pair<String /* file */, String /* format */>> files;
};


String formatMetaDouble(double);
String formatMetaDateTime(Time);
String formatMetaBoolean(bool);

void writeEscaped(const WriteCallback &, StringView);

using Escaped = stappler::ValueWrapper<StringView, class EscapedTag>;
using PercentValue = stappler::ValueWrapper<float, class PercentValueTag>;

const WriteCallback &operator<<(const WriteCallback &cb, const char *str);
const WriteCallback &operator<<(const WriteCallback &cb, const StringView &str);
const WriteCallback &operator<<(const WriteCallback &cb, const Escaped &str);
const WriteCallback &operator<<(const WriteCallback &cb, const PercentValue &str);


}

#endif /* UTILS_SRC_ODT_H_ */
