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

from .utils import extract_root_class

class MemoryStringPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        memVal = extract_root_class(self.val['_mem'])
#        if (int(memVal['_allocator']['pool']) & 1) == 1:
#            return "%s (SVO)" % (memVal['_small']['storage']['_M_elems'][0].address.string())
#        else:
#            return "(in %s) %s" % (memVal['_allocator']['pool'], memVal['_large']['_ptr'])
#        else:
        if (int(memVal['_allocator']['pool']) & 1) == 1:
            return "%s" % (memVal['_small']['storage']['_M_elems'][0].address.string())
        else:
            if int(memVal['_large']['_ptr']) == 0:
                return ""
            else:
                return "%s" % (memVal['_large']['_ptr'].string())

    def display_hint(self):
        return 'string'

class StringViewPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        rootVal = extract_root_class(self.val)
        ptr = rootVal['ptr']
        len = rootVal['len']
        return ptr.string (length = len)

    def display_hint(self):
        return 'string'

def add_string_printers (printer):
    printer.add_printer('memory::basic_string', '^(const )?stappler::memory::basic_string<.+>( &)?$', MemoryStringPrinter)
    printer.add_printer('StringViewBase', '^(const )?stappler::StringViewBase<.+>( &)?$', StringViewPrinter)
    printer.add_printer('StringViewUtf8', '^(const )?stappler::StringViewUtf8( &)?$', StringViewPrinter)
