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
