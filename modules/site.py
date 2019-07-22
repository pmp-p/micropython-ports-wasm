import sys
import embed

for name in ("sys", "embed", "__main__"):
    print("4: module %s was not registered in sys.modules" % name)
    sys.modules[name] = __import__(name)

import gc
import utime as time
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


builtins.asyncio = asyncio
builtins.imp = imp
builtins.gc = gc
builtins.Time = time

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


class RTAwait(Exception):
    pass


builtins.RTAwait = RTAwait


async def awaited_sleep(s):  # awaited_ prefix means we want to be awaited also on standard calls.
    ms = float(s) * 1000
    for step in (0, 1, 2):
        if step == 0:
            yield "'awaited_sleep'"
            continue
        if step == 1:
            try:
                raise RTAwait("async call in sync repl")
            except:
                continue
        if step == 2:
            try:
                yield from asyncio.sleep_ms(ms)
            except:
                continue
    #embed.sleep(5)
    embed.log(" ** should have slept %s s" % s)
    return None

builtins.sleep = awaited_sleep
#builtins.sleep = embed.sleep


def await_(gen):
    try:
        tuple(gen)
    except StopIteration as e:
        return e.args[0]


builtins.await_ = await_

# mask
del await_

# mask
del awaited_sleep


old_sdh = __repl_print__


async def late_repl(argv):
    embed.os_stderr("ASYNC-REPL: %r\n" % argv)
    await argv
    embed.os_stderr("ASYNC-REPL: done")


# async ?
def printer(argv):
    global old_sdh
    if argv is not None:
        if repr(argv).count("'") > 1:
            if repr(argv).split("'", 2)[1].startswith("awaited_"):
                embed.log("comefrom 90")
                for x in argv:
                    embed.log("%r" % x)
                    syscall.stack.append(argv)
                embed.log("goto 85")
    return old_sdh(argv)


builtins.__repl_print__ = printer

# mask
del printer
