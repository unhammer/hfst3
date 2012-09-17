#!/bin/sh
TOOLDIR=../../tools/src
for i in "" .sfst .ofst .foma; do
    if test -f cat2dog$i ; then
        if ! $TOOLDIR/hfst-invert cat2dog$i > test ; then
            exit 1
        fi
        if ! $TOOLDIR/hfst-compare -s test dog2cat$i  ; then
            exit 1
        fi
        rm test;
    fi
done
