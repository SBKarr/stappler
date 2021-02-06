set print static-members off

set auto-load safe-path /

source ../../gdb/printers.py

python
import sys 
sys.path.insert(0, '/mingw64/share/gcc-10.2.0/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
end

set print pretty on
set print object on
set print vtbl on
set print demangle on
set demangle-style gnu-v3
set print sevenbit-strings off
