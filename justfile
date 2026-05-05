cc := "gcc"
chibicc_cflags := "-std=c11 -g -fno-common -Wall -Wno-switch"
my_cflags := "-std=c11 -g -Wall -Wextra -pedantic"
chibicc_dir := "chibicc"
chibicc_srcs := "hashmap parse preprocess strings tokenize type unicode"

_build_objs:
    mkdir -p build/chibicc/
    for name in {{chibicc_srcs}}; do \
        {{cc}} {{chibicc_cflags}} -I{{chibicc_dir}} -c -o build/chibicc/$name.o {{chibicc_dir}}/$name.c; \
    done

libchibicc: _build_objs
    #!/usr/bin/env sh
    objs=$(ls build/chibicc/*.o)
    ar rcs build/libchibicc.a $objs

build: libchibicc
    mkdir -p build/
    {{cc}} {{my_cflags}} -I{{chibicc_dir}} -c -o build/main.o main.c
    {{cc}} -o build/bindgen build/main.o build/libchibicc.a
    cp -r {{chibicc_dir}}/include build/include

run: build
    ./build/bindgen

clean:
    rm -rf build/
