//! @file XfstCompiler.h
//!
//! @brief A class that encapsulates compilation of Xerox fst language scripts
//!        expressions into HFST automata.
//!
//!        Xerox fst language is described in Finite state morphology (2004) by
//!        Beesley and Karttunen.
//!
//!        This class is merely a wrapper around lex and yacc functions handling
//!        the parsing.

//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GUARD_XfstCompiler_h
#define GUARD_XfstCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <list>
#include <map>
#include <stack>

#include "HfstTransducer.h"
#include "XreCompiler.h"
#include "LexcCompiler.h"

namespace hfst { 
//! @brief hfst::xfst namespace contains all functions needed to parse XFST scritpts
namespace xfst {

  // Used internally in function 'apply_unary_operator'.
  enum UnaryOperation
  { DETERMINIZE_NET, EPSILON_REMOVE_NET, INVERT_NET,
    LOWER_SIDE_NET, UPPER_SIDE_NET, OPTIONAL_NET, ONE_PLUS_NET,
    ZERO_PLUS_NET, REVERSE_NET, MINIMIZE_NET };

  // Used internally in function 'apply_binaryoperator(_iteratively)'.
  enum BinaryOperation
  { IGNORE_NET, INTERSECT_NET, COMPOSE_NET, CONCATENATE_NET, MINUS_NET,
    UNION_NET, SHUFFLE_NET };

  // Used internally in function 'apply'.
  enum ApplyDirection { APPLY_UP_DIRECTION, APPLY_DOWN_DIRECTION };

  // Used internally in function 'test_uni'.
  enum Level { LOWER_LEVEL, UPPER_LEVEL };

  enum TestOperation { TEST_SUBLANGUAGE_, TEST_OVERLAP_ };

//! @brief Xfst compiler contains all the methods and variables a session of
//! XFST script parser needs.
class XfstCompiler
{
  public:
  //! @brief Construct compiler for unknown format transducers.
  XfstCompiler();
  //! @brief Create compiler for @a impl format transducers
  XfstCompiler(hfst::ImplementationType impl);

  //! @brief Add properties from file, one property per line
  //! @todo properties cannot be stored in HFST automata
  XfstCompiler& add_props(FILE* infile);
  //! @brief Add properties from text, one property per line
  //! @todo properties cannot be stored in HFST automata
  XfstCompiler& add_props(const char* indata);

  //! @brief Perform lookdowns on top of the stack, one per line
  //! @todo lookdown is missing from HFST
  XfstCompiler& apply_up(FILE* infile);
  //! @brief Perform lookdowns on top of the stack, one per line
  //! @todo lookdown is missing from HFST
  XfstCompiler& apply_up(const char* indata);
  //! @brief Perform lookups on top of the stack, one per line
  //! @todo lookup is missing from HFST
  XfstCompiler& apply_down(FILE* infile);
  //! @brief Perform lookups on top of the stack, one per line
  //! @todo lookup is missing from HFST
  XfstCompiler& apply_down(const char* indata);
  //! @brief Perform lookmeds on top of the stack, one per line
  //! @todo lookmed is missing from HFST
  XfstCompiler& apply_med(FILE* infile);
  //! @brief Perform lookmeds on top of the stack, one per line
  //! @todo lookmed is missing from HFST
  XfstCompiler& apply_med(const char* indata);

  //! @brief Define alias for command sequence
  XfstCompiler& define_alias(const char* name, const char* commands);
  //! @brief Define list by range
  //! @todo lists are not supported by HFST
  //! @todo Unicode ranges are not supported
  XfstCompiler& define_list(const char* name, const char* start, const char* end);
  //! @brief Define list by labels
  //! @todo lists are not supportedd by HFST
  XfstCompiler& define_list(const char* name, const char* list);
  //! @brief Define regex macro
  XfstCompiler& define(const char* name, const char* xre);
  //! @brief Define regex macro function
  //! @todo Regex parser does not support macro functions
  XfstCompiler& define(const char* name, const char* prototype, const char* xre);
  //! @brief Remove definition
  XfstCompiler& undefine(const char* name_list);
  //! @brief Remove list
  //! @todo HFST does not support lists
  XfstCompiler& unlist(const char* name);

  //! @brief Name top network of stack
  //! @todo HFST automata do not remember their names
  XfstCompiler& name(const char* name);

  //! @brief Load regex macros from file
  //! @todo Definition names cannot be stored in HFST automata binaries
  XfstCompiler& load_definitions(FILE* infile);

  //! @brief Search help directory
  //! @todo helps have not been written or copied
  XfstCompiler& apropos(const char* text);
  //! @brief Print help topics
  //! @todo helps have not been written or copied
  XfstCompiler& describe(const char* text);

  //! @brief Clear stack
  XfstCompiler& clear();
  //! @brief Pop stack
  XfstCompiler& pop();
  //! @brief Push definition on stack
  XfstCompiler& push(const char* name);
  //! @brief Push last definition on stack
  XfstCompiler& push();
  //! @brief Reverse stack
  XfstCompiler& turn();
  //! @brief Move top of stack to bottom
  XfstCompiler& rotate();
  //! @brief Load stack from file
  XfstCompiler& load_stack(const char* infilename);

  //! @brief Print parts of automaton with epsilon loops
  //! @todo unimplemented yet
  XfstCompiler& collect_epsilon_loops();
  //! @brief Remove unnecessary symbols using ?
  //! @todo HFST does not support ?
  XfstCompiler& compact_sigma();

  //! @brief Eliminate flag diacritic
  //! @todo unimplemented yet
  XfstCompiler& eliminate_flag(const char* name);
  //! @brief Eliminate all flag diacritics
  //! @todo unimplemented yet
  XfstCompiler& eliminate_flags();

  //! @brief Print @a text to stdout
  XfstCompiler& echo(const char* text);
  //! @brief Stop parser, print quit message
  XfstCompiler& quit(const char* message);
  //! @brief Execute @c system()
  XfstCompiler& system(const char* command);

  //! @brief Set variable @c name = @c text
  XfstCompiler& set(const char* name, const char* text);
  //! @brief Set variable @c name = @c number
  XfstCompiler& set(const char* name, unsigned int number);
  //! @brief Show named variable
  XfstCompiler& show(const char* name);
  //! @brief Show all variables
  XfstCompiler& show();

  XfstCompiler& test_uni(Level level);

  //! @brief Test top transducer in stack for equivalence
  //! @todo tests are not implemented
  XfstCompiler& test_eq();
  //! @brief Test top transducer in stack for functionality
  //! @todo tests are not implemented
  XfstCompiler& test_funct();
  //! @brief Test top transducer in stack for identity
  //! @todo tests are not implemented
  XfstCompiler& test_id();
  //! @brief Test top transducer in stack for upper language boundedness
  //! @todo tests are not implemented
  XfstCompiler& test_upper_bounded();
  //! @brief Test top transducer in stack for upper language universality
  //! @todo tests are not implemented
  XfstCompiler& test_upper_uni();
  //! @brief Test top transducer in stack for lower language boundedness
  //! @todo tests are not implemented
  XfstCompiler& test_lower_bounded();
  //! @brief Test top transducer in stack for lower language universality
  //! @todo tests are not implemented
  XfstCompiler& test_lower_uni();
  //! @brief Test top transducer in stack for not emptiness
  //! @todo tests are not implemented
  XfstCompiler& test_nonnull();
  //! @brief Test top transducer in stack for emptiness
  //! \a invert_test_result defines whether the result is inverted
  //! (so that 'test_nonnull' can be implemented with the same function).
  //! @todo tests are not implemented
  XfstCompiler& test_null(bool invert_test_result=false);
  //! @brief Test top transducer in stack for overlapping
  //! @todo tests are not implemented
  XfstCompiler& test_overlap();
  //! @brief Test top transducer in stack for sublanguage
  //! @todo tests are not implemented
  XfstCompiler& test_sublanguage();
  //! @brief Test top transducer in stack for unambiguity
  //! @todo tests are not implemented
  XfstCompiler& test_unambiguous();

  //! @brief Substitute labels @a src by @a target.
  XfstCompiler& substitute(const char* src, const char* target);
  //! @brief Substitute all labels in @a list by @a target.
  XfstCompiler& substitute(const char** list, const char* target);

  //! @brief Print aliases
  XfstCompiler& print_aliases(FILE* outfile);
  //! @brief Print arc count for @a level
  XfstCompiler& print_arc_count(const char* level, FILE* outfile);
  //! @brief Print arc count
  XfstCompiler& print_arc_count(FILE* outfile);
  //! @brief Print definition
  XfstCompiler& print_defined(FILE* outfile);
  //! @brief Print directory contents
  XfstCompiler& print_dir(const char* glob, FILE* outfile);
  //! @brief Print file info
  XfstCompiler& print_file_info(FILE* outfile);
  //! @brief Print flag diacritics
  XfstCompiler& print_flags(FILE* outfile);

  XfstCompiler& print_labels(FILE* outfile, HfstTransducer* tr);

  //! @brief Print labels in network @a name
  XfstCompiler& print_labels(const char* name, FILE* outfile);
  //! @brief Print labels
  XfstCompiler& print_labels(FILE* outfile);
  //! @brief Print label mappings
  XfstCompiler& print_labelmaps(FILE* outfile);
  //! @brief Print label count
  XfstCompiler& print_label_count(FILE* outfile);
  //! @brief Print list named @a name
  XfstCompiler& print_list(const char* name, FILE* outfile);
  //! @brief Print all lists
  XfstCompiler& print_list(FILE* outfile);

  XfstCompiler& shortest_string
    (const hfst::HfstTransducer* transducer, hfst::HfstTwoLevelPaths& paths);

  //! @brief Print shortest string of network
  XfstCompiler& print_shortest_string(FILE* outfile);
  //! @brief Print length of shortest string
  XfstCompiler& print_shortest_string_size(FILE* outfile);
  //! @brief Print longest string in network
  XfstCompiler& print_longest_string(FILE* outfile);
  //! @brief Print length of longest string
  XfstCompiler& print_longest_string_size(FILE* outfile);
  //! @brief Print strings of lower language
  XfstCompiler& print_lower_words(const char* name, unsigned int number,
                                  FILE* outfile);
  //! @brief Print random strings of lower language
  XfstCompiler& print_random_lower(unsigned int number, FILE* outfile);
  //! @brief Print astrings of upper language
  XfstCompiler& print_upper_words(const char* name, unsigned int number,
                                  FILE* outfile);
  //! @brief Print random strings of upper language
  XfstCompiler& print_random_upper(unsigned int number, FILE* outfile);
  //! @brief Print pair strings of language
  XfstCompiler& print_words(const char* name, unsigned int number,
                            FILE* outfile);
  //! @brief Print random pair strings of language
  XfstCompiler& print_random_words(unsigned int number, FILE* outfile);
  //! @brief Print name of top network
  XfstCompiler& print_name(FILE* outfile);
  //! @brief Print network
  XfstCompiler& print_net(FILE* outfile);
  //! @brief Print network named @a name
  XfstCompiler& print_net(const char* name, FILE* outfile);
  //! @brief Print properties of top network
  XfstCompiler& print_properties(FILE* outfile);
  //! @brief Print properties of network named @a name
  XfstCompiler& print_properties(const char* name, FILE* outfile);
  //! @brief Print all symbols of network named @a name
  XfstCompiler& print_sigma(const char* name, FILE* outfile);
  //! @brief Print all symbols of network
  XfstCompiler& print_sigma(FILE* outfile);
  //! @brief Print nnumber of symbols in network
  XfstCompiler& print_sigma_count(FILE* outfile);
  //! @brief Print number of paths with all symbols on @a level
  XfstCompiler& print_sigma_word_count(const char* level, FILE* outfile);
  //! @brief Print number of paths with all symbols
  XfstCompiler& print_sigma_word_count(FILE* outfile);
  //! @brief Print size of network named @a name
  XfstCompiler& print_size(const char* name, FILE* outfile);
  //! @brief Print size of top network
  XfstCompiler& print_size(FILE* outfile);
  //! @brief Print all networks in stack
  XfstCompiler& print_stack(FILE* outfile);
  //! @brief Save @a name network in dot form in @a outfile
  XfstCompiler& write_dot(const char* name, FILE* outfile);
  //! @brief Save top networks dot form in @a outfile
  XfstCompiler& write_dot(FILE* outfile);
  //! @brief Save top networks prolog form in @a outfile
  XfstCompiler& write_prolog(FILE* outfile);
  //! @brief Save top networks spaced paths form in @a outfile
  XfstCompiler& write_spaced(FILE* outfile);
  //! @brief Save top networks paths form in @a outfile
  XfstCompiler& write_text(FILE* outfile);
  //! @brief Save function @a name in @a outfile
  //! @todo HFST does not support function macros in automata
  XfstCompiler& write_function(const char* name, const char* outfilename);
  //! @brief Save definition @a name in @a outfile
  //! @todo HFST does not support saving name of definition in file
  XfstCompiler& write_definition(const char* name, const char* outfilename);
  //! @brief Save all definitions in @a outfile
  //! @todo HFST does not support saving name of definition in file
  XfstCompiler& write_definitions(const char* outfilename);
  //! @brief Save all transducers in stack to @a outfile
  XfstCompiler& write_stack(const char* outfilename);

  //! @brief Read properties from @a infile, one per line
  //! @todo HFST automata do not support properties
  XfstCompiler& read_props(FILE* infile);
  //! @brief Read properties from @a indata, one per line
  //! @todo HFST automata do not support properties
  XfstCompiler& read_props(const char* indata);
  //! @brief Compile file data as one regex and save on stack.
  XfstCompiler& read_regex(FILE* infile);
  //! @brief Compile regex of @a indata and save on stack.
  XfstCompiler& read_regex(const char* indata);
  //! @brief Read prolog form transducer from @a infile
  XfstCompiler& read_prolog(FILE* infile);
  //! @brief Read prolog form transducer from @a indata
  XfstCompiler& read_prolog(const char* indata);
  //! @brief Read spaced form transducer from @a infile
  XfstCompiler& read_spaced(FILE* infile);
  //! @brief Read spaced form transducer from @a indata
  XfstCompiler& read_spaced(const char* indata);
  //! @brief Read text form transducer from @a infile
  XfstCompiler& read_text(FILE* infile);
  //! @brief Read text form transducer from @a indata
  XfstCompiler& read_text(const char* indata);
  //! @brief Read lexicons from @a infile
  XfstCompiler& read_lexc(FILE* infile);
  //! @brief Read lexicons from @a indata
  XfstCompiler& read_lexc(const char* indata);

  //! @brief do some label pushing
  //! @todo HFST automata cannot push labels
  XfstCompiler& cleanup_net();
  //! @brief Make transducer functional
  //! @todo unimplemented
  XfstCompiler& complete_net();
  //! @brief Compose stack
  XfstCompiler& compose_net();
  //! @brief concatenate stack
  XfstCompiler& concatenate_net();
  //! @brief Crossproduct top of stack
  XfstCompiler& crossproduct_net();
  //! @brief Determinize top of stack
  XfstCompiler& determinize_net();
  //! @brief Remove epsilons from top of stack
  XfstCompiler& epsilon_remove_net();
  //! @brief Ignore top of stack with second automaton
  //! @todo unimplemented
  XfstCompiler& ignore_net();
  //! @brief Intersect stack
  XfstCompiler& intersect_net();
  //! @brief invert top of stack
  XfstCompiler& invert_net();
  //! @brief Make top of stack label network
  //! @todo Find out wtf this is
  XfstCompiler& label_net();
  //! @brief Project input for top of stack
  XfstCompiler& lower_side_net();
  //! @brief Project output for top of stack
  XfstCompiler& upper_side_net();
  //! @brief Minimize top of stack
  XfstCompiler& minimize_net();
  //! @brief Subtract second from top of stack
  XfstCompiler& minus_net();
  //! @brief Name top of stack
  //! @todo HFST automata do not remember their names
  XfstCompiler& name_net(const char* name);
  //! @brief Negate top of stack
  XfstCompiler& negate_net();
  //! @brief Kleene plus top network of stack
  XfstCompiler& one_plus_net();
  //! @brief Kleene star top network of stack
  XfstCompiler& zero_plus_net();
  //! @brief Prune top network of stack
  //! @todo Most of HFST automata are pruned by default?
  XfstCompiler& prune_net();
  //! @brief Reverse top network of the stack
  XfstCompiler& reverse_net();
  //! @brief Shuffle top network with second
  //! @todo unimplemented
  XfstCompiler& shuffle_net();
  //! @brief Sigma top network of stack
  //! @todo Find out wtf this is
  XfstCompiler& sigma_net();
  //! @brief Sort top network of the stack
  //! @todo HFST automata sort or not by default
  XfstCompiler& sort_net();
  //! @brief Substring top network of stack
  //! @todo unimplementedd
  XfstCompiler& substring_net();
  //! @brief Disjunct the stack
  XfstCompiler& union_net();
  //! @brief Interactive network traversal tool
  XfstCompiler& inspect_net();
  //! @brief Repeat 0..1 times
  XfstCompiler& optional_net();
  //! @brief Compile-replace lower
  //! @todo missing from HFST
  XfstCompiler& compile_replace_lower_net();
  //! @brief Compile-replace upper
  //! @todo missing from HFST
  XfstCompiler& compile_replace_upper_net();


  //! @brief Sekrit HFST raw command mode!
  XfstCompiler& hfst(const char* data);
  //! @brief Get current stack of compiler
  const std::stack<HfstTransducer*>& get_stack() const;
  //! @brief Parse from @a infile
  int parse(FILE* infile);
  //! @brief Parse @a filename
  int parse(const char* filename);
  //! @brief Parse @a line
  int parse_line(char line[]);
  int parse_line(std::string line);
  //! @brief Print prompts and XFST outputs
  XfstCompiler& setVerbosity(bool verbosity);
  //! @brief Print prompts
  XfstCompiler& setPromptVerbosity(bool verbosity);
  //! @brief Explicitly print the prompt
  const XfstCompiler& prompt() const;
  //! @brief Get the prompt
  char* get_prompt() const;

 protected:
  XfstCompiler& print_apply_prompt(ApplyDirection direction);
  XfstCompiler& print_symbol(const char* symbol, FILE* outfile=stdout);
  XfstCompiler& print_paths(const hfst::HfstTwoLevelPaths &paths, FILE* outfile=stdout);
  XfstCompiler& print_paths(const hfst::HfstOneLevelPaths &paths, FILE* outfile=stdout);

  //! @brief Perform lookup on the top transducer using strings in \a infile.
  //! \a direction specifies whether apply is done on input (up) or output (down) 
  //! side. If infile is stdin, interactive mode with prompts is used.
  //! The results are printed to standard output.
  XfstCompiler& apply(FILE* infile, ApplyDirection direction);

  //! @brief Apply \a operation on top transducer in the stack.
  //! If the stack is empty, print a warning.
  XfstCompiler& apply_unary_operation(UnaryOperation operation);
  //! @brief Apply \a operation on two top transducers in the stack.
  //! The top transducers are popped, the operation is applied 
  //! (the topmost transducer is the first transducer in the operation), 
  //! and the result is pushed to the top of the stack.
  //! If the stack has less than two transducers, print a warning.
  XfstCompiler& apply_binary_operation(BinaryOperation operation);

  //! @brief Apply \a operation on all transducers in the stack.
  //! The top transducer (n1) is popped, the operation is applied iteratively
  //! for all next transducers (n2, n3, n4 ...) in the stack:
  //! [[[n1 OPERATION n2] OPERATION n3] OPERATION n4] ...
  //! popping each of them and the result is pushed to the stack.
  //! If the stack is empty, print a warning.
  XfstCompiler& apply_binary_operation_iteratively(BinaryOperation operation);

  //! @brief Print the result of \a operation when applied to the whole stack.
  XfstCompiler& test_operation(TestOperation operation);

  //! @brief The topmost transducer in the stack.
  //! If empty, print a warning message and return NULL.
  HfstTransducer * top();

  //! @brief Get next line from \a file. Return NULL if end of file is reached.
  char * xfst_getline(FILE * file);

  //! @brief Get current readline history index.
  int current_history_index();

  //! @brief Remove all readline history after \a index. 
  void ignore_history_after_index(int index);

  private:
  /* */
  const XfstCompiler& error(const char* message) const;
  const XfstCompiler& print_transducer_info() const;
  XfstCompiler& add_prop_line(char* line);
  XfstCompiler& apply_line(char* line, ApplyDirection direction);
  XfstCompiler& apply_up_line(char* line);
  XfstCompiler& apply_down_line(char* line);
  XfstCompiler& apply_med_line(char* line);
  XfstCompiler& print_bool(bool value);
  XfstCompiler& read_prop_line(char* line);

  hfst::xre::XreCompiler xre_;
  hfst::lexc::LexcCompiler lexc_;
#if HAVE_TWOLC
  hfst::twolc::TwolcCompiler twolc_;
#endif
  std::map<std::string,hfst::HfstTransducer*> definitions_;
  std::map<std::string,hfst::HfstTransducer*> functions_;
  std::stack<hfst::HfstTransducer*> stack_;
  std::map<std::string,hfst::HfstTransducer*> names_;
  std::map<std::string,std::string> aliases_;
  std::map<std::string,std::string> variables_;
  std::map<std::string,std::string> properties_;
  std::map<std::string,std::list<string> > lists_;
  hfst::HfstTransducer* last_defined_;
  hfst::ImplementationType format_;
  bool verbose_;
  bool verbose_prompt_;
}
;

// ugh, the global
extern XfstCompiler* xfst_;
}}
// vim:set ft=cpp.doxygen:
#endif


