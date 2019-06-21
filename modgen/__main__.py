# also to look precompiled lib for arduino IDE
# https://arduino.stackexchange.com/questions/57891/how-to-use-a-precompiled-library-in-a-project-with-arduino-ide
# https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5:-Library-specification
# https://github.com/arduino/Arduino/wiki/Library-Manager-FAQ
# Interface for Arduino Libs https://forum.micropython.org/viewtopic.php?t=938
# https://github.com/arduino/arduino-cli
# https://blog.jayway.com/2011/10/08/arduino-on-ubuntu-without-ide/
# https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/debian_ubuntu.md
# https://github.com/plerup/makeEspArduino

import sys,os

from . import *

namespace = 'embed'

SOURCE = os.path.join( os.path.dirname(__file__) , f'{namespace}.pym' )


clines=["""/* http://github.com/pmp-p */"""]


with open(SOURCE,'r') as source:
    pylines, codemap = py2c( namespace, source, clines)
    print("Begin:====================== transpiler ========================")
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

    print("End:====================== transpiler ========================")
    print()
    print()
    print()
    clines.append( table )
    with open(f"mod{namespace}.c",'w') as code:
        for l in clines:
            print(l, file=code)

