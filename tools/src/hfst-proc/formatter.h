#ifndef _HFST_PROC_FORMATTER_H_
#define _HFST_PROC_FORMATTER_H_

#include "lookup-path.h"
#include "tokenizer.h"

/**
 * Abstract base class for handling the outputting of lookup results. Subclasses
 * implement different formatting systems.
 */
class OutputFormatter
{
 protected:
  TokenIOStream& token_stream;
  bool do_compound_filtering;
  
  /**
   * Return a copy of the given token vector which has all superblanks
   * converted to blanks
   */
  TokenVector clear_superblanks(const TokenVector& tokens) const;
  
  /**
   * Filter any analyses with more compound-boundaries than the minimum
   * boundary count of any analysis in the set
   */
  void filter_compound_analyses(LookupPathSet& finals) const;
  
  /**
   * Return a sorted copy of the path vector that contains no more than
   * maxAnalyses entries
   */
  LookupPathSet preprocess_finals(const LookupPathSet& finals) const;
 public:
  OutputFormatter(TokenIOStream& s, bool f): token_stream(s), do_compound_filtering(f) {}
  virtual ~OutputFormatter() {}
  
  /**
   * Take a list of lookup paths that end in final states, and produce a list of
   * string representations of the paths that can be written to the output
   * @param finals a list of lookup paths ending in final states
   * @param state the capitalization of the surface form
   */
  virtual std::set<std::string> process_finals(const LookupPathSet& finals,
                                                  CapitalizationState state) const = 0;
  virtual void print_word(const TokenVector& surface_form, 
                          std::set<std::string> const &analyzed_forms) const = 0;
  virtual void print_unknown_word(const TokenVector& surface_form) const = 0;
  
  /**
   * Whether non-alphabetic characters that aren't in the transducer should be
   * passed through to the output or not
   */
  virtual bool preserve_nonalphabetic() const = 0;
};

class ApertiumOutputFormatter: public OutputFormatter
{
 public:
  ApertiumOutputFormatter(TokenIOStream& s, bool f): OutputFormatter(s,f) {}
  
  std::set<std::string> process_finals(const LookupPathSet& finals,
                                          CapitalizationState state) const;
  void print_word(const TokenVector& surface_form, 
                  std::set<std::string> const &analyzed_forms) const;
  void print_unknown_word(const TokenVector& surface_form) const;
  
  bool preserve_nonalphabetic() const {return true;}
};

class CGOutputFormatter: public OutputFormatter
{
  std::string process_final(const SymbolNumberVector& symbols, CapitalizationState caps) const;
 public:
  CGOutputFormatter(TokenIOStream& s, bool f): OutputFormatter(s,f) {}
  
  std::set<std::string> process_finals(const LookupPathSet& finals,
                                          CapitalizationState caps) const;
  void print_word(const TokenVector& surface_form, 
                  std::set<std::string> const &analyzed_forms) const;
  void print_unknown_word(const TokenVector& surface_form) const;
  
  bool preserve_nonalphabetic() const {return false;}
};

class XeroxOutputFormatter: public OutputFormatter
{
 public:
  XeroxOutputFormatter(TokenIOStream& s, bool f): OutputFormatter(s,f) {}
  
  std::set<std::string> process_finals(const LookupPathSet& finals,
                                          CapitalizationState state) const;
  void print_word(const TokenVector& surface_form, 
                  std::set<std::string> const &analyzed_forms) const;
  void print_unknown_word(const TokenVector& surface_form) const;
  
  bool preserve_nonalphabetic() const {return false;}
};

#endif
