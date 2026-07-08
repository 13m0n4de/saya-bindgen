set dotenv-load

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
    ar rcs build/libchibicc.a build/chibicc/*.o

build: libchibicc
    mkdir -p build/

    {{cc}} {{my_cflags}} -I{{chibicc_dir}} -c -o build/shim.o shim.c
    $SAYA chibicc.saya -o build/chibicc.ssa -t build/chibicc.td -N chibicc
    $SAYA main.saya -o build/main.ssa -M chibicc=build/chibicc.td

    qbe build/chibicc.ssa -o build/chibicc.s
    qbe build/main.ssa -o build/main.s

    {{cc}} -c build/chibicc.s -o build/chibicc.o
    {{cc}} -c build/main.s -o build/main.o
    {{cc}} -o build/bindgen build/main.o build/chibicc.o build/shim.o build/libchibicc.a

    cp -r {{chibicc_dir}}/include build/include

build-c: libchibicc
    mkdir -p build/

    {{cc}} {{my_cflags}} -I{{chibicc_dir}} -c -o build/main.o main.c
    {{cc}} -o build/bindgen-c build/main.o build/libchibicc.a

    cp -r {{chibicc_dir}}/include build/include

run: build
    ./build/bindgen

run-c: build-c
    ./build/bindgen-c

clean:
    rm -rf build/
