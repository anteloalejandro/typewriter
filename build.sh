#!/bin/bash

cd $(dirname $0)

LIBS="-lGL -lm -lpthread -ldl -lrt -lX11 -Lraylib/include/lib -lraylib"
OPTIONS="$@"
DEBUG=1

if test $DEBUG -ne 0; then OPTIONS="${OPTIONS} -ggdb"; fi

mkdir -p bin
gcc main.c -o bin/main $LIBS $OPTIONS
