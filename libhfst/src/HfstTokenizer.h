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

#ifndef _HFST_TOKENIZER_H_
#define _HFST_TOKENIZER_H_
#include "HfstSymbolDefs.h"
#include <iostream>
#include <climits>
#include <string>

/** @file HfstTokenizer.h
    \brief Declaration of class #hfst::HfstTokenizer. */

namespace hfst
{
  using hfst::String;
  using hfst::StringSet;
  using hfst::StringPair;
  using hfst::StringPairVector;

  // Copied from HfstDataTypes.h because including the file
  // causes problems with header file #ifndef _HEADER_FILE_H_ guards
  typedef std::vector<std::string> StringVector;
  typedef std::pair<StringVector,float> HfstOneLevelPath;

  class MultiCharSymbolTrie;
  typedef std::vector<MultiCharSymbolTrie*> MultiCharSymbolTrieVector;
  typedef std::vector<bool> SymbolEndVector;

  class MultiCharSymbolTrie
  {
  private:
    MultiCharSymbolTrieVector symbol_rests;
    SymbolEndVector is_leaf;
    bool is_end_of_string(const char * p) const ;
    void set_symbol_end(const char * p);
    void init_symbol_rests(const char * p);
    void add_symbol_rest(const char * p);
    bool is_symbol_end(const char * p) const;
    MultiCharSymbolTrie * get_symbol_rest_trie(const char * p) const;

  public:
    MultiCharSymbolTrie(void);
    ~MultiCharSymbolTrie(void);
    void add(const char * p);
    const char * find(const char * p) const;  
  };
  
  /** \brief A tokenizer for creating transducers from UTF-8 strings.

      Strings are tokenized from left to right using longest match tokenization.
      For example, if the tokenizer contains a multicharacter symbol 
      "foo" and a skip symbol "fo",
      the string "foo" is tokenized as "foo:foo".
      If the tokenizer contains a multicharacter symbol "fo" and a skip 
      symbol "foo",
      the string "foo" is tokenized as an empty string.

      An example:
\verbatim
      HfstTokenizer TOK;
      TOK.add_multichar_symbol("<br />");
      TOK.add_skip_symbol("<p>");
      TOK.add_skip_symbol("</p>");
      StringPairVector spv = TOK.tokenize("<p>A<br />paragraph!</p>");
      // spv now contains
      //    A:A <br />:<br /> p:p a:a r:r a:a g:g r:r a:a p:p h:h !:!
\endverbatim

      @note The tokenizer only tokenizes utf-8 strings. 
      Special symbols (see #String) are not included in the tokenizer 
      unless added to it.

      @see {hfst::HfstTransducer::HfstTransducer
      (const std::string&, const HfstTokenizer&, ImplementationType type)} */
  class HfstTokenizer
  {  
  private:
    MultiCharSymbolTrie multi_char_symbols;
    StringSet skip_symbol_set;
    int get_next_symbol_size(const char * symbol) const;
    bool is_skip_symbol(String &s) const;

  public:

    /** \brief Create a tokenizer that recognizes utf-8 symbols. */
    HfstTokenizer();

    /** \brief Add a symbol to be skipped to this tokenizer. 

        After skipping a symbol, tokenization is always started again.
        For example if we have a multicharacter symbol "foo" and a 
        skip symbol "bar", the string "fobaro" will be tokenized 
        "f" "o" "o", not "foo". */
    void add_skip_symbol(const std::string &symbol);

    /** \brief Add a multicharacter symbol \a symbol to this tokenizer. 

        If a multicharacter symbol has a skip symbol inside it, it is
        not considered a multicharacter symbol. For example if we have 
        a multicharacter symbol "foo" and a skip symbol "bar", the string
        "fobaro" will be tokenized "f" "o" "o", not "foo". */
    void add_multichar_symbol(const std::string& symbol);

    /** \brief Tokenize the string \a input_string. */
    StringPairVector tokenize(const std::string &input_string) const;

    /** \brief Tokenize the string \a input_string. */
    StringVector tokenize_one_level(const std::string &input_string) const;

    /** \brief Tokenize the string pair \a input_string : \a output_string. 

        If one string has more tokens than the other, epsilons will be
        inserted to the end of the tokenized string with less tokens
        so that both tokenized strings have the same number of tokens.
     */
    StringPairVector tokenize(const std::string &input_string,
                              const std::string &output_string) const;
  };
}
#endif
