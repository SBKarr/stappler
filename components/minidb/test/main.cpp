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

#include "STRoot.h"
#include "MDBStorage.h"

namespace stappler {

static constexpr auto HELP_STRING = "MiniDB Test\n";

static constexpr auto s_config = R"Config({
	"listen": "127.0.0.1:8080",
	"hosts" : [
		{
			"name": "localhost",
			"admin": "serenity@stappler.org",
			"root": "$WORK_DIR/www",
			"components": [{
				"name": "TestComponent",
				"data": {
					"admin": "serenity@stappler.org",
					"root": "$WORK_DIR/www",
				}
			}],
			"db": {
				"host": "localhost",
				"dbname": "test",
				"user": "serenity",
				"password": "serenity",
			}
		}
	]
})Config";

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, StringView str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	}
	return 1;
}

SP_EXTERN_C int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	auto pool = memory::pool::create((memory::pool_t *)nullptr);
	memory::pool::context ctx(pool);

	auto val = data::read<StringView, stellator::mem::Interface>(StringView(s_config));

	std::cout << data::EncodeFormat::Pretty << val << "\n";

	db::Scheme _objects = db::Scheme("objects");
	db::Scheme _refs = db::Scheme("refs");
	db::Scheme _subobjects = db::Scheme("subobjects");
	db::Scheme _images = db::Scheme("images");
	db::Scheme _test = db::Scheme("test");

	using namespace db;

	_objects.define({
		db::Field::Text("text", db::MinLength(3)),
		db::Field::Extra("data", Vector<db::Field>{
			db::Field::Array("strings", db::Field::Text("")),
		}),
		db::Field::Set("subobjects", _subobjects),
		db::Field::File("image", db::MaxFileSize(1_MiB)),
		db::Field::Text("alias", db::Transform::Alias),
		db::Field::Integer("mtime", db::Flags::AutoMTime | db::Flags::Indexed),
		db::Field::Integer("index", db::Flags::Indexed),
		db::Field::View("refs", _refs, db::ViewFn([] (const db::Scheme &objScheme, const data::Value &obj) -> bool {
			return true;
		}), db::FieldView::Delta),
		db::Field::Set("images", _images, db::Flags::Composed),
	});

	_refs.define({
		Field::Text("alias", Transform::Alias),
		Field::Text("text", MinLength(3)),
		Field::Set("features", _objects, RemovePolicy::StrongReference),
		Field::Set("optionals", _objects, RemovePolicy::Reference),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
		Field::File("file", MaxFileSize(100_KiB)),
		Field::Array("array", Field::Text("", MaxLength(10))),
		Field::Object("objectRef", _objects, Flags::Reference),

		Field::Image("cover", MaxImageSize(1080, 1080, ImagePolicy::Resize), Vector<Thumbnail>({
			Thumbnail("thumb", 160, 160),
			Thumbnail("cover512", 512, 512),
			Thumbnail("cover256", 256, 256),
			Thumbnail("cover128", 128, 128),
			Thumbnail("cover64", 64, 64),
		})),

		Field::Data("data")
	});

	_subobjects.define({
		Field::Text("text", MinLength(3)),
		Field::Object("object", _objects),
		Field::Integer("mtime", Flags::AutoMTime | Flags::Indexed),
		Field::Integer("index", Flags::Indexed),
	});

	_images.define({
		Field::Integer("ctime", Flags::ReadOnly | Flags::AutoCTime | Flags::ForceInclude),
		Field::Integer("mtime", Flags::ReadOnly | Flags::AutoMTime | Flags::ForceInclude),

		Field::Text("name", Transform::Identifier, Flags::Required | Flags::Indexed | Flags::ForceInclude),

		Field::Image("content", MaxImageSize(2048, 2048, ImagePolicy::Resize), Vector<Thumbnail>({
			Thumbnail("thumb", 380, 380)
		})),
	});

	_test.define({
		Field::Text("key"),
		Field::Integer("time", Flags::Indexed),
		Field::Data("data")
	});

	mem::Map<mem::StringView, const db::Scheme *> schemes;
	schemes.emplace(_objects.getName(), &_objects);
	schemes.emplace(_refs.getName(), &_refs);
	schemes.emplace(_subobjects.getName(), &_subobjects);
	schemes.emplace(_images.getName(), &_images);
	schemes.emplace(_test.getName(), &_test);

	db::Scheme::initSchemes(schemes);

	db::minidb::Storage *storage = nullptr;
	auto writablePath = filesystem::writablePath("tmp.minidb");
	if (filesystem::exists(writablePath)) {
		storage = db::minidb::Storage::open(pool, writablePath);
	} else {
		storage = db::minidb::Storage::create(pool, writablePath);
	}

	storage->init(schemes);

	return 0;
}

}
