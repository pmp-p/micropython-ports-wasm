#!/bin/bash

export EMSDK=/opt/sdk/emsdk-upstream

if echo "$@"|grep -q update
then
WD=$(pwd)

cd "${EMSDK}"

cd "${WD}"
else
    echo not updating sdk in ${EMSDK}
fi

rm rm ~/.emscripten_sanity ~/.emscripten_sanity_wasm
ln -sf "${EM_CONFIG}_sanity_wasm" ~/.emscripten_sanity_wasm

. rebuild.sh "$@"


