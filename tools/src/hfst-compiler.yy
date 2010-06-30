%{
/*******************************************************************/
/*                                                                 */
/*  FILE     fst-compiler.yy                                       */
/*  MODULE   fst-compiler                                          */
/*  PROGRAM  SFST                                                  */
/*  AUTHOR   Helmut Schmid, IMS, University of Stuttgart           */
/*                                                                 */
/*******************************************************************/

#include <stdio.h>

#include "../../libhfst/src/HfstCompiler.h"
//#include "make-compact.h"
using namespace hfst;
using std::cerr;

extern int  yylineno;
extern char *yytext;

void yyerror(char *text);
void warn(char *text);
void warn2(char *text, char *text2);
int yylex( void );
int yyparse( void );

hfst::ImplementationType type= SFST_TYPE;

static int Switch=0;
HfstTransducer * Result;
%}

/* Slight Hfst addition SFST::... */
%union {
  int        number;
  HfstCompiler::Twol_Type  type;
  HfstCompiler::Repl_Type  rtype;
  char       *name;
  char       *value;
  unsigned char uchar;
  unsigned int  longchar;
  HfstCompiler::Character  character;
  HfstTransducer   *expression;
  HfstCompiler::Range      *range;
  HfstCompiler::Ranges     *ranges;
  HfstCompiler::Contexts   *contexts;
}

%token <number> NEWLINE ALPHA COMPOSE PRINT POS INSERT SWITCH
%token <type>   ARROW
%token <rtype>  REPLACE
%token <name>   SYMBOL VAR SVAR RVAR RSVAR
%token <value>  STRING STRING2 UTF8CHAR
%token <uchar>  CHARACTER

%type  <uchar>      SCHAR
%type  <longchar>   LCHAR
%type  <character>  CODE
%type  <expression> RE
%type  <range>      RANGE VALUE VALUES
%type  <ranges>     RANGES
%type  <contexts>   CONTEXT CONTEXT2 CONTEXTS CONTEXTS2

%left PRINT INSERT
%left ARROW REPLACE
%left COMPOSE
%left '|'
%left '-'
%left '&'
%left SEQ
%left '!' '^' '_'
%left '*' '+'
%%

ALL:        ASSIGNMENTS RE NEWLINES { Result=HfstCompiler::result($2, Switch); }
          ;

ASSIGNMENTS: ASSIGNMENTS ASSIGNMENT {}
          | ASSIGNMENTS NEWLINE     {}
          | /* nothing */           {}
          ;

ASSIGNMENT: // VAR '=' RE              { if (def_var($1,$3)) warn2("assignment of empty transducer to",$1); }
          // | RVAR '=' RE             { if (def_rvar($1,$3)) warn2("assignment of empty transducer to",$1); }
          // | SVAR '=' VALUES         { if (def_svar($1,$3)) warn2("assignment of empty symbol range to",$1); }
          // | RSVAR '=' VALUES        { if (def_svar($1,$3)) warn2("assignment of empty symbol range to",$1); }
          // | RE PRINT STRING         { write_to_file($1, $3); }
          ALPHA RE                { HfstCompiler::def_alphabet($2); }
          ;

RE:       //   RE ARROW CONTEXTS2      { $$ = restriction($1,$2,$3,0); }
	  // | RE '^' ARROW CONTEXTS2  { $$ = restriction($1,$3,$4,1); }
	  // | RE '_' ARROW CONTEXTS2  { $$ = restriction($1,$3,$4,-1); }
          // | RE REPLACE CONTEXT2     { $$ = replace_in_context(minimise(explode($1)),$2,$3,false); }
          // | RE REPLACE '?' CONTEXT2 { $$ = replace_in_context(minimise(explode($1)),$2,$4,true);}
          // | RE REPLACE '(' ')'      { $$ = replace(minimise(explode($1)), $2, false); }
          // | RE REPLACE '?' '(' ')'  { $$ = replace(minimise(explode($1)), $2, true); }
          // | RE RANGE ARROW RANGE RE { $$ = make_rule($1,$2,$3,$4,$5); }
          // | RE RANGE ARROW RANGE    { $$ = make_rule($1,$2,$3,$4,NULL); }
          // | RANGE ARROW RANGE RE    { $$ = make_rule(NULL,$1,$2,$3,$4); }
          // | RANGE ARROW RANGE       { $$ = make_rule(NULL,$1,$2,$3,NULL); }
          // | RE COMPOSE RE    { $$ = composition($1, $3); }
          '{' RANGES '}' ':' '{' RANGES '}' { $$ = HfstCompiler::make_mapping($2,$6,type); }
          | RANGE ':' '{' RANGES '}' { $$ = HfstCompiler::make_mapping(add_range($1,NULL),$4,type); }
          | '{' RANGES '}' ':' RANGE { $$ = HfstCompiler::make_mapping($2,add_range($5,NULL),type); }
          // | RE INSERT CODE ':' CODE  { $$ = freely_insert($1, $3, $5); }
          // | RE INSERT CODE           { $$ = freely_insert($1, $3, $3); }
          | RANGE ':' RANGE  { $$ = HfstCompiler::new_transducer($1,$3,type); }
          | RANGE            { $$ = HfstCompiler::new_transducer($1,$1,type); }
          // | VAR              { $$ = var_value($1); }
          // | RVAR             { $$ = rvar_value($1); }
          // | RE '*'           { $$ = repeat_star($1); }
          // | RE '+'           { $$ = repeat_plus($1); }
          // | RE '?'           { $$ = optionalize($1); }
          // | RE RE %prec SEQ  { $$ = concatenate($1, $2); delete $2; }
          // | '!' RE           { $$ = negation($2); }
          // | SWITCH RE        { $$ = invert($2); }
          // | '^' RE           { $$ = x_project($2); }
          // | '_' RE           { $$ = x_project($2); }
          // | RE '&' RE        { $$ = conjunct($1, $3); delete $3; }
          // | RE '-' RE        { $$ = subtract($1, $3); delete $3; }
          // | RE '|' RE        { $$ = disjunct($1, $3); delete $3; }
          // | '(' RE ')'       { $$ = $2; }
          // | STRING           { $$ = read_words($1); }
          // | STRING2          { $$ = read_transducer($1); }
          ;

RANGES:     RANGE RANGES     { $$ = add_range($1,$2); }
          |                  { $$ = NULL; }
          ;

RANGE:      '[' VALUES ']'   { $$=$2; }
          | '[' '^' VALUES ']' { $$=complement_range($3); }
          //| '[' RSVAR ']'    { $$=rsvar_value($2); }
          | '.'              { $$=NULL; }
          | CODE             { $$=HfstCompiler::add_value($1,NULL); }
          ;

CONTEXTS2:  CONTEXTS               { $$ = $1; }
          | '(' CONTEXTS ')'       { $$ = $2; }
          ;

CONTEXTS:   CONTEXT ',' CONTEXTS   { $$ = $3.insert($1)); }
          | CONTEXT                { $$ = $1; }
          ;

CONTEXT2:   CONTEXT                { $$ = $1; }
          | '(' CONTEXT ')'        { $$ = $2; }
          ;

CONTEXT :   RE POS RE              { }// $$ = make_context($1, $3); }
          |    POS RE              { }// $$ = make_context(NULL, $2); }  // check
          | RE POS                 { }// $$ = make_context($1, NULL); }  // check
          ;

VALUES:     VALUE VALUES           { $$=HfstCompiler::append_values($1,$2); }
          | VALUE                  { $$ = $1; }
          ;

VALUE:      LCHAR '-' LCHAR	   { $$=HfstCompiler::add_values($1,$3,NULL); }
          // | SVAR                   { $$=svar_value($1); }
          | LCHAR  	           { $$=HfstCompiler::add_value(HfstCompiler::character_code($1),NULL); }
          | CODE		   { $$=HfstCompiler::add_value($1,NULL); }
	  | SCHAR		   { $$=HfstCompiler::add_value($1,NULL); }
          ;

LCHAR:      CHARACTER	{ $$=$1; }
          | UTF8CHAR	{ $$=HfstCompiler::utf8toint($1); free($1); }
	  | SCHAR       { $$=$1; }
          ;

CODE:       CHARACTER	{ $$=HfstCompiler::character_code($1); }
          | UTF8CHAR	{ $$=HfstCompiler::symbol_code($1); }
          | SYMBOL	{ $$=HfstCompiler::symbol_code($1); }
          ;

SCHAR:      '.'		{ $$=(unsigned char)HfstCompiler::character_code('.'); }
          | '!'		{ $$=(unsigned char)HfstCompiler::character_code('!'); }
          | '?'		{ $$=(unsigned char)HfstCompiler::character_code('?'); }
          | '{'		{ $$=(unsigned char)HfstCompiler::character_code('{'); }
          | '}'		{ $$=(unsigned char)HfstCompiler::character_code('}'); }
          | ')'		{ $$=(unsigned char)HfstCompiler::character_code(')'); }
          | '('		{ $$=(unsigned char)HfstCompiler::character_code('('); }
          | '&'		{ $$=(unsigned char)HfstCompiler::character_code('&'); }
          | '|'		{ $$=(unsigned char)HfstCompiler::character_code('|'); }
          | '*'		{ $$=(unsigned char)HfstCompiler::character_code('*'); }
          | '+'		{ $$=(unsigned char)HfstCompiler::character_code('+'); }
          | ':'		{ $$=(unsigned char)HfstCompiler::character_code(':'); }
          | ','		{ $$=(unsigned char)HfstCompiler::character_code(','); }
          | '='		{ $$=(unsigned char)HfstCompiler::character_code('='); }
          | '_'		{ $$=(unsigned char)HfstCompiler::character_code('_'); }
          | '^'		{ $$=(unsigned char)HfstCompiler::character_code('^'); }
          | '-'		{ $$=(unsigned char)HfstCompiler::character_code('-'); }
          ;

NEWLINES:   NEWLINE NEWLINES     {}
          | /* nothing */        {}
          ;

%%

extern FILE  *yyin;
static int Compact=0;
static int LowMem=0;

/*******************************************************************/
/*                                                                 */
/*  yyerror                                                        */
/*                                                                 */
/*******************************************************************/

void yyerror(char *text)

{
  cerr << "\n" << SFST::FileName << ":" << yylineno << ": " << text << " at: ";
  cerr << yytext << "\naborted.\n";
  exit(1);
}


/*******************************************************************/
/*                                                                 */
/*  warn                                                           */
/*                                                                 */
/*******************************************************************/

void warn(char *text)

{
  cerr << "\n" << SFST::FileName << ":" << yylineno << ": warning: " << text << "!\n";
}


/*******************************************************************/
/*                                                                 */
/*  warn2                                                          */
/*                                                                 */
/*******************************************************************/

void warn2(char *text, char *text2)

{
  cerr << "\n" << SFST::FileName << ":" << yylineno << ": warning: " << text << ": ";
  cerr << text2 << "\n";
}


/*******************************************************************/
/*                                                                 */
/*  get_flags                                                      */
/*                                                                 */
/*******************************************************************/

void get_flags( int *argc, char **argv )

{
  for( int i=1; i<*argc; i++ ) {
    if (strcmp(argv[i],"-c") == 0) {
      Compact = 1;
      argv[i] = NULL;
    }
    else if (strcmp(argv[i],"-l") == 0) {
      LowMem = 1;
      argv[i] = NULL;
    }
    else if (strcmp(argv[i],"-q") == 0) {
      SFST::Verbose = 0;
      argv[i] = NULL;
    }
    else if (strcmp(argv[i],"-s") == 0) {
      Switch = 1;
      argv[i] = NULL;
    }
  }
  // remove flags from the argument list
  int k;
  for( int i=k=1; i<*argc; i++)
    if (argv[i] != NULL)
      argv[k++] = argv[i];
  *argc = k;
}


/*******************************************************************/
/*                                                                 */
/*  main                                                           */
/*                                                                 */
/*******************************************************************/

int main( int argc, char *argv[] )

{
  FILE *file;

  get_flags(&argc, argv);
  if (argc < 3) {
    fprintf(stderr,"\nUsage: %s [options] infile outfile\n", argv[0]);
    fprintf(stderr,"\nOPTIONS:\n");
    fprintf(stderr,"-c\tStore the transducer in fst-infl2 format.\n");
    fprintf(stderr,"-l\tStore the transducer in fst-infl3 format.\n");
    fprintf(stderr,"-s\tSwitch the upper and lower levels producing a transducer for generation rather than recognition.\n");
    fprintf(stderr,"-q\tquiet mode\n\n");
    exit(1);
  }
  if ((file = fopen(argv[1],"rt")) == NULL) {
    fprintf(stderr,"\nError: Cannot open grammar file \"%s\"\n\n", argv[1]);
    exit(1);
  }
  SFST::FileName = argv[1];
  //Result = NULL;
  SFST::TheAlphabet.utf8 = SFST::UTF8;
  yyin = file;
  try {
    yyparse();
    //Result->alphabet.utf8 = UTF8;
    //if (Verbose)
    //  cerr << "\n";
    //if (Result->is_empty()) 
    //  warn("resulting transducer is empty"); 
    if ((file = fopen(argv[2],"wb")) == NULL) {
	fprintf(stderr,"\nError: Cannot open output file %s\n\n", argv[2]);
	exit(1);
    }
    //if (Compact) {
    //  MakeCompactTransducer ca(*Result);
    //  delete Result;
    //  ca.store(file);
    //}
    //else if (LowMem)
    //  Result->store_lowmem(file);
    else
      Result->write_in_att_format(file);
    fclose(file);
  }
  catch(const char* p) {
      cerr << "\n" << p << "\n\n";
      exit(1);
  }
}
