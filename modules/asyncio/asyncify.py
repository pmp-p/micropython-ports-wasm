import embed
import imp

def convert():
    code = []

    for line in embed.os_read().decode().split('\n'):
        if line.strip().startswith('#'):
            code.append(line)
        else:
            code.append('    '+line)

    #remove the shebang with the async block start
    code[0]='async def asyncode():'
    code = '\n'.join(code)

    main = __import__('__main__')

    code = imp.getbc( code , vars(main) , file='<asyncify>')

    asyncio.task( main.__dict__.get('asyncode') )


