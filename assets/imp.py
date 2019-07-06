
def new_module(module):
    import sys
    save = list(sys.path)
    sys.path.clear()
    sys.path.append('/tmp')
    m = None
    with open('/tmp/%s.py' % module,'w') as empty:
        empty.write('#\n')

    try:
        m = __import__(module)
    except Exception as e :
        sys.print_exception(e)
    finally:
        import sys
        sys.path.clear()
        sys.path.extend(save)
        print("#TODO: os.remove / unlink empty mod")
    return m

def load_module(module, *argv):
    import sys
    m = new_module(module)
    if m:
        import embed
        file = '/assets/%s.py' % module
        mroot = module.split('.')[0]
        m = sys.modules[mroot]
        runf(file, module=embed.vars(m), patch='\n\n__file__=%r\n' % file )
        globals()[mroot] = m
        return m

if not 'dev' in __import__('sys').argv:
    print(""" This can provide a workaround for :\r
    https://github.com/pmp-p/micropython-ports-wasm/issues/5\r
use imp.load_module(modulename) to load modules from /assets/*.py
""")
