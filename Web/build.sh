#!/bin/sh

# clang -I include -I src -I . main.c -o main \
# && ./main

rm -rf build
mkdir -p build
# emcc -sEXIT_RUNTIME -I include -I src -I . main.c -o build/index.html --preload-file assets
sources_path=../Sources/CWavPackFilereader
emcc -sEXIT_RUNTIME \
  -I $sources_path/include -I $sources_path/src -I $sources_path -I . \
  -L $sources_path/lib -L $sources_path/src \
  $sources_path/lib/*.c $sources_path/src/*.c wavpack-js.c \
  -o build/index.html \
  --shell-file index.html \
  -sEXPORTED_FUNCTIONS=_malloc,_read_wavpack_file \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  --preload-file assets
cp main.js build/main.js
