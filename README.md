# MicroPython on Emscripten

What are the differences between this repo and the official javascript port?

see https://github.com/pmp-p/micropython-ports-wasm/issues/4

Follow the instructions for getting started with Emscripten [here](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html).

Then you can run

```
# micropython-ports-wasm will go in micropython/ports/wasm

git clone --recursive https://github.com/micropython/micropython

cd micropython/ports
git clone https://github.com/pmp-p/micropython-ports-wasm.git wasm

cd wasm

. /path/to/emsdk/emsdk_set_env.sh
emmake make && ./runtest.sh
```

now you can navigate http://127.0.0.1:8000/index.html

to edit code samples look in micropython/*.html


# Usage


copy the 4 files located in micropython folder found inside the build folder
 ``pythons.js micropython.js micropython.data micropython.wasm``
and drop them where your main let's say myupython.html file will be

now you have 3 options to run code :

 1) from the web via arguments ( sys.argv ) with a call myupython.html?full_url_to_the_script.py

 2) from a <script type="text/µpython> tag

 3) interactively from repl with xterm.js


for 1&2 you will need to provide a javascript term_impl(text) function that output stdout stream where you want it to go



![Preview1](./docs/runtest.png)


meet me on  #microPython or #micropython-fr on freenode

https://kiwi.freenode.net/?nick=upy-wasm-guest&channel=#micropython-fr





## history:

codebase
https://github.com/matthewelse/micropython

Micropython webassembly target
https://github.com/micropython/micropython/issues/3313

Support for Emscripten
https://github.com/micropython/micropython/pull/2618

RFC: emscripten: Got something to compile and link.
https://github.com/micropython/micropython/pull/1561

Javascript Port - MicroPython transmuted into Javascript by Emscripten.
https://github.com/micropython/micropython/pull/3575

Emscripten

https://github.com/micropython/micropython/issues/888


MicroPython and emscripten

https://www.bountysource.com/issues/5037165-emscripten

https://github.com/micropython/micropython/issues/3474

https://github.com/kkimdev/epsilon

