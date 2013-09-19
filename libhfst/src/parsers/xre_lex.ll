%option 8Bit batch noyylineno noyywrap nounput reentrant bison-bridge prefix="xre"

%{


#include <string.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstXeroxRules.h"

#include "xre_parse.hh"
#include "xre_utils.h"

#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)   (retval = hfst::xre::getinput(buf, maxlen))

// These variablese are used when scanning a regex for a given SYMBOL
// when performing variable substitution in function definition.
namespace hfst { 
  namespace xre {
    extern unsigned int cr; // number of characters read
    extern std::set<unsigned int> positions; // positions of a given SYMBOL
    extern char * position_symbol;  // the given SYMBOL
} }

// a macro that increments the number of characters read
#define CR hfst::xre::cr += (unsigned int)strlen(yytext)

extern int xrelex ( YYSTYPE * lvalp, yyscan_t scanner );

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
/* special meaning in xre */
A7RESTRICTED [- |<>%!,^:;@0~\\&?$+*/_(){}\]\[-]
/* non-restricted ASCII */
A7UNRESTRICTED [\x21-\x7e]{-}[- |<>%!,^:;@0~\\&?$+*/_(){}\]\[-]

WEIGHT [0-9]+(\.[0-9]+)?

/* token character */
NAME_CH {A7UNRESTRICTED}|{U8H}|{EC}
UINTEGER [1-9][0-9]*
INTEGER -?[1-9][0-9]*
WSP [\t ]
LWSP [\t\r\n ]

/* curly brackets */
BRACED      [{]([^}]|[\300-\337].|[\340-\357]..|[\360-\367]...)+[}]
%%




{BRACED} {
  CR;
  yylval->label = hfst::xre::strip_curly(yytext);
  return CURLY_BRACKETS;
}



"~"   { CR; return COMPLEMENT; }
"\\"  { CR; return TERM_COMPLEMENT; }
"&"   { CR; return INTERSECTION; }
"-"   { CR; return MINUS; }

"$."  { CR; return CONTAINMENT_ONCE; }
"$?"  { CR; return CONTAINMENT_OPT; }
"$"   { CR; return CONTAINMENT; }

"+"   { CR; return PLUS; }
"*"   { CR; return STAR; }

"./." { CR; return IGNORE_INTERNALLY; }
"/"   { CR; return IGNORING; }

"|"   { CR; return UNION; }

"<>"  { CR; return SHUFFLE; }
"<"   { CR; return BEFORE; }
">"   { CR; return AFTER; }

".o." { CR; return COMPOSITION; }
".O." { CR; return LENIENT_COMPOSITION; }
".x." { CR; return CROSS_PRODUCT; }
".P." { CR; return UPPER_PRIORITY_UNION; }
".p." { CR; return LOWER_PRIORITY_UNION; }
".-u." { CR; return UPPER_MINUS; }
".-l." { CR; return LOWER_MINUS; }
"`" {   CR; return SUBSTITUTE_LEFT; }

"\\<=" { CR; return LEFT_RESTRICTION; }
"<=>" { CR; return LEFT_RIGHT_ARROW; }
"<=" { CR; return LEFT_ARROW; }
"=>" { CR; return RIGHT_ARROW; }

"->" { CR; return REPLACE_RIGHT; }
"(->)" { CR; return OPTIONAL_REPLACE_RIGHT; }
"<-" { CR; return REPLACE_LEFT; }
"(<-)" { CR; return OPTIONAL_REPLACE_LEFT; }
"<->" { CR; return REPLACE_LEFT_RIGHT; }
"(<->)" { CR; return OPTIONAL_REPLACE_LEFT_RIGHT; }
"@->" { CR; return LTR_LONGEST_MATCH; }
"@>" { CR; return LTR_SHORTEST_MATCH; }
"->@" { CR; return RTL_LONGEST_MATCH; }
">@" { CR; return RTL_SHORTEST_MATCH; }

"||" { CR; return REPLACE_CONTEXT_UU; }
"//" { CR; return REPLACE_CONTEXT_LU; }
"\\\\" { CR; return REPLACE_CONTEXT_UL; }
"\\//" { CR; return REPLACE_CONTEXT_LL; }
"_"+ { CR; return CENTER_MARKER; }
"..."+ { CR; return MARKUP_MARKER; }

"\\\\\\" { CR; return LEFT_QUOTIENT; }

"^"{UINTEGER}","{UINTEGER} { 
    CR;
    yylval->values = hfst::xre::get_n_to_k(yytext);
    return CATENATE_N_TO_K;
}

"^{"{UINTEGER}","{UINTEGER}"}" {
    CR;
    yylval->values = hfst::xre::get_n_to_k(yytext);
    return CATENATE_N_TO_K;
}

"^>"{UINTEGER} { 
    CR;
    yylval->value = strtol(yytext + 2, 0, 10);
    return CATENATE_N_PLUS; 
}

"^<"{UINTEGER} { 
    CR;
    yylval->value = strtol(yytext + 2, 0, 10);
    return CATENATE_N_MINUS;
}

"^"{UINTEGER}                  { 
    CR;
    yylval->value = strtol(yytext + 1, 0, 10);
    return CATENATE_N;
}

".r" { CR; return REVERSE; }
".i" { CR; return INVERT; }
".u" { CR; return UPPER; }
".l" { CR; return LOWER; }

"@bin\""[^""]+"\""|"@\""[^""]+"\"" { 
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_BIN;
}

"@txt\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_TEXT;
}

"@stxt\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_SPACED;
}

"@pl\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_PROLOG;
}

"@re\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_RE;
}

"[." { CR; return LEFT_BRACKET_DOTTED; }
".]" { CR; return RIGHT_BRACKET_DOTTED; }
"[" { CR; return LEFT_BRACKET; }
"]" { CR; return RIGHT_BRACKET; }
"(" { CR; return LEFT_PARENTHESIS; }
")" { CR; return RIGHT_PARENTHESIS; }


{LWSP}":"{LWSP} { CR; return PAIR_SEPARATOR_SOLE; }
^":"$ { CR; return PAIR_SEPARATOR_SOLE; }
{LWSP}":" { CR; return PAIR_SEPARATOR_WO_LEFT; }
":"{LWSP} { CR; return PAIR_SEPARATOR_WO_RIGHT; }
":" { CR; return PAIR_SEPARATOR; }

"::"{WEIGHT} {
    CR; 
    yylval->weight = hfst::xre::get_weight(yytext + 2);
    return WEIGHT;
}

"\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::parse_quoted(yytext);
    return QUOTED_LITERAL;
}


",," { CR; return COMMACOMMA; }
"," { CR; return COMMA; }

"\"\"" { CR; return EPSILON_TOKEN; }
"0" { CR; return EPSILON_TOKEN; }
"[]" { CR; return EPSILON_TOKEN; }
"?" { CR; return ANY_TOKEN; }

"0"({NAME_CH}|"0")+ {
    if (hfst::xre::position_symbol != NULL) {
      if (strcmp(hfst::xre::position_symbol, yytext) == 0) {
        hfst::xre::positions.insert(hfst::xre::cr);
      }
    }
    CR;
    yylval->label = hfst::xre::strip_percents(yytext);
    return SYMBOL;
}  

{NAME_CH}({NAME_CH}|"0")* {
    if (hfst::xre::position_symbol != NULL) {
      if (strcmp(hfst::xre::position_symbol, yytext) == 0) {
        hfst::xre::positions.insert(hfst::xre::cr);
      }
    }
    CR;
    yylval->label = hfst::xre::strip_percents(yytext);
    return SYMBOL;
}  

{NAME_CH}({NAME_CH}|"0")*"(" {
    CR;
    yylval->label = yytext;
    return FUNCTION_NAME;
}

"0"({NAME_CH}|"0")+"(" {
    CR;
    yylval->label = yytext;
    return FUNCTION_NAME;
}

";\t"{WEIGHT} {
    CR; 
    yylval->weight = hfst::xre::get_weight(yytext + 2);
    return END_OF_WEIGHTED_EXPRESSION;
}

";"" "+{WEIGHT} {
    CR; 
    yylval->weight = hfst::xre::get_weight(yytext + 2);
    return END_OF_WEIGHTED_EXPRESSION;
}

";" { 
    CR; 
    return END_OF_EXPRESSION;
}

{LWSP}* { CR; /*fprintf(stderr, "ignoring whitespace '%s'..\n", yytext); */ /* ignorable whitespace */ }

("!"|"#")[^\n]*$ { CR; /* fprintf(stderr, "ignoring comment '%s'..\n", yytext); */ /* ignore comments */ }

("!"|"#")[^\n]* { CR; /* fprintf(stderr, "ignoring comment '%s'..\n", yytext); */ /* ignore comments */ }

. { 
    CR; 
    return LEXER_ERROR;
}

%%
