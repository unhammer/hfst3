#!/bin/sh
if test -f cat2dogol ; then
    for c in 1 256 65536 16777216 ; do
        if ! yes cat | head -n $c | ../../tools/src/hfst-proc-apertium-proc cat2dogol > /dev/null ; then
            exit 1
        fi
    done
fi
