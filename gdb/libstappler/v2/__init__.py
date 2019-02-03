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
import gdb.printing

def register_printers(obj):
    stappler_printer = gdb.printing.RegexpCollectionPrettyPrinter("stappler.v2")

    from .string import add_string_printers
    add_string_printers(stappler_printer)

    from .tree import add_tree_printers
    add_tree_printers(stappler_printer)

    from .vector import add_vector_printers
    add_vector_printers(stappler_printer)
    
    from .value import add_value_printers
    add_value_printers(stappler_printer)

    from .time import add_time_printers
    add_time_printers(stappler_printer)

    gdb.printing.register_pretty_printer(obj, stappler_printer)
