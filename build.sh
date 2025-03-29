#!/bin/bash

cd $(dirname $0)

LIBS="-lm -Lraylib/lib -lraylib"
FILES=$(ls src/*)
OPTIONS="-Wall -Werror -Wextra $@"
DEBUG=1

if test $DEBUG -ne 0; then OPTIONS="${OPTIONS} -ggdb"; fi

mkdir -p bin
gcc main.c -o bin/main $FILES $LIBS $OPTIONS
