import gdb.printing
import itertools
import re
import sys
from datetime import datetime, timedelta

if sys.version_info[0] > 2:
	### Python 3 stuff
	Iterator = object
	# Python 3 folds these into the normal functions.
	imap = map
	izip = zip
	# Also, int subsumes long
	long = int
else:
	### Python 2 stuff
	class Iterator:
		"""Compatibility mixin for iterators

		Instead of writing next() methods for iterators, write
		__next__() methods and use this mixin to make them work in
		Python 2 as well as Python 3.

		Idea stolen from the "six" documentation:
		<http://pythonhosted.org/six/#six.Iterator>
		"""

		def next(self):
			return self.__next__()

	# In Python 2, we still need these from itertools
	from itertools import imap, izip

def extract_root_class(val):
	for field in val.type.fields():
		if field.is_base_class and len(field.type.fields()) != 0:
			return val[field]
	return None

class MemoryStringPrinter(object):
	def __init__(self, val):
		self.val = val

	def to_string(self):
		memVal = extract_root_class(self.val['_mem'])
#		if (int(memVal['_allocator']['pool']) & 1) == 1:
#			return "%s (SVO)" % (memVal['_small']['storage']['_M_elems'][0].address.string())
#		else:
#			return "(in %s) %s" % (memVal['_allocator']['pool'], memVal['_large']['_ptr'])
#		else:
		if (int(memVal['_allocator']['pool']) & 1) == 1:
			return "%s" % (memVal['_small']['storage']['_M_elems'][0].address.string())
		else:
			return "%s" % (memVal['_large']['_ptr'])

	def display_hint(self):
		return 'string'

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
				return "(in %s) vector (s) (Small Value Ooptimized)" % (memVal['_allocator']['pool'])
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
				return self._iterator(storagePtr, storagePtr + int(1))

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
			return ('[%d]' % count, elt)

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
				return "(in %s) dict (s) (Small Value Ooptimized)" % (memVal['_allocator']['pool'])
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
				return self._iterator(storagePtr, storagePtr + int(1))

	def display_hint(self):
		return 'array'

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

def build_pretty_printer():
	pp = gdb.printing.RegexpCollectionPrettyPrinter("stappler")
	pp.add_printer('memory::basic_string', '^(const )?stappler::memory::basic_string<.+>( &)?$', MemoryStringPrinter)
	pp.add_printer('memory::vector', '^(const )?stappler::memory::vector<.*>( &)?$', MemoryVectorPrinter)
	pp.add_printer('memory::dict', '^(const )?stappler::memory::dict<.*>( &)?$', MemoryDictPrinter)
	pp.add_printer('Time', '^(const )?stappler::Time( &)?$', TimePrinter)
	pp.add_printer('TimeInterval', '^(const )?stappler::TimeInterval( &)?$', TimeIntervalPrinter)
	pp.add_printer('data::Value', '^(const )?stappler::data::ValueTemplate<.*>( &)?$', DataValuePrinter)

	pp.add_printer('StringViewBase', '^(const )?stappler::StringViewBase<.+>( &)?$', StringViewPrinter)
	pp.add_printer('StringViewUtf8', '^(const )?stappler::StringViewUtf8( &)?$', StringViewPrinter)
	return pp

def extract_allocator_pointer(val):
	ptr = val['pool']
	if ptr.type.code == gdb.TYPE_CODE_PTR:
		if not ptr: return 'Null'
		else: return str(ptr)
	return 'Null'


gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer(), True)
