// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"

#include "Test.h"
#include "Format.h"

#include <random>

NS_SP_BEGIN

struct TestManager {
	static TestManager *getInstance();

	TestManager();

	void insert(Test *t);
	void erase(Test *t);

	int64_t rand_int64_t();
	uint64_t rand_uint64_t();

	int32_t rand_int32_t();
	uint32_t rand_uint32_t();

	float rand_float();
	double rand_double();

	bool runAll() const;
	bool run(const String &) const;

	bool colorsSupported = false;
	Set<Test *> tests;

	std::random_device rd;
	std::mt19937 gen32;
	std::mt19937_64 gen64;
};

static TestManager *s_instance;

TestManager *TestManager::getInstance() {
	if (!s_instance) {
		s_instance = new TestManager();
	}
	return s_instance;
}

TestManager::TestManager() : rd(), gen32(rd()), gen64(rd()) { }

void TestManager::insert(Test *t) {
	tests.insert(t);
}
void TestManager::erase(Test *t) {
	tests.erase(t);
}

int64_t TestManager::rand_int64_t() {
	std::uniform_int_distribution<int64_t> dis;
	return dis(gen64);
}
uint64_t TestManager::rand_uint64_t() {
	std::uniform_int_distribution<uint64_t> dis;
	return dis(gen64);
}

int32_t TestManager::rand_int32_t() {
	std::uniform_int_distribution<int32_t> dis;
	return dis(gen64);
}
uint32_t TestManager::rand_uint32_t() {
	std::uniform_int_distribution<uint32_t> dis;
	return dis(gen64);
}

float TestManager::rand_float() {
	std::uniform_real_distribution<float> dis;
	return dis(gen32);
}
double TestManager::rand_double() {
	std::uniform_real_distribution<double> dis;
	return dis(gen64);
}

bool TestManager::runAll() const {
	bool success = true;
	for (auto &it : tests) {
		std::cout << format::color(format::Cyan) << format::bold() << it->name() << format::drop() << ": ";
		if (!it->run()) {
			success = false;
			std::cout << format::color(format::Red) << format::bold() << "[Failed]" << format::drop();
		} else {
			std::cout << format::color(format::Green) << format::bold() << "[Passed]" << format::drop();
		}

		const auto &desc = it->desc();
		if (!desc.empty()) {
			std::cout << ": " << it->desc();
		}
		std::cout << "\n";
	}
	return success;
}

bool TestManager::run(const String &str) const {
	bool success = true;
	for (auto &it : tests) {
		if (it->name() == str) {
			std::cout << format::color(format::Cyan) << format::bold() << it->name() << format::drop() << ": ";
			if (!it->run()) {
				success = false;
				std::cout << format::color(format::Red) << format::bold() << "[Failed]" << format::drop();
			} else {
				std::cout << format::color(format::Green) << format::bold() << "[Passed]" << format::drop();
			}

			const auto &desc = it->desc();
			if (!desc.empty()) {
				std::cout << ": " << it->desc();
			}
			std::cout << "\n";
		}
	}
	return success;
}

bool Test::RunAll() {
	return TestManager::getInstance()->runAll();
}

bool Test::Run(const String &str) {
	return TestManager::getInstance()->run(str);
}

Test::Test(const String &name) : _name(name) {
	TestManager::getInstance()->insert(this);
}

Test::~Test() {
	TestManager::getInstance()->erase(this);
}

const String & Test::name() const {
	return _name;
}
const String & Test::desc() const {
	return _desc;
}

int64_t Test::rand_int64_t() const {
	return TestManager::getInstance()->rand_int64_t();
}
uint64_t Test::rand_uint64_t() const {
	return TestManager::getInstance()->rand_uint64_t();
}

int64_t Test::rand_int32_t() const {
	return TestManager::getInstance()->rand_int32_t();
}
uint64_t Test::rand_uint32_t() const {
	return TestManager::getInstance()->rand_uint32_t();
}

float Test::rand_float() const {
	return TestManager::getInstance()->rand_float();
}
double Test::rand_double() const {
	return TestManager::getInstance()->rand_double();
}

bool MemPoolTest::run() {
	pool_t *pool = memory::pool::create(memory::pool::acquire());

	memory::pool::push(pool);
	auto ret = run(pool);
	memory::pool::pop();

	memory::pool::destroy(pool);
	return ret;
}

NS_SP_END
