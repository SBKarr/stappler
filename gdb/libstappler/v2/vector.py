# Copyright (C) 2019 Roman Katuntsev <sbkarr@stappler.org>

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from .utils import Iterator, extract_root_class, make_string_value

class MemoryVectorPrinter(object):
    class _iterator(Iterator):
        def __init__ (self, start, finish):
            self.item = start
            self.finish = finish
            self.count = 0

        def __iter__(self):
            return self

        def __next__(self):
            count = self.count
            self.count = self.count + 1
            if self.item == self.finish:
                raise StopIteration
            elt = self.item.dereference()
            self.item = self.item + 1
            return ('[%d]' % count, elt)

    def __init__(self, val):
        self.val = val

    def to_string(self):
        memVal = extract_root_class(self.val['_mem'])
        memValRoot = extract_root_class(memVal)
        if memValRoot:
            return "(in %s) vector (l) of size %s, capacity %s" % (
                memVal['_allocator']['pool'], memValRoot['_used'], memValRoot['_allocated'])
        else:
            if (int(memVal['_allocator']['pool']) & 1) == 1:
                size = 23 / self.val.type.template_argument(0).sizeof
                return "(in %s) vector (s) of size %d (svo for %d)" % (
                    memVal['_allocator']['pool'], size - int(memVal['_small']['storage']['_M_elems'][23]), size)
            else:
                return "(in %s) vector (s) of size %s, capacity %s" % (
                    memVal['_allocator']['pool'], memVal['_large']['_used'], memVal['_large']['_allocated'])

    def children(self):
        memVal = extract_root_class(self.val['_mem'])
        memValRoot = extract_root_class(memVal)
        if memValRoot:
            return self._iterator(memValRoot['_ptr'], memValRoot['_ptr'] + int(memValRoot['_used']))
        else:
            if (int(memVal['_allocator']['pool']) & 1) == 0:
                return self._iterator(memVal['_large']['_ptr'], memVal['_large']['_ptr'] + int(memVal['_large']['_used']))
            else:
                ptrType = memVal['_large']['_ptr'].type
                storagePtr = memVal['_small']['storage']['_M_elems'][0].address.cast(ptrType)
                capacity = 23 / self.val.type.template_argument(0).sizeof
                size = capacity - int(memVal['_small']['storage']['_M_elems'][23])
                return self._iterator(storagePtr, storagePtr + int(size))

    def display_hint(self):
        return 'array'

class MemoryDictPrinter(object):
    class _iterator(Iterator):
        def __init__ (self, start, finish):
            self.item = start
            self.finish = finish
            self.count = 0

        def __iter__(self):
            return self

        def __next__(self):
            count = self.count
            self.count = self.count + 1
            if self.item == self.finish:
                raise StopIteration
            elt = self.item.dereference()
            self.item = self.item + 1
            if elt.type.template_argument(0).name.startswith("stappler::memory::basic_string"):
                return (make_string_value(elt['first']), elt['second'])
            else:
                return ('[%d] % count', elt)

    def __init__(self, val):
        self.val = val

    def to_string(self):
        memVal = extract_root_class(self.val['_data'])
        memValRoot = extract_root_class(memVal)
        if memValRoot:
            return "(in %s) dict (l) of size %s, capacity %s" % (
                memVal['_allocator']['pool'], memValRoot['_used'], memValRoot['_allocated'])
        else:
            if (int(memVal['_allocator']['pool']) & 1) == 1:
                size = 23 / self.val.type.template_argument(0).sizeof
                return "(in %s) dict (s) of size %d (svo for %d)" % (
                    memVal['_allocator']['pool'], size - int(memVal['_small']['storage']['_M_elems'][23]), size)
            else:
                return "(in %s) dict (s) of size %s, capacity %s" % (
                    memVal['_allocator']['pool'], memVal['_large']['_used'], memVal['_large']['_allocated'])

    def children(self):
        memVal = extract_root_class(self.val['_data'])
        memValRoot = extract_root_class(memVal)
        if memValRoot:
            return self._iterator(memValRoot['_ptr'], memValRoot['_ptr'] + int(memValRoot['_used']))
        else:
            if (int(memVal['_allocator']['pool']) & 1) == 0:
                return self._iterator(memVal['_large']['_ptr'], memVal['_large']['_ptr'] + int(memVal['_large']['_used']))
            else:
                ptrType = memVal['_large']['_ptr'].type
                storagePtr = memVal['_small']['storage']['_M_elems'][0].address.cast(ptrType)
                capacity = 23 / self.val.type.template_argument(0).sizeof
                size = capacity - int(memVal['_small']['storage']['_M_elems'][23])
                return self._iterator(storagePtr, storagePtr + int(size))

    def display_hint(self):
        if self.val.type.template_argument(0).name.startswith("stappler::memory::basic_string"):
            return 'map'
        else:
            return 'array'

def add_vector_printers (printer):
    printer.add_printer('memory::vector', '^(const )?stappler::memory::vector<.*>( &)?$', MemoryVectorPrinter)
    printer.add_printer('memory::dict', '^(const )?stappler::memory::dict<.*>( &)?$', MemoryDictPrinter)
