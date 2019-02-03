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

from .utils import Iterator, increment_tree_node

class MemorySetPrinter(object):
    class _iterator(Iterator):
        def __init__ (self, start, finish, valtype):
            self.item = start
            self.finish = finish
            self.count = 0
            self.valtype = valtype

        def __iter__(self):
            return self

        def __next__(self):
            count = self.count
            self.count = self.count + 1
            if self.item == self.finish:
                raise StopIteration
            storageAddr = self.item.cast(self.valtype).dereference()['value']['_storage'].address
            elt = storageAddr.cast(self.valtype.target().template_argument(0).pointer()).dereference()
            self.item = increment_tree_node(self.item)
            return ('[%d]' % count, elt)

    def __init__(self, val):
        self.val = val

    def to_string(self):
        tree = self.val['_tree']
        return "(in %s) set of size %s" % (tree['_allocator']['pool'], tree['_size'])

    def children(self):
        tree = self.val['_tree']
        left = int(tree['_header']['left'])
        if left == 0:
            return self._iterator(tree['_header'].address, tree['_header'].address, tree['_tmp'].type)
        else:
            return self._iterator(tree['_header']['parent'], tree['_header'].address, tree['_tmp'].type)

class MemoryMapPrinter(object):
    class _iterator(Iterator):
        def __init__ (self, start, finish, valtype):
            self.item = start
            self.finish = finish
            self.count = 0
            self.valtype = valtype

        def __iter__(self):
            return self

        def __next__(self):
            count = self.count
            self.count = self.count + 1
            if self.item == self.finish:
                raise StopIteration
            storageAddr = self.item.cast(self.valtype).dereference()['value']['_storage'].address
            elt = storageAddr.cast(self.valtype.target().template_argument(0).pointer()).dereference()
            self.item = increment_tree_node(self.item)
            return ('[%d]' % count, elt)

    def __init__(self, val):
        self.val = val

    def to_string(self):
        tree = self.val['_tree']
        return "(in %s) set of size %s" % (tree['_allocator']['pool'], tree['_size'])

    def children(self):
        tree = self.val['_tree']
        left = int(tree['_header']['left'])
        if left == 0:
            return self._iterator(tree['_header'].address, tree['_header'].address, tree['_tmp'].type)
        else:
            return self._iterator(tree['_header']['parent'], tree['_header'].address, tree['_tmp'].type)

def add_tree_printers (printer):
    printer.add_printer('memory::set', '^(const )?stappler::memory::set<.*>( &)?$', MemorySetPrinter)
    printer.add_printer('memory::map', '^(const )?stappler::memory::map<.*>( &)?$', MemoryMapPrinter)
