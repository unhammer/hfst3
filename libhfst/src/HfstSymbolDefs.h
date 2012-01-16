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

#ifndef _SYMBOL_DEFS_H_
#define _SYMBOL_DEFS_H_
#include <vector>
#include <string>
#include <map>
#include <set>

/** @file HfstSymbolDefs.h
    \brief Typedefs and functions for symbols, symbol pairs and 
    sets of symbols. */

namespace hfst
{

  /** \brief A UTF-8 symbol in a transition. 

    Strings <i>"@_EPSILON_SYMBOL_@"</i>, <i>"@_UNKNOWN_SYMBOL_@"</i> 
    and <i>"@_IDENTITY_SYMBOL_@"</i> are reserved.

    - <i>"@_EPSILON_SYMBOL_@"</i> denotes the epsilon (in AT&T formalism 
      <tt>[0]</tt>). 
    - <i>"@_UNKNOWN_SYMBOL_@"</i> denotes an unknown symbol, 
      i.e. any symbol that 
      does not occur or has not occurred in the transducer 
      (in AT&T formalism <tt>[?], [?:x], [x:?]</tt>). 
    - <i>"@_IDENTITY_SYMBOL_@"</i> denotes any unknown symbol that is the same
      on the input and output side of a transition (in AT&T formalism 
      <tt>[?:?]</tt>).

      For an example of special symbols, see HFST transducer
      <a href="classhfst_1_1HfstTransducer.html#symbols">
      documentation</a>.

    Strings of form <CODE> @[PNDRCU][.][A-Z]+([.][A-Z]+)?@ </CODE> 
    are reserved for flag diacritics. For an example of flag diacritics, see 
    #hfst::HfstTransducer::extract_paths_fd(hfst::HfstTwoLevelPaths&, int, int, bool) const.
    For more information on flag diacritics, see Finite State Morphology
    (Beesley & Karttunen, 2003).
   */
  typedef std::string String;

  /* A set of strings. */
  typedef std::set<String> StringSet;

  typedef std::vector<String> StringVector; 

  /** \brief A symbol pair in a transition. 

      @see HfstTransducer::substitute(const StringPair&, const StringPair&) 
      and other substitute functions */
  typedef std::pair<String, String> StringPair;

  /** \brief A vector of transitions that represents a path in a transducer. 

      @see HfstTokenizer */
  typedef std::vector<StringPair> StringPairVector;

  /** \brief A set of symbol pairs used in substituting symbol pairs 
      and in rule functions. 

     @see HfstTransducer::substitute(const StringPair&, const StringPairSet &)
     #hfst::rules */
  typedef std::set<StringPair> StringPairSet;

  /** \brief A map of substitutions used when performing multiple
      symbol-to-symbol substitutions. 

      @see HfstTransducer::substitute(const HfstSymbolSubstitutions&) */
  typedef std::map<String, String> HfstSymbolSubstitutions;

  /** \brief A map of substitutions used when performing multiple
      symbol pair-to-symbol pair substitutions. 

      @see HfstTransducer::substitute(const HfstSymbolPairSubstitutions&) */
  typedef std::map<StringPair, StringPair> HfstSymbolPairSubstitutions;
  
/* The internal representations */
  const std::string internal_epsilon = "@_EPSILON_SYMBOL_@";
  const std::string internal_unknown = "@_UNKNOWN_SYMBOL_@";
  const std::string internal_identity = "@_IDENTITY_SYMBOL_@";
  
  /* Check whether a string is equal to reserved internal representation. */
  bool is_epsilon(std::string const & str);
  bool is_unknown(std::string const & str);
  bool is_identity(std::string const & str);
  bool is_epsilon(const char * str);
  bool is_unknown(const char * str);
  bool is_identity(const char * str);

  /* For internal use */
  typedef std::pair<unsigned int, unsigned int> NumberPair;
  typedef std::vector<NumberPair> NumberPairVector;
  typedef std::set<NumberPair> NumberPairSet;
  typedef std::map<String,unsigned int> StringNumberMap;
  typedef std::map<unsigned int,unsigned int> NumberNumberMap;

  namespace symbols {
    void collect_unknown_sets(StringSet &s1, StringSet &unknown1,
                  StringSet &s2, StringSet &unknown2);
  }
}
#endif
