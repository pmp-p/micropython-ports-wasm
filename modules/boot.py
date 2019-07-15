import sys
import gc
import utime as time
import micropython
if not 'dev' in sys.argv:
    print(sys.implementation.name,'%s.%s.%s' % sys.implementation.version, sys.version, sys.platform)
sys.path.clear()
sys.path.append( '' )
sys.path.append( 'assets' )
import asyncio
import imp
import builtins
builtins.asyncio = asyncio
builtins.imp = imp
builtins.gc = gc
builtins.Time = time
