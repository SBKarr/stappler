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

class TimePrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        epoch = datetime(1970, 1, 1)
        rootVal = extract_root_class(self.val)
        cookie_datetime = epoch + timedelta(microseconds=int(rootVal['_value']))
        return "(time) %d \"%s\"" % (int(rootVal['_value']), cookie_datetime)

class TimeIntervalPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        epoch = datetime(1, 1, 1)
        rootVal = extract_root_class(self.val)
        return "(interval) %d \"%s\"" % (int(rootVal['_value']), timedelta(microseconds=int(rootVal['_value'])))

def add_time_printers (printer):
    printer.add_printer('Time', '^(const )?stappler::Time( &)?$', TimePrinter)
    printer.add_printer('TimeInterval', '^(const )?stappler::TimeInterval( &)?$', TimeIntervalPrinter)
