import sys
import os
import glob


from . import *

namespace = 'embed'

#SOURCE = os.path.join( os.path.dirname(__file__) , f'{namespace}.pym' )

USER_C_MODULES = os.environ.get('USER_C_MODULES','cmod')

for pym in glob.glob( os.path.join(USER_C_MODULES,'*.pym')):

    clines=["""/* http://github.com/pmp-p */"""]


    with open(pym,'r') as source:
        pylines, codemap = py2c( namespace, source, clines)
        print(f"Begin:====================== transpiling [{pym}] ========================")
        for l in pylines:
            print(l)



        bytecode = compile( '\n'.join(pylines), '<modgen>', 'exec')
        exec(bytecode,  __import__('__main__').__dict__, globals())

        cmap = cify( module(), namespace)
        table = cmap.pop(-1)

        print("== code map ==")
        while len(codemap):
            defname, prepend, append = codemap.pop()
            code,rti = cmap.pop( defname )
            print(defname,'pre=', prepend,'post=', append, len( code) )
            clines.insert(append, rti )
            clines.insert(prepend, code )

        mod_dir = f"{USER_C_MODULES}/{namespace}"
        os.makedirs(mod_dir, exist_ok=True)
        with open(f"{mod_dir}/micropython.mk","w") as makefile:
            makefile.write(f"""
{namespace.upper()}_MOD_DIR := $(USERMOD_DIR)

# Add all C files to SRC_USERMOD.
SRC_USERMOD += $({namespace.upper()}_MOD_DIR)/mod{namespace}.c

# add module folder to include paths if needed
CFLAGS_USERMOD += -I$({namespace.upper()}_MOD_DIR)
""")



        ctarget = f"{mod_dir}/mod{namespace}.c"
        print(f"End:====================== transpiled [{ctarget}] ========================")
        print()
        print()
        clines.append( table )

        with open(ctarget,'w') as code:
            for l in clines:
                print(l, file=code)

