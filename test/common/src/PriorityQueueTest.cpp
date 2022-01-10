/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPMemPriorityQueue.h"
#include "Test.h"

NS_SP_BEGIN

struct PriorityQueueTest : Test {
	PriorityQueueTest() : Test("PriorityQueueTest") { }

	virtual bool run() override {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "Ordering test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			for (size_t i = 0; i <  memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			bool success = true;
			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success;
		});

		runTest(stream, "Ordering first test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			for (size_t i = 0; i <  memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, true, i);
			}

			bool success = true;
			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success;
		});

		runTest(stream, "Free/Capacity test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::StorageNodes * 2 + memory::PriorityQueue<int>::PreallocatedNodes;

			stream << "init: " << queue.free_capacity();
			if (queue.free_capacity() != memory::PriorityQueue<int>::PreallocatedNodes) {
				return false;
			}

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			stream << "; fill: " << queue.free_capacity();
			if (queue.free_capacity() != queue.capacity() - nIter) {
				return false;
			}

			auto half = nIter / 2;
			for (size_t i = 0; i < half; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					stream << "; no object to pop;";
					return false;
				}
			}

			stream << "; half: " << queue.free_capacity();
			if (queue.free_capacity() != queue.capacity() - (nIter - half)) {
				return false;
			}

			for (size_t i = 0; i < nIter - nIter / 2; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					stream << "; no object to pop;";
					return false;
				}
			}

			if (queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
				stream << "; extra object to pop;";
				return false;
			}

			stream << "; end: " << queue.free_capacity();
			if (queue.free_capacity() != memory::PriorityQueue<int>::PreallocatedNodes || queue.capacity() != memory::PriorityQueue<int>::PreallocatedNodes) {
				return false;
			}

			return true;
		});

		runTest(stream, "Deallocation test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes * 2;

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			stream << "max: " << queue.capacity();

			bool success = true;
			for (size_t i = 0; i < nIter; ++ i) {
				if (!queue.pop_direct([&] (memory::PriorityQueue<int>::PriorityType, int) { })) {
					success = false;
				}
			}

			stream << "; min: " << queue.capacity();

			// check freelist validity
			for (size_t i = 0; i < memory::PriorityQueue<int>::PreallocatedNodes; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			auto p = minOf<memory::PriorityQueue<int>::PriorityType>();
			queue.foreach([&] (memory::PriorityQueue<int>::PriorityType priority, int value) {
				if (priority < p) {
					success = false;
					stream << priority << ": " << value;
				}
			});

			return success && queue.capacity() == memory::PriorityQueue<int>::PreallocatedNodes;
		});

		runTest(stream, "Clear test", count, passed, [&] {
			memory::PriorityQueue<size_t> queue;

			auto nIter = memory::PriorityQueue<int>::PreallocatedNodes + memory::PriorityQueue<int>::StorageNodes * 2;

			for (size_t i = 0; i < nIter; ++ i) {
				auto p = memory::PriorityQueue<int>::PriorityType(rand() % 33 - 16);
				queue.push(p, false, i);
			}

			queue.clear();

			return queue.capacity() == memory::PriorityQueue<int>::PreallocatedNodes;
		});

		_desc = stream.str();

		return count == passed;
	}
} _PriorityQueueTest;

NS_SP_END
