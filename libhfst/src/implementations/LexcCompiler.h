//! @file LexcCompiler.h
//!
//! @brief Functions for building trie representation of lexc data
//!
//! LexcCompiler contains a finite state compilater for objects that resemble
//! Xerox lexc style lexicons. This compiler is modeled in LexcCompiler class.
//! LexcCompiler tries to be a minimal implementation needed to clone original
//! lexc functionality, it supports only the methods necessary to implement
//! original lexc, e.g. deleting entries from a lexicon during compilation is
//! not implemented.

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

#ifndef GUARD_LexcCompiler_h
#define GUARD_LexcCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>

#include "../HfstTransducer.h"
#include "XreCompiler.h"

namespace hfst {
//! @brief Namespace for Xerox LexC related specific functions and classes.
namespace lexc {

//! @brief A compiler holding information contained in lexc style lexicons.
//! A single LexcCompiler can be extended by adding entries to it, but little
//! else can be done with it. It is sufficient to implement clone of lexc.
class LexcCompiler
{
  public:
  //! @brief create a lexc compiler for unspecified transducer format.
  LexcCompiler();

  //! @brief create a lexc compiler with @c impl as transducer format.
  LexcCompiler(hfst::ImplementationType impl);

  //! @brief compile lexc description from @c infile into current compiler
  LexcCompiler& parse(FILE* infile);

  //! @brief compile lexc description from file @a filename into current
  //!        compiler.
  LexcCompiler& parse(const char* filename);

  //! @brief set verbosity options
  LexcCompiler& setVerbosity(bool verbose);

  //! @brief add @a alphabet to multicharacter symbol set.
  //!        This may include regular expression ? for backends that do not
  //!        support open alphabets.
  LexcCompiler& addAlphabet(const std::string& alphabet);

  //! @brief set current processing lexicon name to @a lexicon_name.
  LexcCompiler& setCurrentLexiconName(const std::string& lexicon_name);

  //! @brief add entry defined by a @a entry to current lexicon, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addStringEntry(const std::string& entry, const std::string& continuation,
                      const double weight);

  //! @brief add entry defined by @a upper:@a lower, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addStringPairEntry(const std::string& upper, const std::string& lower,
          const std::string& continuation, const double weight);

  //! @brief add entry defined by regular expression @a xre, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addXreEntry(const std::string& xre, const std::string& continuation, 
                   const double weight);

  //! @brief add macro definition named @a name matching regular expression
  //! @a xre to known xerox regular expressions.
  LexcCompiler& addXreDefinition(const std::string& name, const std::string& xre);

  //! @brief set start lexicon's name to @a lexicon_name.
  LexcCompiler& setInitialLexiconName(const std::string& lexicon_name);

  //! @brief create final usable version of current lexicons and entries.
  hfst::HfstTransducer* compileLexical();

  //! @brief get trie formed by current string entries
  const std::map<std::string,hfst::HfstTransducer>& getStringTries() const;

  //! @brief get union formed by current regular expression entries
  const std::map<std::string,hfst::HfstTransducer>& getRegexpUnions() const;

  //! @brief check that current morphotax is connected and print anomalies.
  //! Works like xerox lexc, for compatibility.
  const LexcCompiler& printConnectedness() const;

  private:
  bool quiet_;
  hfst::ImplementationType format_;
  hfst::HfstTokenizer tokenizer_;
  hfst::xre::XreCompiler xre_;
  std::string initialLexiconName_;
  std::map<std::string,hfst::HfstTransducer> stringTries_;
  std::map<std::string,hfst::HfstTransducer> regexps_;
  std::set<std::string> lexiconNames_;
  std::set<std::string> continuations_;
  std::string currentLexiconName_;
  size_t totalEntries_;
  size_t currentEntries_;
}
;

// ugh, the global
extern LexcCompiler* lexc_;
} }

// vim:set ft=cpp.doxygen:
#endif

