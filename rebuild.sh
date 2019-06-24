#!/bin/bash
if echo $0|grep -q bash
then
    echo not to be sourced, use chmod +x on script
else
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

    if cd $(dirname $(realpath "$0") )
    then
        rm libmicropython.* build/main.o

        $PYTHON -u -B -mmodgen
        $PYTHON -u -B -m fstrings_helper micropython/link.cpy > assets/ulink.py

        if emmake make USER_C_MODULES=cmod CFLAGS_EXTRA=-DMODULE_EXAMPLE_ENABLED=1 "$@"
        then
            . runtest.sh
        fi
    fi
fi
