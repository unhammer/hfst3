# programs
XFST="/home/eaxelson/xfst/bin/xfst -q"
FOMA="foma -q"
HFST="../../../tools/src/parsers/hfst-xfst2fst -q"
SH="/bin/bash"

# location of hfst tools
tooldir="../../../tools/src/"
# back-end formats used in testing (common format must be listed first as it is used for comparison)    
backend_formats="openfst-tropical sfst foma"
# back-end format used when comparing results
common_format="openfst-tropical" 

# todo: fix and add:
#   DateParser
#   NumbersToNumerals (FinnishNumerals hfst script depends on this)
#   PlusOrMinus

examples="BetterColaMachine BrazilianPortuguese1 BrazilianPortuguese2 EnglishNumerals "\
"EsperantoAdjectives EsperantoNounsAdjectivesAndVerbs EsperantoNounsAndAdjectivesWithTags "\
"EsperantoNounsAndAdjectives EsperantoNouns FinnishOTProsody Lingala "\
"MonishAnalysis MonishGuesserAnalyzer YaleShooting FinnishProsody Palindromes EinsteinsPuzzle"

if ! [ "$1" = "" ]; then
    examples=$1
fi

for example in $examples;
do
    echo "Testing "$example"..."
    
    # (1) If xfst solution exists,
    if ! [ "$example" = "FinnishNumerals" -o \
        "$example" = "FinnishProsody" -o \
        "$example" = "Palindromes" ]; then
        # compile with xfst/foma..
        if [ "$example" = "FinnishOTProsody" -o \
            "$example" = "Lingala" -o \
            "$example" = "YaleShooting" ]; then
            echo "  compiling with foma (result from xfst will be too big).."
            if ! ($FOMA -f xfst-scripts/$example.xfst.script > /dev/null 2> /dev/null); then
                exit 1;
            fi
        else
            echo "  compiling with xfst.."
            if ! ($XFST -f xfst-scripts/$example.xfst.script); then
                exit 1;
            fi
        fi
        # and convert from prolog to openfst-tropical for comparing.
        if ! (cat Result | $tooldir/hfst-txt2fst --prolog -f $common_format > tmp && \
            mv tmp Result_from_xfst); then
            exit 1;
        fi
    fi

    # (2) Compile hfst script with all back-end formats and compare the results..
    for format in $backend_formats; 
    do
        echo "  compiling hfst script with back-end format "$format".."
        if ! ($SH hfst-scripts/$example.hfst.script $format $tooldir); then
            exit 1
        fi
        cat Result | $tooldir/hfst-fst2fst -f $common_format > tmp
        mv tmp Result_from_hfst_script_$format
        
        # with the result from xfst/foma, if available..
        if ! [ "$example" = "FinnishNumerals" -o \
            "$example" = "FinnishProsody" -o \
            "$example" = "Palindromes" ]; then
            # (a special case)
            if [ "$example" = "EinsteinsPuzzle" ]; then
                if ! ($tooldir/hfst-fst2strings --xfst=print-space Result_from_xfst | grep "German coffee Prince fish" > /dev/null); then
                    echo "FAIL"
                fi
                if ! ($tooldir/hfst-fst2strings Result_from_hfst_script_$format | \
                      grep "fish" | grep "German coffee Prince fish" > /dev/null); then
                    echo "FAIL"
                fi
            else 
                if ! ($tooldir/hfst-compare -q Result_from_xfst Result_from_hfst_script_$format); then
                    echo "  FAIL: Results from xfst and hfst scripts ("$format") differ in test "$example", storing results in files:"
                    echo "    log/"$example.result_from_xfst_script_using_xfst_tool 
                    echo "    log/"$example.result_from_hfst_script_using_backend_format_$format
                    if ! [ -d log ]; then
                        mkdir log
                    fi
                    cp Result_from_xfst log/$example.result_from_xfst_script_using_xfst_tool
                    cp Result_from_hfst_script_$format log/$example.result_from_hfst_script_using_backend_format_$format
                fi
            fi
        # or with the result in common format.
        else
            if ! [ "$format" = "$common_format" ]; then
                if ! ($tooldir/hfst-compare -q Result_from_hfst_script_$common_format Result_from_hfst_script_$format); then
                    echo "  FAIL: Results from hfst scripts ("$format" and "$common_format") differ in test "$example
                fi
            fi
        fi
    done

done

rm -f Result_from_xfst Result
for format in $backend_formats;
do 
    rm -f Result_from_hfst_script_$format
done
