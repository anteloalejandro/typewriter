#!/bin/bash

LIBS="-lGL -lm -lpthread -ldl -lrt -lX11 -Lraylib/include/lib -lraylib"
OPTIONS=""
DEBUG=1

if test $DEBUG -ne 0; then OPTIONS="${OPTIONS} -ggdb"; fi

gcc main.c -o bin/main $LIBS $OPTIONS
