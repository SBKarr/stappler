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

namespace stappler::mempool::custom {

void AllocManager::reset(void *p) {
	memset(this, 0, sizeof(AllocManager));
	pool = p;
}

void *AllocManager::alloc(size_t &sizeInBytes, AllocFn allocFn) {
	if (buffered) {
		MemAddr *c, **lastp;

		c = buffered;
		lastp = &buffered;
		while (c) {
			if (c->size > sizeInBytes * 2) {
				break;
			} else if (c->size >= sizeInBytes) {
				*lastp = c->next;
				c->next = free_buffered;
				free_buffered = c;
				sizeInBytes = c->size;
				increment_return(sizeInBytes);
				return c->address;
			}

			lastp = &c->next;
			c = c->next;
		}
	}
	increment_alloc(sizeInBytes);
	return allocFn(pool, sizeInBytes);
}

void AllocManager::free(void *ptr, size_t sizeInBytes, AllocFn allocFn) {
	MemAddr *addr = nullptr;
	if (allocated == 0) {
		return;
	}

	if (free_buffered) {
		addr = free_buffered;
		free_buffered = addr->next;
	} else {
		addr = (MemAddr *)allocFn(pool, sizeof(MemAddr));
		increment_alloc(sizeof(MemAddr));
	}

	if (addr) {
		addr->size = sizeInBytes;
		addr->address = ptr;
		addr->next = nullptr;

		if (buffered) {
			MemAddr *c, **lastp;

			c = buffered;
			lastp = &buffered;
			while (c) {
				if (c->size >= sizeInBytes) {
					addr->next = c;
					*lastp = addr;
					break;
				}

				lastp = &c->next;
				c = c->next;
			}

			if (!addr->next) {
				*lastp = addr;
			}
		} else {
			buffered = addr;
			addr->next = nullptr;
		}
	}
}

void MemNode::insert(MemNode *point) {
	this->ref = point->ref;
	*this->ref = this;
	this->next = point;
	point->ref = &this->next;
}

void MemNode::remove() {
	*this->ref = this->next;
	this->next->ref = this->ref;
}

size_t MemNode::free_space() const {
	return endp - first_avail;
}

void Cleanup::run(Cleanup **cref) {
	Cleanup *c = *cref;
	while (c) {
		*cref = c->next;
		if (c->fn) {
			(*c->fn)((void *)c->data);
		}
		c = *cref;
	}
}

}
