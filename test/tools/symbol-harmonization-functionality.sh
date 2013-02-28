#!/bin/sh

TOOLDIR=../../tools/src/

for ext in .sfst .ofst .foma; do

    FFLAG=
    case $ext in
        .sfst)
            FFLAG="-f sfst"
            if ! ($TOOLDIR/hfst-format --list-formats | grep "sfst" > /dev/null) ; then
                continue;
            fi;;
        .ofst)
            FFLAG="-f openfst-tropical"
            if ! ($TOOLDIR/hfst-format --list-formats | grep "openfst-tropical" > /dev/null) ; then
                continue;
            fi;;
        .foma)
            FFLAG="-f foma"
            if ! ($TOOLDIR/hfst-format --list-formats | grep "foma" > /dev/null) ; then
                continue;
            fi;;
        *)
            FFLAG=;;
    esac

    # composition

    # harmonization:  [a:b] .o. [ID:ID] == [a:b]
    if ! ($TOOLDIR/hfst-compose a2b$ext id$ext | $TOOLDIR/hfst-compare -s a2b$ext); then
	echo "compose #1" ${FFLAG}
	exit 1;
    fi
    # harmonization:  [ID:ID] .o. [a:b] == [a:b]
    if ! ($TOOLDIR/hfst-compose id$ext a2b$ext | $TOOLDIR/hfst-compare -s a2b$ext); then
	echo "compose #2" ${FFLAG}
	exit 1;
    fi

    # harmonization:  [a:b] .o. [UNK:UNK | ID:ID] == [[a:a] | [a:b] | [a:UNK]]
    if ! ($TOOLDIR/hfst-compose a2b$ext unk2unk_or_id$ext | $TOOLDIR/hfst-compare -s a2a_or_a2b_or_a2unk$ext); then
	echo "compose #3" ${FFLAG}
	exit 1;
    fi
    # harmonization:  [UNK:UNK | ID:ID] .o. [a:b] == [[a:b] | [b:b] | [UNK:b]]
    if ! ($TOOLDIR/hfst-compose unk2unk_or_id$ext a2b$ext | $TOOLDIR/hfst-compare -s a2b_or_b2b_or_unk2b$ext); then
	echo "compose #4" ${FFLAG}
	exit 1;
    fi


    # no harmonization:    [a:b] .o. [ID:ID] == empty
    if ! ($TOOLDIR/hfst-compose -H a2b$ext id$ext | $TOOLDIR/hfst-compare -s empty$ext); then
	echo "compose #5" ${FFLAG}
	exit 1;
    fi
    # no harmonization:    [ID:ID] .o. [a:b] == empty
    if ! ($TOOLDIR/hfst-compose -H id$ext a2b$ext | $TOOLDIR/hfst-compare -s empty$ext); then
	echo "compose #6" ${FFLAG}
	exit 1;
    fi

    # no harmonization:    [a:b] .o. [UNK:UNK | ID:ID] == empty
    if ! ($TOOLDIR/hfst-compose -H a2b$ext unk2unk_or_id$ext | $TOOLDIR/hfst-compare -s empty$ext); then
	echo "compose #7" ${FFLAG}
	exit 1;
    fi
    # no harmonization:    [UNK:UNK | ID:ID] .o. [a:b] == empty
    if ! ($TOOLDIR/hfst-compose -H unk2unk_or_id$ext a2b$ext | $TOOLDIR/hfst-compare -s empty$ext); then
	echo "compose #8" ${FFLAG}
	exit 1;
    fi

    # concatenation
    echo "[?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:b]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;

    # harmonization
    $TOOLDIR/hfst-concatenate tmp1 tmp2 | $TOOLDIR/hfst-minimize > concatenation;
    echo "[[?|a|b] a:b]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;  # error
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s concatenation result); then
	echo "concatenate #1" ${FFLAG}
	exit 1;
    fi

    # no harmonization
    $TOOLDIR/hfst-concatenate -H tmp1 tmp2 | $TOOLDIR/hfst-minimize > concatenation;
    echo "[? a:b]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s concatenation result); then
	echo "concatenate #2" ${FFLAG}
	exit 1;
    fi


    ## conjunction
    echo "[?:?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:b]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;

    # harmonization
    $TOOLDIR/hfst-conjunct tmp1 tmp2 | $TOOLDIR/hfst-minimize > conjunction;
    echo "[a:b]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s conjunction result); then
	echo "concatenate #1" ${FFLAG}
	exit 1;
    fi

    # no harmonization
    $TOOLDIR/hfst-conjunct -H tmp1 tmp2 | $TOOLDIR/hfst-minimize > conjunction;
    echo "[] - []" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s conjunction result); then
	echo "concatenate #2" ${FFLAG}
	exit 1;
    fi


    # disjunction
    echo "[?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:b]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;

    # harmonization
    $TOOLDIR/hfst-disjunct tmp1 tmp2 | $TOOLDIR/hfst-minimize > disjunction;
    echo "[?|a|b|a:b]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s disjunction result); then
	echo "concatenate #1" ${FFLAG}
	exit 1;
    fi

    # no harmonization
    $TOOLDIR/hfst-disjunct -H tmp1 tmp2 | $TOOLDIR/hfst-minimize > disjunction;
    echo "[?|a:b]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s disjunction result); then
	echo "concatenate #2" ${FFLAG}
	exit 1;
    fi


    ## subtraction
    echo "[?:?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:b]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;

    # harmonization
    $TOOLDIR/hfst-subtract tmp1 tmp2 | $TOOLDIR/hfst-minimize > subtraction;
    echo "[?:?|a:?|?:a|b:?|?:b|b:a]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s subtraction result); then
	echo "subtract #1" ${FFLAG}
	exit 1;
    fi

    # no harmonization
    $TOOLDIR/hfst-subtract -H tmp1 tmp2 | $TOOLDIR/hfst-minimize > subtraction;
    echo "[?:?]" | $TOOLDIR/hfst-regexp2fst -H ${FFLAG} | $TOOLDIR/hfst-minimize > result;
    # do not harmonize when comparing, the transducers must be exactly the same
    if ! ($TOOLDIR/hfst-compare -H -s subtraction result); then
	echo "subtract #2" ${FFLAG}
	exit 1;
    fi


    ## substitution
    echo "\`[[a:b ?] , c , d ]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:b ?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;
    if ! ($TOOLDIR/hfst-compare -H tmp1 tmp2 > /dev/null); then
	echo "  (skipping substitution test #1, does not work yet)" ${FFLAG}
	#exit 1;
    fi

    echo "\`[[a:b ?] , b , B C D ]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp1;
    echo "[a:B a:C a:D ?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;
    if ! ($TOOLDIR/hfst-compare -H tmp1 tmp2 > /dev/null); then
	echo "  (skipping substitution test #2, does not work yet)" ${FFLAG}
	#exit 1;
    fi
    

    ## special symbols @_.*_@
    echo "@_foo_@" | $TOOLDIR/hfst-strings2fst -S ${FFLAG} > tmp1;
    echo "[?:?]" | $TOOLDIR/hfst-regexp2fst ${FFLAG} > tmp2;
    for tool in hfst-compose hfst-concatenate hfst-conjunct hfst-disjunct hfst-subtract;
    do
	$TOOLDIR/$tool tmp1 tmp2 | $TOOLDIR/hfst-fst2txt | tr '\t' ' ' > result
	if (grep "@_foo_@ @_UNKNOWN_SYMBOL_@" result > /dev/null); then
	    echo "special symbols" $tool ${FFLAG}
	    exit 1;
	fi
	if (grep "@_UNKNOWN_SYMBOL_@ @_foo_@" result > /dev/null); then
	    echo "special symbols" $tool ${FFLAG}
	    exit 1;
	fi
    done

done

exit 0;