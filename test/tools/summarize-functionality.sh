#!/bin/sh
TOOLDIR=../../tools/src
for i in "" .sfst .ofst .foma; do
    if test -f cat$i ; then
        if ! $TOOLDIR/hfst-summarize cat$i > test.txt ; then
            exit 1
        fi
        rm test.txt;
    fi
done
