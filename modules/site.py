import sys
import embed

for name in ("sys", "embed", "__main__"):
    print("4: module %s was not registered in sys.modules" % name)
    sys.modules[name] = __import__(name)

#embed.os_showloop()
#import m1


import gc
import utime as Time
import micropython
import builtins

# ? builtins.embed = embed

sys.path.clear()
sys.path.append("")
sys.path.append("assets")

import asyncio
import imp
import types

#top level pause/resume handler
builtins.syscall = types.ModuleType('syscall')
sys.modules['syscall'] = syscall
syscall.stack = []


builtins.aio = asyncio
builtins.asyncio = asyncio # almost ... for the non bloat part.
builtins.imp = imp
builtins.gc = gc
builtins.Time = Time

# for js script compat
builtins.true = True
builtins.false = False
builtins.undefined = None
builtins.embed = embed

setattr(__import__("__main__"), "__dict__", globals())


def vars(o=__import__("__main__")):
    try:
        return getattr(o, "__dict__")
    except:
        raise TypeError("vars() argument must have __dict__ attribute")


builtins.vars = vars

# mask
del vars

builtins.undef = object()
if 0:
    def await_(gen):
        return_value = None
        while True:
            try:
                return_value = next(gen)
            except StopIteration:
                break
        return return_value

else:
    def await_(gen):
        nvalue = None
        return_value = None
        while True:
            nvalue = next(gen,undef)
            if nvalue is undef:
                return return_value
            return_value = nvalue

builtins.await_ = await_


def awaited(fn, *argv, **kw):

    def syscall(*argv,**kw):
        return await_(fn(*argv,**kw))

    return syscall

builtins.awaited = awaited

@awaited
def sleep(t):
    start = Time.time() * 1_000_000
    diff = int( t * 1_000_000 )
    stop = start + diff
    while Time.time()*1_000_000< stop:
        yield
    return None


builtins.sleep = sleep
#builtins.sleep = embed.sleep

# mask
del await_

# mask
#del awaited_sleep


displayhook = __repl_print__


