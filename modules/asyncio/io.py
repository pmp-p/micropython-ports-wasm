import sys
import asyncio
import ujson

q = {}


def step(*jsdata):
    global q
    if not asyncio.io_error and len(jsdata):
        try:
            q.update( ujson.loads(jsdata[0]) )
        except Exception as e :
            sys.print_exception(e)
            asyncio.io_error = True
    elif not asyncio.failure:
        try:
            asyncio.__auto__()
        except  Exception as e :
            sys.print_exception(e)
            asyncio.failure = True
    return None
