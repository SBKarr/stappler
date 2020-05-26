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

#include "SPMemPoolStruct.h"

#if LINUX
#include <sys/mman.h>
#endif

namespace stappler::mempool::custom {

static uint32_t allocator_mmap_realloc(int filedes, void *ptr, uint32_t idx, uint32_t required) {
#if LINUX
	auto oldSize = idx * BOUNDARY_SIZE;
	auto newSize = idx * 2 * BOUNDARY_SIZE;
	if (newSize / BOUNDARY_SIZE < required) {
		newSize = required * BOUNDARY_SIZE;
	}

	if (newSize > ALLOCATOR_MMAP_RESERVED) {
		perror("ALLOCATOR_MMAP_RESERVED exceeded");
		return 0;
	}

	if (lseek(filedes, newSize - 1, SEEK_SET) == -1) {
		close(filedes);
		perror("Error calling lseek() to 'stretch' the file");
		return 0;
	}

	if (write(filedes, "", 1) == -1) {
		close(filedes);
		perror("Error writing last byte of the file");
		return 0;
	}

	munmap((char *)ptr + oldSize, newSize - oldSize);
	auto err = mremap(ptr, oldSize, newSize, 0);
	if (err != MAP_FAILED) {
		return newSize / BOUNDARY_SIZE;
	}
	auto memerr = errno;
	switch (memerr) {
	case EAGAIN: perror("EAGAIN"); break;
	case EFAULT: perror("EFAULT"); break;
	case EINVAL: perror("EINVAL"); break;
	case ENOMEM: perror("ENOMEM"); break;
	default: break;
	}
	return 0;
#else
	return 0;
#endif
}

Allocator::Allocator() {
	buf.fill(nullptr);
}

Allocator::~Allocator() {
	MemNode *node, **ref;

	if (!mmapPtr) {
		for (uint32_t index = 0; index < MAX_INDEX; index++) {
			ref = &buf[index];
			while ((node = *ref) != nullptr) {
				*ref = node->next;
				::free(node);
			}
		}
	} else {
#if LINUX
		munmap(mmapPtr, mmapMax * BOUNDARY_SIZE);
		close(mmapdes);
#endif
	}
}

bool Allocator::run_mmap(uint32_t idx) {
#if LINUX
	if (idx == 0) {
		idx = 1_KiB;
	}

	std::unique_lock<Allocator> lock(*this);

	if (mmapdes != -1) {
		return true;
	}

	char nameBuff[256] = { 0 };
	snprintf(nameBuff, 255, "/tmp/stappler.mmap.%d.%p.XXXXXX", getpid(), (void *)this);

	mmapdes = mkstemp(nameBuff);
	unlink(nameBuff);

	size_t size = BOUNDARY_SIZE * idx;

	if (lseek(mmapdes, size - 1, SEEK_SET) == -1) {
		close(mmapdes);
		perror("Error calling lseek() to 'stretch' the file");
		return false;
	}

	if (write(mmapdes, "", 1) == -1) {
		close(mmapdes);
		perror("Error writing last byte of the file");
		return false;
	}

	void *reserveMem = mmap(NULL, ALLOCATOR_MMAP_RESERVED, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	// Now the file is ready to be mmapped.
	void *map = mmap(reserveMem, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED | MAP_NORESERVE, mmapdes, 0);
	if (map == MAP_FAILED) {
		close(mmapdes);
		perror("Error mmapping the file");
		return false;
	}

	mmapPtr = map;
	mmapMax = idx;
	return true;
#else
	return false;
#endif
}

void Allocator::set_max(uint32_t size) {
	std::unique_lock<Allocator> lock(*this);

	uint32_t max_free_index = uint32_t(ALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX);
	current += max_free_index;
	current -= max;
	max = max_free_index;
	if (current > max) {
		current = max;
	}
}

MemNode *Allocator::alloc(uint32_t in_size) {
	std::unique_lock<Allocator> lock;

	uint32_t size = uint32_t(ALIGN(in_size + SIZEOF_MEMNODE, BOUNDARY_SIZE));
	if (size < in_size) {
		return nullptr;
	}
	if (size < MIN_ALLOC) {
		size = MIN_ALLOC;
	}

	size_t index = (size >> BOUNDARY_INDEX) - 1;
	if (index > maxOf<uint32_t>()) {
		return nullptr;
	}

	/* First see if there are any nodes in the area we know
	 * our node will fit into.
	 */
	if (index <= last) {
		lock = std::unique_lock<Allocator>(*this);
		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		uint32_t max_index = last;
		MemNode **ref = &buf[index];
		uint32_t i = index;
		while (*ref == nullptr && i < max_index) {
			ref++;
			i++;
		}

		MemNode *node = nullptr;
		if ((node = *ref) != nullptr) {
			/* If we have found a node and it doesn't have any
			 * nodes waiting in line behind it _and_ we are on
			 * the highest available index, find the new highest
			 * available index
			 */
			if ((*ref = node->next) == nullptr && i >= max_index) {
				do {
					ref--;
					max_index--;
				}
				while (*ref == NULL && max_index > 0);

				last = max_index;
			}

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;

			return node;
		}
	} else if (buf[0]) {
		lock = std::unique_lock<Allocator>(*this);
		/* If we found nothing, seek the sink (at index 0), if
		 * it is not empty.
		 */

		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		MemNode *node = nullptr;
		MemNode **ref = &buf[0];
		while ((node = *ref) != nullptr && index > node->index) {
			ref = &node->next;
		}

		if (node) {
			*ref = node->next;

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;

			return node;
		}
	}

	/* If we haven't got a suitable node, malloc a new one
	 * and initialize it.
	 */
	MemNode *node = nullptr;
	if (mmapPtr) {
		if (mmapCurrent + (index + 1) > mmapMax) {
			auto newMax = allocator_mmap_realloc(mmapdes, mmapPtr, mmapMax, mmapCurrent + index + 1);
			if (!newMax) {
				return nullptr;
			} else {
				mmapMax = newMax;
			}
		}

		node = (MemNode *) ((char *)mmapPtr + mmapCurrent * BOUNDARY_SIZE);
		mmapCurrent += index + 1;

		if (lock.owns_lock()) {
			lock.unlock();
		}
	} else {
		if (lock.owns_lock()) {
			lock.unlock();
		}

		if ((node = (MemNode *)malloc(size)) == nullptr) {
			return nullptr;
		}
	}

	node->next = nullptr;
	node->index = (uint32_t)index;
	node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;
	node->endp = (uint8_t *)node + size;

	return node;
}

void Allocator::free(MemNode *node) {
	MemNode *next, *freelist = nullptr;

	std::unique_lock<Allocator> lock(*this);

	uint32_t max_index = last;
	uint32_t max_free_index = max;
	uint32_t current_free_index = current;

	/* Walk the list of submitted nodes and free them one by one,
	 * shoving them in the right 'size' buckets as we go.
	 */
	do {
		next = node->next;
		uint32_t index = node->index;

		if (max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED && index + 1 > current_free_index) {
			node->next = freelist;
			freelist = node;
		} else if (index < MAX_INDEX) {
			/* Add the node to the appropiate 'size' bucket.  Adjust
			 * the max_index when appropiate.
			 */
			if ((node->next = buf[index]) == nullptr && index > max_index) {
				max_index = index;
			}
			buf[index] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		} else {
			/* This node is too large to keep in a specific size bucket,
			 * just add it to the sink (at index 0).
			 */
			node->next = buf[0];
			buf[0] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		}
	} while ((node = next) != nullptr);

#if DEBUG
	int i = 0;
	auto n = buf[1];
	while (n && i < 1024 * 16) {
		n = n->next;
		++ i;
	}

	if (i >= 1024 * 128) {
		printf("ERRER: pool double-free detected!\n");
		abort();
	}
#endif

	if (lock.owns_lock()) {
		lock.unlock();
	}

	last = max_index;
	current = current_free_index;

	if (!mmapPtr) {
		while (freelist != NULL) {
			node = freelist;
			freelist = node->next;
			::free(node);
		}
	}
}

void Allocator::lock() {
	mutex.lock();
}

void Allocator::unlock() {
	mutex.unlock();
}

}
