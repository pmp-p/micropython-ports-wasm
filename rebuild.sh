#!/bin/bash

if echo $0|grep -q bash
then
    echo not to be sourced, use chmod +x on script
else
    reset
    export EM_CONFIG="${EMSDK}/EM_CONFIG"

    rm ~/.emscripten
    ln -sf "${EM_CONFIG}" ~/.emscripten

    if cmp "${EM_CONFIG}" ~/.emscripten
    then
        echo ok cache
    else
        echo "# OMG WHY ???"
        cat "${EM_CONFIG}" > ~/.emscripten
    fi

    WD="$(pwd)"
    cd "${EMSDK}"

    tmpfile=`mktemp` || exit 1
    ./emsdk construct_env $tmpfile
    echo  $tmpfile
    . $tmpfile
    rm -f $tmpfile

    cd "${WD}"
    export PATH
    export EM_CACHE="${EMSDK}/EM_CACHE"
    export EM_PORTS="${EMSDK}/EM_PORTS"

    mkdir -p "${EM_CACHE}" "${EM_PORTS}"

    env|grep ^EM
    echo $PATH

    export PYTHONDONTWRITEBYTECODE=1
    for py in 8 7 6
    do
        if which python3.${py}
        then
            export PYTHON=python3.${py}
            break
        fi
    done
    echo Will use python $PYTHON
    echo


    if cd $(dirname $(realpath "$0") )
    then
        if echo "$@"|grep -q fast
        then
                rm -vf libmicropython.* build/main.o
        else
            echo "Cleaning up ( use 'fast' arg to to partial cleanup only )"
            emmake make clean
        fi

        echo
        echo Building ...

        export USER_C_MODULES=cmod
        $PYTHON -u -B -m modgen
        $PYTHON -u -B -m fstrings_helper micropython/link.cpy > micropython/ulink.py


# WASM_FILE_API=1 \

        #export EMCC_FORCE_STDLIBS=libc,libc++abi,libc++,libdlmalloc
        export MOD="-DMODULE_EMBED_ENABLED=1 -DMODULE_EXAMPLE_ENABLED=1 -DMODULE_LVGL_ENABLED=1"

        if emmake make \
 JSFLAGS="-s USE_SDL=2" \
 USER_C_MODULES=${USER_C_MODULES} \
 CFLAGS_EXTRA="${MOD}" \
 FROZEN_MPY_DIR=modules \
 FROZEN_DIR=flash \
 "$@"
        then
            . runtest.sh
        fi
    fi
fi
