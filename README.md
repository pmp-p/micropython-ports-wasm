# MicroPython and Web Assembly (wasm)

### NEWS:

16 august 2019 : retrying stackless with clang CI intregration
 this is using emsdk tot-upstream (tip of tree) not latest-upstream (yet)


10 august 2019 : currently paused because reached somehow limit of emscripten compiler by abusing jump tables
   meanwhile trying new upstream clang with micropython no nlr branch which is better for stackless
https://github.com/pmp-p/micropython/tree/wasm-nonlr/ports/wasm-no-nlr


old working versions always stored in branches.



## What ?

wasm is a virtual machine for internet browsers which run bytecode closer to native speeds
see https://webassembly.org/

but we want to run python virtual machine and its own bytecode ;) so thanks to micropython
we'll be able to by compiling micropython core to wasm bytecode first.



## Requirements

 - Linux OS with gcc/clang, a decent build environnement and libltdl-dev

 - have python3 in your path 3.6 should do it but 3.7 / 3.8 are safer.

 - Follow the instructions for getting started with Emscripten [here](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html).



## Getting started

Beware this is not a micropython fork :
 it's a port folder to add support to official micropython for a new "machine"


You first need to get easy building your own official micropython and its javascript port


Follow the instructions for getting started with micropython unix build

https://github.com/micropython/micropython/


to check if your emscripten build works ( facultative, FYI last test on emscripten 1.38.31 was ok )

https://github.com/micropython/micropython/tree/master/ports/javascript

but i suggest using ```emmake make -C ports/javascript PYTHON=python2``` instead of just make



What are the differences between this repo and the official javascript port?

see https://github.com/pmp-p/micropython-ports-wasm/issues/4


## Dive in !

now you're ready to build the port, you can run

```
# micropython-ports-wasm will go in micropython/ports/wasm
# as it's not a fork but a drop in target port we need to checkout a full microPython

# get the micropython core.

# official ( should always work ! )
git clone --recursive --recurse-submodules https://github.com/micropython/micropython
cd micropython


# or lvgl enabled ( wip could not work )
# git clone --recursive --recurse-submodules https://github.com/littlevgl/lv_micropython.git
# cd lv_micropython
# git checkout lvgl_javascript
# git submodule update --recursive --init
# if you want to use asyncio with lvgl just add the patch https://patch-diff.githubusercontent.com/raw/littlevgl/lv_binding_micropython/pull/30.diff
# that allows you to service with SDL.run_once()

#build host tools
make -C mpy-cross
make -C ports/unix

#add the target port
cd micropython/ports
git clone https://github.com/pmp-p/micropython-ports-wasm.git wasm


cd wasm

#transpile the mixed python/C module to pure C
#use a python3 version with annotations support !
# . modgen.sh

. /path/to/emsdk/emsdk_set_env.sh

# for LVGL support use "./rebuild.sh LVGL=1" instead
# test will be run automatically.
# emmake make USER_C_MODULES=cmod && . runtest.sh

# modgen and make have been merged in rebuild-upsteam.sh script
chmod +x *sh

# avoid using ./rebuild-fastcom.sh
# only there for testing clang 8-10 and binaryen discrepancy

./rebuild-upstream.sh

# fast rebuild
# ./rebuild-upstream.sh fast=1

# using asyncfy (bad perf even on upstream) avoid fast=1 when changing
# mode
# ./rebuild-upstream.sh ASYNCIFY=1

```

now you can navigate http://127.0.0.1:8000/index.html

to edit code samples look in micropython/*.html


## Usage


copy the 4 files located in micropython folder found inside the build folder
 ``pythons.js micropython.js micropython.data micropython.wasm``
and drop them where your main let's say myupython.html file will be

now you have 3 options to run code :

 1) from the web via arguments ( sys.argv ) with a call myupython.html?full_url_to_the_script.py

 2) from a <script type="text/µpython> tag

 3) interactively from repl with xterm.js


for 1&2 you will need to provide a javascript term_impl(text) function that output stdout stream where you want it to go



![Preview1](./docs/runtest.png)



## History:

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



## Experimental Features (current or future):

Using requestIdleCallback
https://developers.google.com/web/updates/2015/08/using-requestidlecallback

multi core interpreters
https://github.com/ericsnowcurrently/multi-core-python/wiki

multiplexed rpc
https://brionv.com/log/2019/05/10/wasm-rpc-thoughts/

a C api for pythons ?
https://github.com/pyhandle/hpy


native code compiler ?
https://github.com/windelbouwman/ppci-mirror


integrated gui
https://github.com/littlevgl/lv_binding_micropython


various shared/distributed memory objects related subjects.
https://github.com/apache/arrow
https://capnproto.org/
http://doc.pypy.org/en/latest/stm.html#python-3-cpython-and-others

mesh network
https://github.com/chr15m/bugout
https://chr15m.github.io/bep44-for-decentralized-applications.html




## goal ( far far away ):


This is a research project aimed toward a cooperative multitasking multicore/multinode python solution.
Though still keeping some level of preemption over coroutines at byte code level for soft-rt purpose.


testing on browser+wasm makes it easier than baremetal.




meet me on  #microPython or #micropython-fr on freenode

https://kiwi.freenode.net/?nick=upy-wasm-guest&channel=#micropython-fr



#

