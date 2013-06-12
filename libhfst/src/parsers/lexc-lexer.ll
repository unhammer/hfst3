%option 8Bit batch nounput noyywrap prefix="hlexc"

%{
//! @file lexc-lexer.cc
//!
//! @brief a lexer for lexc
//!
//! @author Tommi A. Pirinen


//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#if HAVE_CONFIG_H
#  include <config.h>
#endif
#include "lexc-parser.hh"
#include "lexc-utils.h"

#include <assert.h>

extern void hlexcerror(const char *text);

%}

/* c.f. Unicode Standard 5.1 D92 Table 3-7 */
U1 [\x20-\x7e]
U2 [\xc2-\xdf][\x80-\xbf]
U31 \xe0[\xa0-\xbf][\x80-\xbf]
U32 [\xe1-\xec][\x80-\xbf][\x80-\xbf]
U33 \xed[\x80-\x9f][\x80-\xbf]
U34 [\xee-\xef][\x80-\xbf][\x80-\xbf]
U41 \xf0[\x90-\xbf][\x80-\xbf][\x80-\xbf]
U42 [\xf1-\xf3][\x80-\xbf][\x80-\xbf][\x80-\xbf]
U43 \xf4[\x80-\x8f][\x80-\xbf][\x80-\xbf]
/* non US-ASCII */
U8H {U43}|{U42}|{U41}|{U34}|{U33}|{U32}|{U31}|{U2}
/* any UTF-8 */
U8C {U8H}|{U1}
/* Escaped */
EC "%"{U8C}

/* any ASCII */
A7 [\x00-\x7e]
/* special meaning in lexc */
A7RESTRICTED [ <>%!;:""]
/* non-restricted ASCII */
A7UNRESTRICTED [\x21-\x7e]{-}[ <>%!;:""]
/* special meaning in xre */
XRERESTRICTED [- <>%!:;@0~""\\&?$+*/_{}\]\[-]
/* non-restricted ASCII in XRE */
XREUNRESTRICTED [\x21-\x7e\n]{-}[- <>%!:;@0~""\\&?$+*/_{}\]\[-]


/* RegExp. stuff */
XREALPHA {XREUNRESTRICTED}|{U8H}|{EC}
XREOPERATOR [~\\&-/ |+*$_\]\[{}()0?:""]|"@\""|"$."|"$?"|"./."|"<>"|".#."|"."[riul]
XREQUOTSTRING "\""[^""]*"\""
XRECHAR {XREOPERATOR}|{XREALPHA}|{XREQUOTSTRING}
XRETOKEN {XRECHAR}+

/* String entry part:
 * - any unrestricted ASCII-7,
 * - any high unicode codepoint (past U+007F) UTF-8-encoded
 * - any %-escaped UTF-8
 */
STRINGCHAR {A7UNRESTRICTED}|{U8H}|{EC}
STRINGTOKEN {STRINGCHAR}+

/* Lexicon name part */
LEXICONCHAR {A7UNRESTRICTED}|{U8H}|{EC}
LEXICONNAME {LEXICONCHAR}+


/* White space */
WSP [\t ]
LWSP [\r\n\t ]

%x MULTICHARS DEFINITIONS NOFLAGS LEXICONS ENDED
%%


<INITIAL>^{WSP}*("Multichar"|"multichar"|"MULTICHAR")("_"|" ")("symbols"|"Symbols"|"SYMBOLS"){LWSP}+ {
    BEGIN MULTICHARS;
    hfst::lexc::token_update_positions(hlexctext);
    return MULTICHARS_START;
}

<INITIAL,MULTICHARS>^{WSP}*("NOFLAGS"|"NoFlags"|"Noflags"|"noflags"){LWSP}+ {
    BEGIN NOFLAGS;
    hfst::lexc::token_update_positions(hlexctext);
    return NOFLAGS_START;
}

<INITIAL,MULTICHARS,NOFLAGS>^{WSP}*("Definitions"|"Declarations"|"definitions"|"declarations"|"DEFINITIONS"|"DECLARATIONS"){LWSP}+ {
    BEGIN DEFINITIONS;
    hfst::lexc::token_update_positions(hlexctext);
    return DEFINITIONS_START;
}

<INITIAL,MULTICHARS,NOFLAGS,DEFINITIONS>^{WSP}*("LEXICON"){WSP}+{LEXICONNAME} {
    BEGIN LEXICONS;
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "LEXICON",
                                          NULL, true);
    free(lexicon_start);
    return LEXICON_START;
}

<INITIAL,MULTICHARS,NOFLAGS,DEFINITIONS>^{WSP}*("lexicon"){WSP}+{LEXICONNAME} {
    BEGIN LEXICONS;
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "lexicon",
                                          NULL, true);
    free(lexicon_start);
    hlexcerror("Lowercase lexicon parsed as LEXICON");
    return LEXICON_START_WRONG_CASE;
}

<INITIAL,MULTICHARS,NOFLAGS,DEFINITIONS>^{WSP}*("Lexicon"){WSP}+{LEXICONNAME} {
    BEGIN LEXICONS;
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "Lexicon",
                                          NULL, true);
    free(lexicon_start);
    hlexcerror("Titlecase Lexicon parsed as LEXICON");
    return LEXICON_START_WRONG_CASE;
}

<INITIAL,MULTICHARS,NOFLAGS,DEFINITIONS>^{WSP}*("END"|"End"|"end"){LWSP}+ {
    hfst::lexc::token_update_positions(hlexctext);
    return END_START;
}

<INITIAL>!.* {
    hfst::lexc::token_update_positions(hlexctext);
}

<INITIAL>{LWSP} {
    hfst::lexc::token_update_positions(hlexctext);
}

<MULTICHARS>{STRINGTOKEN} {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = hfst::lexc::strip_percents(hlexctext, false);
    return MULTICHAR_SYMBOL;
}

<MULTICHARS>!.* { hfst::lexc::token_update_positions(hlexctext); }

<MULTICHARS>{LWSP} { hfst::lexc::token_update_positions(hlexctext); }

<NOFLAGS>{LEXICONNAME} {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = strdup(hlexctext);
    return LEXICON_NAME;
}

<NOFLAGS>!.* { hfst::lexc::token_update_positions(hlexctext); }

<NOFLAGS>{LWSP} { hfst::lexc::token_update_positions(hlexctext); }

<NOFLAGS>";" { BEGIN INITIAL; }

<DEFINITIONS>{STRINGTOKEN}{WSP}*/"=" {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = hfst::lexc::strstrip(hlexctext);
    return DEFINITION_NAME;
}

<DEFINITIONS>"="{XRETOKEN}";" {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(hlexctext, "=", ";", false);
    return DEFINITION_EXPRESSION;
}

<DEFINITIONS>!.* { hfst::lexc::token_update_positions(hlexctext); }

<DEFINITIONS>{LWSP} { hfst::lexc::token_update_positions(hlexctext); }

<LEXICONS>^{WSP}*"LEXICON"{WSP}+{LEXICONNAME} {
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "LEXICON", 0, true);
    free(lexicon_start);
    return LEXICON_START;
}

<LEXICONS>^{WSP}*"Lexicon"{WSP}+{LEXICONNAME} {
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "Lexicon", 0, true);
    free(lexicon_start);
    hlexcerror("Titlecase Lexicon parsed as LEXICON");
    return LEXICON_START_WRONG_CASE;
}

<LEXICONS>^{WSP}*"lexicon"{WSP}+{LEXICONNAME} {
    hfst::lexc::token_update_positions(hlexctext);
    char* lexicon_start;
    lexicon_start = hfst::lexc::strstrip(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(lexicon_start, "lexicon", 0, true);
    free(lexicon_start);
    hlexcerror("Lowercase lexicon parsed as LEXICON");
    return LEXICON_START_WRONG_CASE;
}

<LEXICONS>{STRINGTOKEN} {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = hfst::lexc::strip_percents(hlexctext, false);
    return ULSTRING;
}

<LEXICONS>"<"{WSP}*{XRETOKEN}{WSP}*">" {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = hfst::lexc::strdup_nonconst_part(hlexctext, "<", ">", false);
    return XEROX_REGEXP;
}

<LEXICONS>{LEXICONNAME}/{WSP}*("\""|";") {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = strdup(hlexctext);
    return LEXICON_NAME;
}

<LEXICONS>"\""[^\n\t""]*"\"" {
    hfst::lexc::token_update_positions(hlexctext);
    hlexclval.name = strdup(hlexctext);
    return ENTRY_GLOSS;
}

<LEXICONS>^{WSP}*"END"{LWSP}+ {
    BEGIN ENDED;
    hfst::lexc::token_update_positions(hlexctext);
    return END_START;
}

<LEXICONS>";" {
    hfst::lexc::token_update_positions(hlexctext);
    return hlexctext[0];
}

<LEXICONS>":" {
    hfst::lexc::token_update_positions(hlexctext);
    return hlexctext[0];
}

<LEXICONS>!.* {
    hfst::lexc::token_update_positions(hlexctext);

}
<LEXICONS>{LWSP} {
    hfst::lexc::token_update_positions(hlexctext);

}

<ENDED>. { 
    hfst::lexc::token_update_positions(hlexctext);

}
<ENDED>{LWSP} {
    hfst::lexc::token_update_positions(hlexctext);
}

<*>[\x80-\xff] {
    hfst::lexc::token_update_positions(hlexctext);
    hlexcerror("Illegal 8-bit sequence (cannot form valid UTF-8)");
    return ERROR;
}

<*>. {
    hfst::lexc::token_update_positions(hlexctext);
    hlexcerror("Syntax error in lexer (no valid token found at the point)");
    return ERROR;
}

%%
