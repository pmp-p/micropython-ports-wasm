# MicroPython on Emscripten

Follow the instructions for getting started with Emscripten [here](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html).

Then you can run

```
. /path/to/emsdk/emsdk_set_env.sh
cd /directory/of/micropython//ports
git clone https://github.com/pmp-p/micropython-ports-wasm.git wasm
cd wasm
emmake make && ./runtest.sh
chromium-browser http://127.0.0.1:8000/test.html
```

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

Emscripten
https://github.com/micropython/micropython/issues/888



MicroPython and emscripten
https://www.bountysource.com/issues/5037165-emscripten
https://github.com/micropython/micropython/issues/3474
https://github.com/kkimdev/epsilon
