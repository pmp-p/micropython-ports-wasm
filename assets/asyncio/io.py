import sys
import asyncio
import ujson

def step(*jsdata):
    if len(jsdata):
        if not asyncio.io_error:
            try:
                jsdata = ujson.loads(jsdata[0])
                if jsdata:
                    print("io:", jsdata)
            except Exception as e :
                sys.print_exception(e)
    else:
        if not asyncio.failure:
            try:
                asyncio.__auto__()
            except  Exception as e :
                sys.print_exception(e)
                asyncio.failure = True
    return None
