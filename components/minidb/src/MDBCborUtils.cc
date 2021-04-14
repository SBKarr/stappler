/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "MDBCbor.h"
#include "SPByteOrder.h"
#include "SPHalfFloat.h"

#define CBOR_DEBUG 0

namespace db::minidb::cbor {

const uint8_t CborHeaderData[] = { (uint8_t)0xd9, (uint8_t)0xd9, (uint8_t)0xf7 };
uint32_t CborHeaderSize = 3;

static void *CborAlloc(size_t bytes) {
	return malloc(bytes);
}

static void *CborRealloc(void *ptr, size_t bytes) {
	return realloc(ptr, bytes);
}

static void CborFree(void *ptr) {
	free(ptr);
}

#if CBOR_DEBUG
static const char *getCborTokenName(IteratorToken tok) {
	switch (tok) {
	case IteratorToken::Done: return "Done"; break;
	case IteratorToken::Key: return "Key"; break;
	case IteratorToken::Value: return "Value"; break;
	case IteratorToken::BeginArray: return "BeginArray"; break;
	case IteratorToken::EndArray: return "EndArray"; break;
	case IteratorToken::BeginObject: return "BeginObject"; break;
	case IteratorToken::EndObject: return "EndObject"; break;
	case IteratorToken::BeginByteStrings: return "BeginByteStrings"; break;
	case IteratorToken::EndByteStrings: return "EndByteStrings"; break;
	case IteratorToken::BeginCharStrings: return "BeginCharStrings"; break;
	case IteratorToken::EndCharStrings: return "EndCharStrings"; break;
	}
	return nullptr;
}
#endif

static void writeTokenInfo(IteratorContext *ctx) {
#if CBOR_DEBUG
	auto indent = ctx->stackSize;
	if (ctx->token == IteratorToken::BeginObject || ctx->token == IteratorToken::BeginArray) {
		-- indent;
	}

	for (size_t i = 0; i < indent; ++ i) {
		std::cout << "\t";
	}
	std::cout << getCborTokenName(ctx->token);
	switch (ctx->token) {
	case IteratorToken::Key:
		if (MajorType(ctx->type) == MajorType::ByteString || MajorType(ctx->type) == MajorType::CharString) {
			auto key = mem::StringView((const char *)ctx->current.ptr, ctx->objectSize);
			std::cout << ": \"" << key << "\"";
		}
		break;
	case IteratorToken::Value:
		switch (MajorType(ctx->type)) {
		case MajorType::Unsigned: std::cout << ": (unsigned)"; break;
		case MajorType::Negative: std::cout << ": (negative)"; break;
		case MajorType::ByteString: std::cout << ": (bytes)"; break;
		case MajorType::CharString: std::cout << ": (string)"; break;
		case MajorType::Array: std::cout << ": (array)"; break;
		case MajorType::Map: std::cout << ": (map)"; break;
		case MajorType::Tag: std::cout << ": (tag)"; break;
		case MajorType::Simple: std::cout << ": (simple)"; break;
		}
		break;
	case IteratorToken::BeginObject:
	case IteratorToken::BeginArray:
		std::cout << ": [" << ctx->getContainerSize() << "]";
		break;
	default:
		break;
	}
	std::cout << "\n";
#endif
}

bool data_is_cbor(const uint8_t *data, uint32_t size) {
	if (size > 3 && data[0] == 0xd9 && data[1] == 0xd9 && data[2] == 0xf7) {
		return true;
	}
	return false;
}

static inline uint32_t get_cbor_integer_length(uint8_t info) {
	switch (Flags(info)) {
	case Flags::AdditionalNumber8Bit: return 1; break;
	case Flags::AdditionalNumber16Bit: return 2; break;
	case Flags::AdditionalNumber32Bit: return 4; break;
	case Flags::AdditionalNumber64Bit: return 8; break;
	default: return 0; break;
	}

	return 0;
}

uint8_t Data::getUnsigned() const {
	return *ptr;
}

uint16_t Data::getUnsigned16() const {
	uint16_t ret = 0;
	memcpy(&ret, ptr, sizeof(uint16_t));
	return stappler::byteorder::bswap16(ret);
}

uint32_t Data::getUnsigned32() const {
	uint32_t ret = 0;
	memcpy(&ret, ptr, sizeof(uint32_t));
	return stappler::byteorder::bswap32(ret);
}

uint64_t Data::getUnsigned64() const {
	uint64_t ret = 0;
	memcpy(&ret, ptr, sizeof(uint64_t));
	return stappler::byteorder::bswap64(ret);
}

float Data::getFloat16() const {
	uint16_t value = getUnsigned16();
	return stappler::halffloat::decode(value);
}

float Data::getFloat32() const {
	float ret = 0.0f;
	uint32_t value = getUnsigned32();
	memcpy(&ret, &value, sizeof(float));
	return ret;
}

double Data::getFloat64() const {
	double ret = 0.0f;
	uint64_t value = getUnsigned64();
	memcpy(&ret, &value, sizeof(double));
	return ret;
}

uint64_t Data::getUnsignedValue(uint8_t type) const {
	if (type < stappler::toInt(Flags::MaxAdditionalNumber)) {
		return type;
	} else if (type == stappler::toInt(Flags::AdditionalNumber8Bit)) {
		return getUnsigned();
	} else if (type == stappler::toInt(Flags::AdditionalNumber16Bit)) {
		return getUnsigned16();
	} else if (type == stappler::toInt(Flags::AdditionalNumber32Bit)) {
		return getUnsigned32();
	} else if (type == stappler::toInt(Flags::AdditionalNumber64Bit)) {
		return getUnsigned64();
	} else {
		return 0;
	}
}

uint64_t Data::readUnsignedValue(uint8_t type) {
	uint64_t ret = getUnsignedValue(type);
	switch (Flags(type)) {
	case Flags::AdditionalNumber8Bit:
		offset(1);
		break;
	case Flags::AdditionalNumber16Bit:
		offset(2);
		break;
	case Flags::AdditionalNumber32Bit:
		offset(4);
		break;
	case Flags::AdditionalNumber64Bit:
		offset(8);
		break;
	default: break;
	}
	return ret;
}


bool IteratorContext::init(const uint8_t *data, size_t size) {
	memset(this, 0, sizeof(IteratorContext));

	if (data_is_cbor(data, size)) { // prefixed block
		data += 3;
		size -= 3;
	} else if (size == 0) { // non-prefixed
		return false;
	}

	valueStart = data;
	current.ptr = data;
	current.size = size;

	currentStack = defaultStack;
	stackCapacity = CBOR_STACK_DEFAULT_SIZE;

	return true;
}

void IteratorContext::finalize() {
	if (extendedStack) {
		CborFree(extendedStack);
		extendedStack = NULL;
	}
}

void IteratorContext::reset() {
	if (extendedStack) {
		CborFree(extendedStack);
	}
	memset(this, 0, sizeof(IteratorContext));
}

static IteratorToken CborIteratorPushStack(IteratorContext *ctx, StackType type, uint32_t count, const uint8_t *ptr) {
	IteratorStackValue * newStackValue;

	if (ctx->stackCapacity == ctx->stackSize) {
		if (!ctx->extendedStack) {
			ctx->extendedStack = (IteratorStackValue *)CborAlloc(sizeof(IteratorStackValue) * ctx->stackCapacity * 2);
			memcpy(ctx->extendedStack, ctx->defaultStack, sizeof(IteratorStackValue) * ctx->stackCapacity);
			ctx->currentStack = ctx->extendedStack;
		} else {
			ctx->currentStack = ctx->extendedStack = (IteratorStackValue *)CborRealloc(ctx->extendedStack, sizeof(IteratorStackValue) * ctx->stackCapacity * 2);
			ctx->stackCapacity *= 2;
		}
		ctx->stackCapacity *= 2;
	}

	newStackValue = &ctx->currentStack[ctx->stackSize];
	newStackValue->type = type;
	newStackValue->position = 0;
	newStackValue->ptr = ptr;
	if (type == StackType::Object && count != UINT32_MAX) {
		newStackValue->count = count * 2;
	} else {
		newStackValue->count = count;
	}

	ctx->objectSize = 0;
	ctx->stackHead = newStackValue;
	++ ctx->stackSize;

	switch (type) {
	case StackType::Array:
		return IteratorToken::BeginArray;
		break;
	case StackType::Object:
		return IteratorToken::BeginObject;
		break;
	case StackType::ByteString:
		ctx->isStreaming = true;
		return IteratorToken::BeginByteStrings;
		break;
	case StackType::CharString:
		ctx->isStreaming = true;
		return IteratorToken::BeginCharStrings;
		break;
	default: break;
	}
	return IteratorToken::Done;
}

static IteratorToken CborIteratorPopStack(IteratorContext *ctx) {
	StackType type;

	if (!ctx->stackHead) {
		return IteratorToken::Done;
	}

	type = ctx->stackHead->type;
	-- ctx->stackSize;

	ctx->objectSize = 0;
	if (ctx->stackSize > 0) {
		ctx->stackHead = &ctx->currentStack[ctx->stackSize - 1];
	} else {
		ctx->stackHead = NULL;
	}

	switch (type) {
	case StackType::Array:
		return IteratorToken::EndArray;
		break;
	case StackType::Object:
		return IteratorToken::EndObject;
		break;
	case StackType::ByteString:
		ctx->isStreaming = false;
		return IteratorToken::EndByteStrings;
		break;
	case StackType::CharString:
		ctx->isStreaming = false;
		return IteratorToken::EndCharStrings;
		break;
	default: break;
	}
	return IteratorToken::Done;
}

IteratorToken IteratorContext::next() {
	IteratorStackValue *head;
	StackType nextStackType;
	uint8_t type;
	const uint8_t *ptr;

	if (!current.offset(objectSize)) {
		if (stackHead) {
			token = CborIteratorPopStack(this);
		} else {
			token = IteratorToken::Done;
		}
		value = current.ptr;
		writeTokenInfo(this);
		return token;
	}

	head = stackHead;

	// pop stack value if all objects was parsed
	if (head && head->position >= head->count) {
		token = CborIteratorPopStack(this);
		value = current.ptr;
		writeTokenInfo(this);
		return token;
	}

	// read flag value
	valueStart = ptr = current.ptr;
	type = current.getUnsigned();
	current.offset(1);

	// pop stack value for undefined length container
	if (head && head->count == UINT32_MAX && type == (stappler::toInt(Flags::UndefinedLength) | stappler::toInt(MajorTypeEncoded::Simple))) {
		token = CborIteratorPopStack(this);
		writeTokenInfo(this);
		return token;
	}

	this->type = (type & stappler::toInt(Flags::MajorTypeMaskEncoded)) >> stappler::toInt(Flags::MajorTypeShift);
	this->info = type & stappler::toInt(Flags::AdditionalInfoMask);

	nextStackType = StackType::None;

	switch (MajorType(this->type)) {
	case MajorType::Unsigned:
	case MajorType::Negative:
		this->objectSize = get_cbor_integer_length(this->info);
		if (head) { ++ head->position; }
		break;
	case MajorType::Tag:
		this->objectSize = get_cbor_integer_length(this->info);
		break;
	case MajorType::Simple:
		this->objectSize = get_cbor_integer_length(this->info);
		if (head) { ++ head->position; }
		break;
	case MajorType::ByteString:
		if (this->info == Flags::UndefinedLength) {
			nextStackType = StackType::ByteString;
		} else {
			this->objectSize = this->current.readUnsignedValue(this->info);
		}
		if (head) { ++ head->position; }
		break;
	case MajorType::CharString:
		if (this->info == Flags::UndefinedLength) {
			nextStackType = StackType::CharString;
		} else {
			this->objectSize = this->current.readUnsignedValue(this->info);
		}
		if (head) { ++ head->position; }
		break;
	case MajorType::Array:
		nextStackType = StackType::Array;
		if (head) { ++ head->position; }
		break;
	case MajorType::Map:
		nextStackType = StackType::Object;
		if (head) { ++ head->position; }
		break;
	default: break;
	}

	this->value = ptr;
	if (nextStackType != StackType::None) {
		if (this->info == Flags::UndefinedLength) {
			this->token = CborIteratorPushStack(this, nextStackType, UINT32_MAX, ptr);
		} else {
			this->token = CborIteratorPushStack(this, nextStackType, this->current.readUnsignedValue(this->info), ptr);
		}
		writeTokenInfo(this);
		return this->token;
	}

	this->token = (head && head->type == StackType::Object)
		? ( head->position % 2 == 1 ? IteratorToken::Key : IteratorToken::Value )
		: IteratorToken::Value;
	writeTokenInfo(this);
	return this->token;
}

Type IteratorContext::getType() const {
	switch (MajorType(this->type)) {
	case MajorType::Unsigned:
		return Type::Unsigned;
		break;
	case MajorType::Negative:
		return Type::Negative;
		break;
	case MajorType::ByteString:
		return Type::ByteString;
		break;
	case MajorType::CharString:
		return Type::CharString;
		break;
	case MajorType::Array:
		return Type::Array;
		break;
	case MajorType::Map:
		return Type::Map;
		break;
	case MajorType::Tag:
		return Type::Tag;
		break;
	case MajorType::Simple:
		switch (this->info) {
		case stappler::toInt(SimpleValue::False): return Type::False; break;
		case stappler::toInt(SimpleValue::True): return Type::True; break;
		case stappler::toInt(SimpleValue::Null): return Type::Null; break;
		case stappler::toInt(SimpleValue::Undefined): return Type::Undefined; break;
		case stappler::toInt(Flags::AdditionalFloat16Bit):
		case stappler::toInt(Flags::AdditionalFloat32Bit):
		case stappler::toInt(Flags::AdditionalFloat64Bit):
			return Type::Float;
			break;
		}
		return Type::Simple;
		break;
	default: break;
	}

	return Type::Unknown;
}

StackType IteratorContext::getContainerType() const {
	if (!this->stackHead) {
		return StackType::None;
	} else {
		return this->stackHead->type;
	}
}

uint32_t IteratorContext::getContainerSize() const {
	if (!this->stackHead) {
		return 0;
	} else {
		if (this->stackHead->count == UINT32_MAX) {
			return this->stackHead->count;
		}
		return (this->stackHead->type == StackType::Object) ? this->stackHead->count / 2 : this->stackHead->count;
	}
}

uint32_t IteratorContext::getContainerPosition() const {
	if (!this->stackHead) {
		return 0;
	} else {
		return (this->stackHead->type == StackType::Object) ? (this->stackHead->position - 1) / 2 : (this->stackHead->position - 1);
	}
}

uint32_t IteratorContext::getObjectSize() const {
	return this->objectSize;
}

int64_t IteratorContext::getInteger() const {
	int64_t ret = 0;
	switch (getType()) {
	case Type::Unsigned:
	case Type::Tag:
	case Type::Simple:
		ret = this->current.getUnsignedValue(this->info);
		break;
	case Type::Negative:
		ret = -1 - this->current.getUnsignedValue(this->info);
		break;
	default: break;
	}
	return ret;
}

uint64_t IteratorContext::getUnsigned() const {
	uint64_t ret = 0;
	switch (getType()) {
	case Type::Unsigned:
	case Type::Tag:
	case Type::Simple:
		ret = this->current.getUnsignedValue(this->info);
		break;
	default: break;
	}
	return ret;
}

double IteratorContext::getFloat() const {
	double ret = 0.0;
	switch (getType()) {
	case Type::Float:
		switch (Flags(this->info)) {
		case Flags::AdditionalFloat16Bit:
			ret = this->current.getFloat16();
			break;
		case Flags::AdditionalFloat32Bit:
			ret = this->current.getFloat32();
			break;
		case Flags::AdditionalFloat64Bit:
			ret = this->current.getFloat64();
			break;
		default: break;
		}
		break;
	default: break;
	}
	return ret;
}

const char *IteratorContext::getCharPtr() const {
	const char *ret = NULL;
	switch (getType()) {
	case Type::CharString:
		ret = (const char *)this->current.ptr;
		break;
	default: break;
	}
	return ret;
}

const uint8_t *IteratorContext::getBytePtr() const {
	const uint8_t *ret = NULL;
	switch (getType()) {
	case Type::ByteString:
		ret = this->current.ptr;
		break;
	default: break;
	}
	return ret;
}

const uint8_t *IteratorContext::getCurrentValuePtr() const {
	switch (this->token) {
	case IteratorToken::Key:
	case IteratorToken::Value:
		return this->value;
		break;
	case IteratorToken::BeginArray:
	case IteratorToken::BeginObject:
	case IteratorToken::BeginByteStrings:
	case IteratorToken::BeginCharStrings:
		return this->stackHead->ptr;
		break;
	default: break;
	}
	return NULL;
}

const uint8_t *IteratorContext::readCurrentValue() {
	switch (this->token) {
	case IteratorToken::Value:
	case IteratorToken::Key:
		this->next();
		return this->value;
		break;
	case IteratorToken::BeginArray: {
		uint32_t stack = this->stackSize;
		while (this->next() != IteratorToken::EndArray && this->stackSize > stack - 1) { }
		this->next();
		return this->value;
		break;
	}
	case IteratorToken::BeginObject: {
		uint32_t stack = this->stackSize;
		while (this->next() != IteratorToken::EndObject && this->stackSize > stack - 1) { }
		this->next();
		return this->value;
		break;
	}
	case IteratorToken::BeginByteStrings:
		while (this->next() != IteratorToken::EndByteStrings) { }
		this->next();
		return this->value;
		break;
	case IteratorToken::BeginCharStrings:
		while (this->next() != IteratorToken::EndCharStrings) { }
		this->next();
		return this->value;
		break;
	default:
		return NULL;
		break;
	}
	return NULL;
}

bool IteratorContext::getIth(size_t lindex) {
	uint32_t stackSize;
	IteratorStackValue *h;

	if (!this->stackHead || this->stackHead->type != StackType::Array || this->token != IteratorToken::BeginArray) {
		return false;
	}

	if (lindex >= 0) {
		if ((uint32_t)lindex >= this->stackHead->count) {
			return false;
		}
	} else if (lindex < 0) {
		if (this->stackHead->count != UINT32_MAX) {
			if ((uint32_t)-lindex > this->stackHead->count) {
				return false;
			} else {
				lindex = this->stackHead->count + lindex;
			}
		} else {
			return false;
		}
	}

	stackSize = this->stackSize;
	h = this->stackHead;

	while ((h->position != lindex || this->stackSize > stackSize) && this->token != IteratorToken::Done) {
		this->next();
	}

	if (this->stackSize < stackSize || this->token == IteratorToken::Done) {
		return false;
	} else {
		this->next();
		return true;
	}
}

bool IteratorContext::getKey(const char *str, uint32_t size) {
	uint32_t stackSize;
	const char *tmpstr = str;
	uint32_t tmpsize = size;

	if (!this->stackHead || this->stackHead->type != StackType::Object || this->token != IteratorToken::BeginObject) {
		return false;
	}

	stackSize = this->stackSize;
	while (this->token != IteratorToken::Done && this->stackSize >= stackSize) {
		auto token = this->next();
		if (this->stackSize == stackSize) {
			switch (token) {
			case IteratorToken::Key:
				if (this->isStreaming) {
					if (tmpstr && this->objectSize <= size) {
						if (memcmp(str, this->current.ptr, this->objectSize) == 0) {
							tmpstr += this->objectSize;
							size -= this->objectSize;
						} else {
							// invalid key
							tmpstr = NULL;
						}
					} else {
						tmpstr = NULL;
					}
				} else if (this->type == MajorType::ByteString || this->type == MajorType::CharString) {
					if (this->objectSize == size && memcmp(str, this->current.ptr, size) == 0) {
						this->next();
						return true;
					}
				}
				break;
			case IteratorToken::Value:
				if (tmpsize == 0) {
					return true;
				}
				tmpstr = str;
				tmpsize = size;
				break;
			default:
				break;
			}
		}
	}

	if (this->stackSize < stackSize || this->token == IteratorToken::Done) {
		return false;
	} else {
		return true;
	}
}

bool IteratorContext::path(CborIteratorPathCallback cb, void *ptr) {
	Data data = cb(ptr);

	IteratorToken token = this->next();
	switch (token) {
	case IteratorToken::BeginArray:
	case IteratorToken::BeginObject:
		if (data.ptr == NULL) {
			return true;
		}
		// continue
		break;
	case IteratorToken::Value:
	case IteratorToken::BeginByteStrings:
	case IteratorToken::BeginCharStrings:
		return data.ptr == NULL;
		break;
	default:
		return false;
		break;
	}

	while (data.ptr) {
		switch (token) {
		case IteratorToken::BeginArray: {
			const char *indextext = (const char *)data.ptr;
			char *endptr;
			long int lindex = strtol(indextext, &endptr, 10);
			if (endptr == indextext || *endptr != '\0' || lindex > INT_MAX || lindex < INT_MIN) {
				return false;
			}

			if (!this->getIth(lindex)) {
				return false;
			}
			token = this->token;
			break;
		}
		case IteratorToken::BeginObject:
			if (!this->getKey((const char *)data.ptr, data.size)) {
				return false;
			}
			token = this->token;
			break;
		default:
			return false;
			break;
		}
		data = cb(ptr);
	}

	return true;
}

struct CborIteratorPathStringsData {
	const char **path;
	int npath;
};

static Data CborIteratorPathStringsIter(struct CborIteratorPathStringsData *data) {
	if (data->npath > 0) {
		Data ret({uint32_t((*data->path) ? strlen(*data->path) : 0), (const uint8_t *)*data->path});
		-- data->npath;
		++ data->path;
		return ret;
	} else {
		Data ret = { 0, NULL };
		return ret;
	}
}

bool IteratorContext::pathStrings(const char **path, int npath) {
	struct CborIteratorPathStringsData data = {
		path,
		npath
	};

	return this->path((CborIteratorPathCallback)&CborIteratorPathStringsIter, &data);
}

}
