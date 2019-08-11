import sys
if not 'dev' in sys.argv:
    print(sys.implementation.name,'%s.%s.%s' % sys.implementation.version, sys.version, sys.platform)
sys.path.clear()
sys.path.append( '' )
sys.path.append( 'assets' )
import asyncio
import imp
main = __import__('__main__')
import builtins
builtins.asyncio = asyncio
builtins.imp = imp
