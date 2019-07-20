import sys
import builtins
import types

# keep the builtin function accessible in this module and via imp.__import__
__import__ = __import__



# Deprecated since version 3.4: Use types.ModuleType instead.
# but micropython aims toward full 3.4

# Return a new empty module object called name. This object is not inserted in sys.modules.
def new_module(name):
    return types.ModuleType(name)


def getbc(code, ns, file='<code>'):
    live = compile( code, file, 'exec')
    exec( live, ns, ns)
    return live

# execfile is gone in py3 so this one can be whatever.
def execfile(file='', glob=globals(), loc=globals(), patch='', patched_file=''):
    if not file.endswith('.py'):
        file+='.py'

    with open(file,'r') as code:
        getbc(code.read()+patch, glob, patched_file or file)

def load_module(module, *argv):
    m = new_module(module)
    if m:
        import embed
        file = '/assets/%s.py' % module
        mroot = module.split('.')[0]
        m = sys.modules[mroot]
        runf(file, module=embed.vars(m), patch='\n\n__file__=%r\n' % file )
        globals()[mroot] = m
        return m



def reload(mod):
    try:
        import importlib
    except ImportError:
        print("TODO: importlib not found")
        name = mod.__name__
        if sys.modules.get(name,None):
            del sys.modules[name]
        return importer(name)
    return importlib.reload(mod)


try:
    #patched core yeah
    vars
    DBG=0

    # not spaghetti
    def importer(name,*argv,**kw):
        global __import__
        try:
            return __import__(name,*argv)
        except ImportError:
            pass

        file = ':{0}.py'.format(name)
        if DBG:print("trying to go online for",file)
        # todo open the file via open() or raise importerror
        try:
            code = open(file,'r').read()
        except:
            raise ImportError('module not found')

        #build a empty module
        mod = types.ModuleType(name)

        mod.__file__ = file

        # compile module from cached file
        try:
            code = compile( code, file, 'exec')
        except Exception as e:
            sys.print_exception(e)
            raise

        # execute it in its own empty namespace.
        ns = vars(mod)

        try:
            exec( code, ns, ns)
        except Exception as e:
            sys.print_exception(e)
            raise

        # though micropython would normally insert module before executing the whole body
        # do it after.
        sys.modules[name] = mod
        return mod


except:
    DBG=0
    #wasm port or javascript port on standard core.
    try:
        import embed
        builtins.vars = embed.vars
        print("wasm/javascript: vars() available for modules namespace")
    except:
        print("Found vanilla Micropython core : some imports may not work")
        print("  -> use imp.ort(name);import name instead")

    # for pivot module, only required because import is strange on µpy.
    pivot_code = ""
    pivot_name = ""
    pivot_file = ""

    # spaghetti with hot sauce.
    def ort(name, host=__import__('__main__')):
        global __import__, pivot_name, pivot_file, pivot_code
        name = name.strip().split('#')[0]
        if name.count(' '):
            print("import %s => ' as ' not supported, go ask for fixing import" % name)
            name = name.split(' ')[0]


        if sys.modules.get(name,None) is None:
            try:
                __import__(name)
            except:
                print("\n#FIXME handle /__init__.py correctly instead of relying on import bugs")
                print("  reason => other ports could fix them !")
                print(host.__name__, "imp.ort %s" % name,end=' ')
                pivot_file = ':%s.py' % name.replace('.','/')
                with open(pivot_file,'r') as f:
                    pivot_name = name
                    pivot_code = f.read()
                # the nearly empty pivot module will provide globals() itself to exec so vars not required
                import imp_pivot
                sys.modules[name]=sys.modules.pop('imp_pivot')
        return sys.modules.get(name,None)


    def importer(name,*argv,**kw):
        global __import__
        if name=='syscall':
            print("<async syscall>",end='')
        elif not name in sys.modules:
            print("importing ",name)
        return __import__(name, *argv,**kw)


# install hook
builtins.__import__ = importer
print("__import__ is now", importer)
