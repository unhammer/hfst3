#!/bin/sh

# A script for copying winopenfst and foma backends and libhfst/src
# in a directory given as first argument for native windows compilation.

if [ -d "$1" ]; then
    echo "Directory $1 exists"
    exit 1
else
    mkdir $1
fi

mkdir $1/back-ends

# foma back-end
mkdir $1/back-ends/foma/

for file in \
apply.c coaccessible.c constructions.c \
define.c determinize.c dynarray.c \
extract.c flags.c int_stack.c io.c \
lex.cmatrix.c lex.yy.c mem.c minimize.c \
regex.c reverse.c rewrite.c sigma.c \
spelling.c stringhash.c structures.c \
topsort.c trie.c utf8.c foma.h fomalib.h \
fomalibconf.h regex.h;
do 
    cp back-ends/foma/$file $1/back-ends/foma/;
done

# openfstwin back-end
mkdir $1/back-ends/openfstwin
mkdir $1/back-ends/openfstwin/src
mkdir $1/back-ends/openfstwin/src/include
mkdir $1/back-ends/openfstwin/src/include/fst
mkdir $1/back-ends/openfstwin/src/lib

for file in \
compat flags fst properties symbol-table \
symbol-table-ops util;
do
    cp back-ends/openfstwin/src/lib/$file.cc $1/back-ends/openfstwin/src/lib/$file.cpp 
done

cp back-ends/openfstwin/src/include/fst/*.h $1/back-ends/openfstwin/src/include/fst/

# libhfst/src and subdirectories
mkdir $1/libhfst
mkdir $1/libhfst/src
mkdir $1/libhfst/src/implementations
mkdir $1/libhfst/src/implementations/compose_intersect
mkdir $1/libhfst/src/implementations/optimized-lookup
mkdir $1/libhfst/src/parsers

# libhfst/src without subdirectories
for file in \
FormatSpecifiers.h HarmonizeUnknownAndIdentitySymbols.h \
HfstDataTypes.h HfstEpsilonHandler.h HfstExceptionDefs.h \
HfstExceptions.h HfstExtractStrings.h HfstFlagDiacritics.h \
HfstInputStream.h HfstLookupFlagDiacritics.h HfstOutputStream.h \
HfstSymbolDefs.h HfstTokenizer.h HfstTransducer.h HfstXeroxRules.h \
hfst.h hfst.hpp.in hfst_apply_schemas.h;
do
    cp libhfst/src/$file $1/libhfst/src/
done

for file in \
HarmonizeUnknownAndIdentitySymbols HfstApply HfstDataTypes \
HfstEpsilonHandler HfstExceptionDefs HfstExceptions HfstFlagDiacritics \
HfstInputStream HfstLookupFlagDiacritics HfstOutputStream HfstRules \
HfstSymbolDefs HfstTokenizer HfstTransducer HfstXeroxRules \
HfstXeroxRulesTest;
do
    cp libhfst/src/$file.cc $1/libhfst/src/$file.cpp
done

# libhfst/src/implementations without subdirectories
for file in \
ConvertTransducerFormat.h FomaTransducer.h HfstFastTransitionData.h \
HfstOlTransducer.h HfstTransition.h HfstTransitionGraph.h \
HfstTropicalTransducerTransitionData.h LogWeightTransducer.h \
TropicalWeightTransducer.h;
do
    cp libhfst/src/implementations/$file $1/libhfst/src/implementations/
done

for file in \
ConvertFomaTransducer ConvertLogWeightTransducer ConvertOlTransducer \
ConvertTransducerFormat ConvertTropicalWeightTransducer FomaTransducer \
HfstOlTransducer HfstTransitionGraph HfstTropicalTransducerTransitionData \
LogWeightTransducer TropicalWeightTransducer;
do
    cp libhfst/src/implementations/$file.cc $1/libhfst/src/implementations/$file.cpp
done

# implementations/compose_intersect
for file in \
ComposeIntersectFst ComposeIntersectLexicon ComposeIntersectRule \
ComposeIntersectRulePair ComposeIntersectUtilities;
do
    cp libhfst/src/implementations/compose_intersect/$file.cc \
        $1/libhfst/src/implementations/compose_intersect/$file.cpp
    cp libhfst/src/implementations/compose_intersect/$file.h \
        $1/libhfst/src/implementations/compose_intersect/$file.h
done

# implementations/optimized-lookup
for file in \
convert pmatch transducer;
do
    cp libhfst/src/implementations/optimized-lookup/$file.cc \
        $1/libhfst/src/implementations/optimized-lookup/$file.cpp
    cp libhfst/src/implementations/optimized-lookup/$file.h \
        $1/libhfst/src/implementations/optimized-lookup/$file.h
done
for file in \
find_epsilon_loops ospell;
do
    cp libhfst/src/implementations/optimized-lookup/$file.cc \
        $1/libhfst/src/implementations/optimized-lookup/$file.cpp
done

# parsers
for file in \
LexcCompiler PmatchCompiler XreCompiler \
lexc-utils pmatch_utils xre_utils;
do
    cp libhfst/src/parsers/$file.cc \
        $1/libhfst/src/parsers/$file.cpp
    cp libhfst/src/parsers/$file.h \
        $1/libhfst/src/parsers/$file.h
done
for file in \
lexc-lexer pmatch_lex xre_lex;
do
    cp libhfst/src/parsers/$file.cc \
        $1/libhfst/src/parsers/$file.cpp
done
for file in \
lexc-parser pmatch_parse xre_parse;
do
    cp libhfst/src/parsers/$file.cc \
        $1/libhfst/src/parsers/$file.cpp
    cp libhfst/src/parsers/$file.hh \
        $1/libhfst/src/parsers/$file.hh
done

# make scripts and headers
cp scripts/make-foma.bat $1/back-ends/foma/
cp scripts/make-openfstwin.bat $1/back-ends/openfstwin/src/lib/
# cp scripts/make-parsers.bat $1/libhfst/src/parsers/
# cp scripts/make-implementations.bat $1/libhfst/src/implementations/
cp scripts/make-libhfst.bat $1/libhfst/src/
cp scripts/generate-python-bindings.bat $1/libhfst/src

# copy missing headers and change some headers included
cp scripts/stdint.h $1/back-ends/foma/
cp scripts/inttypes.h $1/back-ends/foma/
# unistd.h is included in some generated files
for file in \
$1/libhfst/src/parsers/pmatch_lex.cpp \
$1/libhfst/src/parsers/xre_lex.cpp \
$1/libhfst/src/parsers/lexc-lexer.cpp \
$1/back-ends/foma/lex.cmatrix.c;
do
    sed -i 's/#include <unistd.h>/#include <io.h>/' $file
done

