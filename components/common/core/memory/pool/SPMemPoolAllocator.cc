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

namespace stappler::mempool::base {

static std::atomic<size_t> s_mappedRegions = 0;

size_t get_mapped_regions_count() {
	return s_mappedRegions.load();
}

void *sp_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	auto ret = ::mmap(addr, length, prot, flags, fd, offset);
	if (ret && ret != MAP_FAILED) {
		++ s_mappedRegions;
		return ret;
	}
	return ret;
}

int sp_munmap(void *addr, size_t length) {
	auto ret = ::munmap(addr, length);
	if (ret == 0) {
		-- s_mappedRegions;
	}
	return ret;
}

}

namespace stappler::mempool::custom {

static std::atomic<size_t> s_nAllocators = 0;

#if LINUX
static uint32_t allocator_mmap_realloc(int filedes, void *ptr, uint32_t idx, uint32_t required) {
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

	base::sp_munmap((char *)ptr + oldSize, newSize - oldSize);
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
}

bool Allocator::run_mmap(uint32_t idx) {
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

	void *reserveMem = base::sp_mmap(NULL, ALLOCATOR_MMAP_RESERVED, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	// Now the file is ready to be mmapped.
	void *map = base::sp_mmap(reserveMem, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED | MAP_NORESERVE, mmapdes, 0);
	if (map == MAP_FAILED) {
		close(mmapdes);
		perror("Error mmapping the file");
		return false;
	}

	mmapPtr = map;
	mmapMax = idx;
	return true;
}

#endif

size_t Allocator::getAllocatorsCount() {
	return s_nAllocators.load();
}

Allocator::Allocator(bool threadSafe) {
	++ s_nAllocators;
	buf.fill(nullptr);

	if (threadSafe) {
		mutex = new AllocMutex;
	}
}

Allocator::~Allocator() {
	MemNode *node, **ref;

	if (mutex) {
		delete mutex;
	}

#if LINUX
	if (mmapPtr) {
		base::sp_munmap(mmapPtr, mmapMax * BOUNDARY_SIZE);
		close(mmapdes);
		return;
	}
#endif

	for (uint32_t index = 0; index < MAX_INDEX; index++) {
		ref = &buf[index];
		while ((node = *ref) != nullptr) {
			*ref = node->next;
			allocated -= node->endp - (uint8_t *)node;
			::free(node);
		}
	}

	-- s_nAllocators;
}

void Allocator::set_max(uint32_t size) {
	std::unique_lock<Allocator> lock(*this);

	uint32_t max_free_index = uint32_t(SPALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX);
	current += max_free_index;
	current -= max;
	max = max_free_index;
	if (current > max) {
		current = max;
	}
}

MemNode *Allocator::alloc(uint32_t in_size) {
	std::unique_lock<Allocator> lock;

	uint32_t size = uint32_t(SPALIGN(in_size + SIZEOF_MEMNODE, BOUNDARY_SIZE));
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
	lock = std::unique_lock<Allocator>(*this);
	if (index <= last) {
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
#if LINUX
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
		allocated += size;
		if (allocationTracker) {
			allocationTracker(node, size);
		}
	}
#else
	if (lock.owns_lock()) {
		lock.unlock();
	}

	if ((node = (MemNode *)malloc(size)) == nullptr) {
		return nullptr;
	}
	allocated += size;
	if (allocationTracker) {
		allocationTracker(node, size);
	}
#endif

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

	last = max_index;
	current = current_free_index;

	if (lock.owns_lock()) {
		lock.unlock();
	}

#if LINUX
	if (mmapPtr) {
		return;
	}
#endif

	while (freelist != NULL) {
		node = freelist;
		freelist = node->next;
		allocated -= node->endp - (uint8_t *)node;
		::free(node);
	}
}

void Allocator::lock() {
	if (mutex) {
		mutex->lock();
	}
}

void Allocator::unlock() {
	if (mutex) {
		mutex->unlock();
	}
}

}
