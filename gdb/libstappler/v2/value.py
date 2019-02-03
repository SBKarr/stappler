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

class DataValuePrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        type = int(self.val['_type'])
        if type == 0:
            return "(null)"
        elif type == 1:
            return "(int) %d" % (int(self.val['intVal']))
        elif type == 2:
            return "(float) %s" % (float(self.val['doubleVal']))
        elif type == 3:
            return "(bool) %s" % ("true" if bool(self.val['boolVal']) else "false")
        elif type == 4:
            return "(string)"
        elif type == 5:
            return "(bytes)"
        elif type == 6:
            return "(array)"
        elif type == 7:
            return "(dict)"
        elif type == 255:
            return "(NONE)"
        return "(unknown)"

    def children(self):
        type = int(self.val['_type'])
        if type == 0: yield "null", gdb.Value(0)
        elif type == 1: yield "intVal", self.val['intVal']
        elif type == 2: yield "doubleVal", self.val['doubleVal']
        elif type == 3: yield "boolVal", self.val['boolVal']
        elif type == 4: yield "strVal", self.val['strVal'].dereference()
        elif type == 5: yield "bytesVal", self.val['bytesVal'].dereference()
        elif type == 6: yield "arrayVal", self.val['arrayVal'].dereference()
        elif type == 7: yield "dictVal", self.val['dictVal'].dereference()
        elif type == 255: yield "NONE", gdb.Value(0)

def add_value_printers (printer):
    printer.add_printer('data::Value', '^(const )?stappler::data::ValueTemplate<.*>( &)?$', DataValuePrinter)
