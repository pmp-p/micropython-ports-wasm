import sys
import asyncio
import ujson

q = {}
req = []

DBG=0
WARNED = 0

def finalize():
    global q,req
    okdel = []
    for key in q:
        if not key in req:
            okdel.append( key )

    while len(okdel):
        key = okdel.pop()
        q.pop(key)

def step(*jsdata):
    global q
    if not asyncio.io_error and len(jsdata):
        try:
            jsdata =  ujson.loads(jsdata[0])
            #if DBG and jsdata: print("\n << IO=",jsdata)
            q.update( jsdata )
        except Exception as e :
            sys.print_exception(e)
            asyncio.io_error = True

        # try to prevent leaks
        if len(q)>100:
            finalize()
        # or fail
        if not WARNED and len(q) > 200:
            print("asyncio.io.q is getting huge, do you need some discard ?")
            asyncio.io_error = True
            asyncio.failure = True

    elif not asyncio.failure:
        try:
            asyncio.__auto__()
        except  Exception as e :
            sys.print_exception(e)
            asyncio.failure = True
    return None
