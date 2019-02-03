
import gdb

from os.path import dirname
sys.path.append(dirname(__file__))

from libstappler.v2 import register_printers
register_printers(gdb.current_objfile())
