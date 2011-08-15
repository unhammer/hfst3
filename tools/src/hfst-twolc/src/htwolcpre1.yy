//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, version 3 of the Licence.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

%{
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "string_src/string_manipulation.h"
#include "io_src/InputReader.h"
#include "grammar_defs.h"
#include "variable_src/RuleSymbolVector.h"
#include "variable_src/RuleVariables.h"
#include "commandline_src/CommandLine.h"

  extern int yylineno;
  extern char * yytext;
  extern int yylineno;
  extern char * yytext;
  extern bool regexp_start;
  extern bool rules_start;
  void yyerror(const char * text );
  void warn(const char * warning );
  int yylex();
  int yyparse();
  void reduce_queue(bool variable_symbol=false);
  void set_variable_values(void);
  void reduce_symbol_pair(void);
  void increase_line_counter(void);
  std::string &get_symbol_queue_front(void);
  void pop_symbol_queue(void);

#define YYERROR_VERBOSE 1

  // For displaying the line number in error messages and warnings.
  size_t line_number = 1;
 
  // For reading input one byte at a time.
  InputReader input_reader(line_number);

  // For keeping track of values of variables.
  VariableValueMap variable_value_map;
  
  // For storing variable constructions of rules.
  RuleVariables rule_variables;

  // For storing the string representation of rules and replacing
  // rule variables by their values.
  RuleSymbolVector rule_symbol_vector(variable_value_map);

  // The latest symbol that was read is always the last element of this queue.
  HandyDeque<std::string> symbol_queue;

  // Stores symbol set names.
  HandySet<std::string> sets;

  // Stores definition names.
  HandySet<std::string> definitions;

  // Stores set names in "__HFST_TWOLC_SET_NAME=X" format and the set symbols.
  HandyMap<std::string,std::vector<std::string> > set_symbols;

  // The name of the lates set which was defined.
  std::string set_name;
  
  // Pointer to the latest symbol set which is being defined.
  std::vector<std::string> latest_set;

  // Tells whether we are inside a ( .. ). For variable rules.
  bool inside_parenthesis = false;

  // For temporarily storing a rule variable and its values
  StringVector variable_vector;

  // Queue for rule-matchers.
  HandyDeque<Matcher> matcher_queue;
%}



%union 
{ int symbol_number; };


 /* 
    All unary operators have stronger precedence than binary ones.
 */

 /* Binary operators ordered by precedence from lowest to highest. */
%left  <symbol_number> FREELY_INSERT
%left  <symbol_number> DIFFERENCE
%left  <symbol_number> INTERSECTION
%left  <symbol_number> UNION

 /* Unary operators ordered by precedence from lowest to highest. */
%right <symbol_number> STAR PLUS
%left  <symbol_number> CONTAINMENT CONTAINMENT_ONCE TERM_COMPLEMENT COMPLEMENT 
%right <symbol_number> POWER

 /* "[", "]", "(", ")", "{", "}". */
%right <symbol_number> RIGHT_SQUARE_BRACKET RIGHT_PARENTHESIS 
%left  <symbol_number> LEFT_SQUARE_BRACKET LEFT_PARENTHESIS
%right <symbol_number> RIGHT_CURLY_BRACKET
%left  <symbol_number> LEFT_CURLY_BRACKET

 /* Twolc rule operators */
%token <symbol_number> LEFT_RESTRICTION_ARROW LEFT_ARROW RIGHT_ARROW 
%token <symbol_number> LEFT_RIGHT_ARROW

 /* Twolc regular expression rule operators */
%token <symbol_number> RE_LEFT_RESTRICTION_ARROW RE_LEFT_ARROW RE_RIGHT_ARROW 
%token <symbol_number> RE_LEFT_RIGHT_ARROW

 /* Twolc regular expression rule center brackets. */
%right <symbol_number> RE_RIGHT_SQUARE_BRACKET
%left  <symbol_number> RE_LEFT_SQUARE_BRACKET

 /* Basic tokens. */
%token <symbol_number>  ALPHABET_DECLARATION DIACRITICS_DECLARATION 
%token <symbol_number>  SETS_DECLARATION DEFINITION_DECLARATION
%token <symbol_number>  RULES_DECLARATION VARIABLE_DECLARATION
%token <symbol_number>  COLON WHERE MATCHED_MATCHER 
%token <symbol_number>  MIXED_MATCHER FREELY_MATCHER IN AND
%token <symbol_number>  COLON_SPACE SYMBOL_SPACE
%token <symbol_number>  SEMI_COLON EQUALS CENTER_MARKER
%token <symbol_number>  RULE_NAME SYMBOL NUMBER NUMBER_SPACE
%token <symbol_number>  QUESTION_MARK EXCEPT
%%

ALL: GRAMMAR {}
;

GRAMMAR: ALPHABET GRAMMAR1 
| GRAMMAR1

GRAMMAR1: DIACRITICS GRAMMAR2 
| GRAMMAR2

GRAMMAR2: VARIABLES GRAMMAR3 
| GRAMMAR3           

GRAMMAR3: SETS GRAMMAR4 
| GRAMMAR4

GRAMMAR4: DEFINITIONS GRAMMAR5
| GRAMMAR5

GRAMMAR5: RULES

RULES:RULES_DECLARATION RULE_LIST

RULE_LIST: /* empty */
| RULE_LIST RULE

RULE: RULE_NAME_DECL RULE_CENTER RULE_OPERATOR RULE_CONTEXTS 
NEGATIVE_RULE_CONTEXTS RULE_VARIABLES
{
  // If this rule didn't have variables, display it. Otherwise iterate
  // through its variable value combinations and display the rule using the 
  // different combinations.
  if (rule_variables.empty())
    { std::cout << rule_symbol_vector.replace_variables(); }
  else
    {
      for (RuleVariables::const_iterator it = rule_variables.begin();
	   it != rule_variables.end();
	   ++it)
	{ 
	  it.set_values(variable_value_map); 
	  std::cout << rule_symbol_vector.replace_variables();
	}
    }

  // Clear all containers, so that we'll be ready to handle the next rule.
  rule_symbol_vector.clear();
  variable_value_map.clear();
  rule_variables.clear();
}
| RULE_NAME_DECL RE_RULE_CENTER RE_RULE_OPERATOR RULE_CONTEXTS 
NEGATIVE_RULE_CONTEXTS
{ 
  std::cout << rule_symbol_vector.replace_variables(); 

  // Clear all containers, so that we'll be ready to handle the next rule.
  rule_symbol_vector.clear();
  variable_value_map.clear();
  rule_variables.clear();
}

RULE_NAME_DECL: RULE_NAME
{ 
  // Add the rule name to rule_symbol_vector.
  reduce_queue(); 
}

RULE_CENTER: ALPHABET_PAIR
| RULE_CENTER UNION ALPHABET_PAIR
| LEFT_SQUARE_BRACKET CENTER_LIST RIGHT_SQUARE_BRACKET
| LEFT_CURLY_BRACKET CENTER_LIST RIGHT_CURLY_BRACKET

RE_RULE_CENTER: RE_LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RE_RIGHT_SQUARE_BRACKET

CENTER_LIST: ALPHABET_PAIR
| CENTER_LIST UNION ALPHABET_PAIR

RULE_OPERATOR:LEFT_ARROW
| RIGHT_ARROW
| LEFT_RESTRICTION_ARROW
| LEFT_RIGHT_ARROW

RE_RULE_OPERATOR: RE_LEFT_ARROW
| RE_RIGHT_ARROW
| RE_LEFT_RESTRICTION_ARROW
| RE_LEFT_RIGHT_ARROW

RULE_CONTEXTS: /* empty */
| RULE_CONTEXTS RULE_CONTEXT

NEGATIVE_RULE_CONTEXTS: /* empty */
| EXCEPT RULE_CONTEXTS

MATCHER:MATCHED_MATCHER
{ matcher_queue.push_back(MATCHED); }
| MIXED_MATCHER
{ matcher_queue.push_back(MIXED); }
| FREELY_MATCHER
{ matcher_queue.push_back(FREELY); }
| /* Empty. FREELY by default. */
{ matcher_queue.push_back(FREELY); }


RULE_CONTEXT: REGULAR_EXPRESSION CENTER_MARKER REGULAR_EXPRESSION
SEMI_COLON_LIST

RULE_VARIABLES: /* empty */
| WHERE RULE_VARIABLE_BLOCKS SEMI_COLON_LIST

RULE_VARIABLE_BLOCKS: RULE_VARIABLE_BLOCK
| RULE_VARIABLE_BLOCKS AND RULE_VARIABLE_BLOCK

RULE_VARIABLE_BLOCK: RULE_VARIABLE_INITIALIZATION_LIST MATCHER
{
  // Set variable block matcher.
  rule_variables.set_matcher
    (matcher_queue.empty() ? FREELY : matcher_queue.get_front_and_pop());
}

RULE_VARIABLE_INITIALIZATION_LIST: /* empty */
| RULE_VARIABLE_INITIALIZATION_LIST RULE_VARIABLE_INITIALIZATION

RULE_VARIABLE_INITIALIZATION:
VAR_SYMBOL IN LEFT_PARENTHESIS VAR_SYMBOL_LIST RIGHT_PARENTHESIS
{ set_variable_values(); }
| VAR_SYMBOL IN VAR_SYMBOL
{ set_variable_values(); }

VAR_SYMBOL:GRAMMAR_SYMBOL_SPACE
{ reduce_queue(true); }

VAR_SYMBOL_LIST: /* empty */
| VAR_SYMBOL_LIST VAR_SYMBOL

ALPHABET: ALPHABET_DECLARATION ALPHABET_PAIR_LIST SEMI_COLON_LIST

DIACRITICS: DIACRITICS_DECLARATION SYMBOL_LIST SEMI_COLON_LIST

VARIABLES: VARIABLE_DECLARATION VARIABLE_LIST SEMI_COLON_LIST

SETS: SETS_DECLARATION SET_LIST 

DEFINITIONS: DEFINITION_DECLARATION DEFINITION_LIST

DEFINITION_LIST: /* empty */
| DEFINITION_LIST DEFINITION

DEFINITION: DEFINITION_NAME EQUALS REGULAR_EXPRESSION SEMI_COLON_LIST

REGULAR_EXPRESSION: RE_LIST
| REGULAR_EXPRESSION UNION RE_LIST
| REGULAR_EXPRESSION INTERSECTION RE_LIST
| REGULAR_EXPRESSION DIFFERENCE RE_LIST
| REGULAR_EXPRESSION FREELY_INSERT RE_LIST

RE_LIST: /* empty */
| RE_LIST RE

RE: PAIR
| RE POWER NUMBER_SPACE
{ 
  symbol_queue.front() = 
    std::string("__HFST_TWOLC_NUMBER=") + symbol_queue.front(); 
  reduce_queue();
}
| RE STAR
| RE PLUS
| CONTAINMENT RE
| CONTAINMENT_ONCE RE
| COMPLEMENT RE
| TERM_COMPLEMENT RE
| LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RIGHT_SQUARE_BRACKET
| LEFT_CURLY_BRACKET REGULAR_EXPRESSION RIGHT_CURLY_BRACKET
| LEFT_PARENTHESIS REGULAR_EXPRESSION RIGHT_PARENTHESIS

SET_LIST: /* empty */ 
| SET_LIST SET_DEFINITION 

SYMBOL_LIST: /* empty */
| SYMBOL_LIST SET_SYMBOL

SET_SYMBOL: GRAMMAR_SYMBOL_SPACE
{ 
  // Push the set_symbol into latest_set, which contains symbols in the next
  // set which will be defined.
  if (sets.find(get_symbol_queue_front()) != sets.end())
    {
      for (std::vector<std::string>::iterator it = 
	     set_symbols
	     ["__HFST_TWOLC_SET_NAME=" + get_symbol_queue_front()].begin();
	   it != set_symbols
	     ["__HFST_TWOLC_SET_NAME=" + get_symbol_queue_front()].end();
	   ++it)
	{ 
	  std::cout << *it << " ";
	  latest_set.push_back(*it);
	}
      pop_symbol_queue();
    }
  else
    { 
      latest_set.push_back(unescape(get_symbol_queue_front())); 
      reduce_queue();  
    }
}

SET_DEFINITION: SET_NAME EQUALS SYMBOL_LIST SEMI_COLON_LIST
{
  // Store the set symbols of the set whose name is set_name into set_symbols.
  set_symbols[set_name] = latest_set;
  latest_set.clear();
}

SET_NAME: SYMBOL_SPACE
{ 
  // Store the set name in sets and push it at the back of 
  // symbol_queue. 
  sets.insert(get_symbol_queue_front());
  get_symbol_queue_front() = 
    "__HFST_TWOLC_SET_NAME=" + get_symbol_queue_front();
  set_name = get_symbol_queue_front(); 
  reduce_queue();
}

DEFINITION_NAME: SYMBOL_SPACE
{
  // Store the definition name in definitions and push it at the back of 
  // symbol_queue.
  definitions.insert(get_symbol_queue_front());
  get_symbol_queue_front() = 
    "__HFST_TWOLC_DEFINITION_NAME=" + get_symbol_queue_front();
  reduce_queue();
}

ALPHABET_PAIR_LIST: /* empty */
| ALPHABET_PAIR_LIST ALPHABET_PAIR


PAIR: GRAMMAR_SYMBOL COLON_SPACE
{ 
  // For pairs "X:" and "X:?".
  // Reduce the first three symbols "X", "__HFST_TWOLC_:" and "__HFST_TWOLC_?"
  // from symbol_queue.
  reduce_symbol_pair(); 
}
| COLON GRAMMAR_SYMBOL_SPACE
{
  // For pairs ":X".
  // Push a "__HFST_TWOLC_?" onto symbol_queue.
  // Reduce the three first symbols "__HFST_TWOLC_?", "__HFST_TWOLC_:" and "X"
  // from symbol_queue.
  symbol_queue.push_front("__HFST_TWOLC_?");
  reduce_symbol_pair(); 
}
| GRAMMAR_SYMBOL COLON GRAMMAR_SYMBOL_SPACE
{ 
  // For pairs "X:Y".
  // Reduce the first three symbols "X", "__HFST_TWOLC_:" and "Y" from
  // symbol_queue.
  reduce_symbol_pair(); 
}
| COLON_SPACE
{
  // For pairs ":" and ":?".
  // Push a "__HFST_TWOLC_?" onto symbol_queue.
  // Reduce the three first symbols "__HFST_TWOLC_?", "__HFST_TWOLC_:" and
  // "__HFST_TWOLC_?" from symbol_queue.
  symbol_queue.push_front("__HFST_TWOLC_?");
  reduce_symbol_pair();
}
| GRAMMAR_SYMBOL_SPACE
{
  // For pairs "X".
  // Push "X" and "__HFST_TWOLC_:" on top of symbol_queue.
  // Reduce the first three symbols "X", "__HFST_TWOLC_:" and "X" from
  // symbol_queue.
  std::string symbol = get_symbol_queue_front();

  // Add the colon and output symbol.
  symbol_queue.push_front("__HFST_TWOLC_:");
  symbol_queue.push_front(symbol); 
  reduce_symbol_pair();
}


ALPHABET_PAIR: GRAMMAR_SYMBOL COLON GRAMMAR_SYMBOL_SPACE
{ 
  // For pairs "X:Y".
  // Reduce the first three symbols "X", "__HFST_TWOLC_:" and "Y" from
  // symbol_queue.
  reduce_symbol_pair(); 
}
| GRAMMAR_SYMBOL_SPACE
{
  // For pairs "X".
  // Push "X" and "__HFST_TWOLC_:" on top of symbol_queue.
  // Reduce the first three symbols "X", "__HFST_TWOLC_:" and "X" from
  // symbol_queue.
  std::string symbol = get_symbol_queue_front();
  symbol_queue.push_back("__HFST_TWOLC_:");
  symbol_queue.push_back(symbol);
  reduce_symbol_pair();
}

GRAMMAR_SYMBOL: SYMBOL
| NUMBER


GRAMMAR_SYMBOL_SPACE: SYMBOL_SPACE
| NUMBER_SPACE


VARIABLE_LIST: /* empty */
| VARIABLE_LIST GRAMMAR_SYMBOL_SPACE
{ pop_symbol_queue(); }

SEMI_COLON_LIST: SEMI_COLON
| SEMI_COLON_LIST SEMI_COLON

%%

// Print warning. 
void warn(const char * warning) 
{ input_reader.warn(warning); }

// Print error messge and exit 1.
void yyerror(const char * text) 
{ 
  input_reader.error(text); 
  std::cout << "__HFST_TWOLC_DIE";
  exit(1);
}

// Set the variable of this variable initialization and set its values.
// If its value list contains set names, replace them by the set elements.
void set_variable_values(void)
{
  rule_variables.set_variable(variable_vector.front());
  for (StringVector::const_iterator it = variable_vector.begin()+1;
       it != variable_vector.end();
       ++it)
    {
      if (set_symbols.has_key(*it))
	{ rule_variables.add_values(set_symbols[*it]); }
      else
	{ rule_variables.add_value(*it); }
    }
  variable_vector.clear(); 
}

// Pop the queue three times: once for the input symbol, once for the pair
// separator and once for the output symbol.
void reduce_symbol_pair(void)
{
  reduce_queue();
  reduce_queue();
  reduce_queue();
}

// Increase line counter for every symbol "__HFST_TWOLC_\\n", which is
// encountered.
void increase_line_counter(void)
{
  while (not symbol_queue.empty() && 
	 symbol_queue.front() == "__HFST_TWOLC_\\n")
    {
      ++line_number;
      symbol_queue.pop_front();
    }
}

// First pop all "__HFST_TWOLC_\\n" in symbol_queue, while incrementing
// line_counter. Then pop the first element != "__HFST_TWOLC_\\n" in 
// symbol_queue.
void pop_symbol_queue(void)
{
  increase_line_counter();
  symbol_queue.pop_front();
}

// First pop all "__HFST_TWOLC_\\n" in symbol_queue, while incrementing
// line_counter. Then return a reference to the first element 
// != "__HFST_TWOLC_\\n" in symbol_queue.
std::string &get_symbol_queue_front(void)
{
  increase_line_counter();
  return symbol_queue.front();
}

// Decide what to do with the next symbol in symbol_queue.
void reduce_queue(bool variable_symbol)
{ 
  increase_line_counter();
  // Test whether the next symbol is a set name and label it a set if it is.
  if (sets.has_element(get_symbol_queue_front()))
    {  get_symbol_queue_front() = 
	std::string("__HFST_TWOLC_SET_NAME=") + get_symbol_queue_front(); }

  // Test whether the next symbol is a definition name and label it a
  // definition if it is. 
  if (definitions.has_element(get_symbol_queue_front()))
    { get_symbol_queue_front() = 
      std::string("__HFST_TWOLC_DEFINITION_NAME=") + get_symbol_queue_front();}

  // Unescape the escaped characters in the symbol.
  get_symbol_queue_front() = unescape(get_symbol_queue_front());
  
  // We treat different symbols differently:
  //
  // 1. We display symbols, which aren't part of a rule.
  // 2. We push back rule symbols into rule_symbol_vector, which stores
  //    string representations of rules.
  // 3. We push back variable symbols into rule_symbol_vector, which stores 
  //    rule-variable names and values.
  if (not variable_symbol)
    {
      if (not rules_start)
	{ 
	  std::cout << get_symbol_queue_front() << " "; 
	  pop_symbol_queue();
	}
      else
	{ 
	  rule_symbol_vector.push_back
	    (StringVector(get_symbol_queue_front()));
	  pop_symbol_queue();
	}
    }
  else
    { 
      variable_vector.push_back(get_symbol_queue_front()); 
      pop_symbol_queue();
    }
}

int main(int argc, char * argv[])
{
  CommandLine command_line(argc,argv);

  if (command_line.help or command_line.version)
    {
      if (command_line.version)
	{ command_line.print_version(); }
      if (command_line.help)
	{ command_line.print_help(); }
      exit(0);
    }
  if (command_line.usage)
    {
      command_line.print_usage();
      exit(0);
    }
  if (not command_line.be_quiet)
    {
      if (not command_line.has_input_file)
	{ std::cerr << "Reading input from STDIN." << std::endl; }
      else
	{ std::cerr << "Reading input from " << command_line.input_file_name
		    << "." << std::endl; }
      if (not command_line.has_output_file)
	{ std::cerr << "Writing output to STDOUT." << std::endl; }
      else
	{ std::cerr << "Writing output to " << command_line.output_file_name
		    << "." << std::endl; }
    }
  if (command_line.be_verbose)
    { std::cerr << "Verbose mode." << std::endl; }

  input_reader.set_input(command_line.set_input_file());

  // Test that the output file is okay.
  (void)command_line.set_output_file();

  //yydebug = 1;

  return yyparse();
}


