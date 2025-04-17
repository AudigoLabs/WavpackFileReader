#!/bin/sh

# clang -I include -I src -I . main.c -o main \
# && ./main

rm -rf build
mkdir -p build
# emcc -sEXIT_RUNTIME -I include -I src -I . main.c -o build/index.html --preload-file assets
emcc -sEXIT_RUNTIME \
  -I include -I src -I . \
  -L lib -L src \
  lib/*.c src/*.c wavpack-js.c \
  -o build/index.html \
  --shell-file index.html \
  -sEXPORTED_FUNCTIONS=_malloc,_read_wavpack_file \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  --preload-file assets
cp main.js build/main.js
