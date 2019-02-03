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

import gdb
import itertools
import re
import sys

if sys.version_info[0] > 2:
    Iterator = object
    imap = map
    izip = zip
    long = int
else:
    class Iterator:
        def next(self):
            return self.__next__()
    from itertools import imap, izip

def extract_root_class(val):
    for field in val.type.fields():
        if field.is_base_class and len(field.type.fields()) != 0:
            return val[field]
    return None

def increment_tree_node(c):
    if int(c.dereference()['right']) != 0:
        # move right one step, then left until end
        ret = c.dereference()['right']
        while ret.dereference()['left']:
            ret = ret.dereference()['left']
        return ret
    else:
        if int(c.dereference()['parent']) != 0:
            if int(c.dereference()['parent'].dereference()['left']) == int(c):
                return c.dereference()['parent']
            else:
                ret = c
                while int(ret.dereference()['parent']) != 0 and int(ret.dereference()['parent'].dereference()['right']) == int(ret):
                    ret = ret.dereference()['parent'];
                if int(ret.dereference()['parent']) == 0:
                    raise StopIteration
                else:
                    ret = ret.dereference()['parent'];
                return ret
        else:
            raise StopIteration
    return c

def make_string_value(val):
    memVal = extract_root_class(val['_mem'])
    if (int(memVal['_allocator']['pool']) & 1) == 1:
        return (memVal['_small']['storage']['_M_elems'][0].address.string())
    else:
        return memVal['_large']['_ptr'].string(memVal['_large']['_used'])
