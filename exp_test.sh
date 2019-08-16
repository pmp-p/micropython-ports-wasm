reset;
export EMSDK=/opt/sdk/emsdk-upstream

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
rm bugrepro.js
if echo $@|grep -q ASYNCIFY
then
    emcc -s WASM=1 -o bugrepro.js bugrepro.c -DASYNCIFY=1 -s ASYNCIFY
else
    emcc -s WASM=1 -o bugrepro.js bugrepro.c
fi
node bugrepro.js
