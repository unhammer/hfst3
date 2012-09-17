#!/bin/bash
TOOLDIR=../../tools/src
old_opts="$HFST_OPTIONS"
export HFST_OPTIONS="--version"
for f in $TOOLDIR/hfst-{compare,compose,compose-intersect,concatenate,disjunct,duplicate,fst2fst,fst2strings,fst2txt,head,invert,lexc2fst,lookup,minimize,name,pair-test,project,push-weights,regexp2fst,remove-epsilons,repeat,reverse,split,strings2fst,substitute,subtract,summarize,tail,txt2fst} ; do
    if [ -x "$f" -a ! -d "$f" ] ; then
        if !  "$f" > version.out ; then
            rm version.out
            echo $f has broken version
            exit 1
        fi
        if ! grep -m 1 'hfst-[^ ]\+ [0-9.]\+' version.out > /dev/null \
        ; then
            rm version.out
            echo $f has malformed version
            exit 1
        fi
    fi
done
rm version.out
HFST_OPTIONS="$old_opts"
