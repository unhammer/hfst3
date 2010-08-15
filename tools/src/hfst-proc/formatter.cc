#include <algorithm>
#include <sstream>
#include "formatter.h"

//////////Function definitions for OutputFormatter

TokenVector
OutputFormatter::clear_superblanks(const TokenVector& tokens) const
{
  TokenVector output_tokens;
  for(TokenVector::const_iterator it=tokens.begin(); it!=tokens.end(); it++)
  {
    if(it->type == Superblank)
      output_tokens.push_back(Token::as_symbol(token_stream.get_alphabet().get_blank_symbol()));
    else
      output_tokens.push_back(*it);
  }
  return output_tokens;
}

void
OutputFormatter::filter_compound_analyses(LookupPathSet& finals) const
{
  int fewest_boundaries = std::numeric_limits<int>::max();
  std::vector<int> boundary_counts;
  
  // first look to find the analysis with the fewest compound boundaries
  for(LookupPathSet::const_iterator it=finals.begin(); it!=finals.end(); it++)
  {
    int num = token_stream.get_alphabet().num_compound_boundaries((*it)->get_output_symbols());
    boundary_counts.push_back(num);
    if(num < fewest_boundaries)
      fewest_boundaries = num;
  }
  
  // filter all analyses with more boundaries than the minimum
  int i=0;
  for(LookupPathSet::iterator it=finals.begin(); it!=finals.end();i++)
  {
    if(boundary_counts[i] > fewest_boundaries)
    {
      LookupPathSet::iterator to_delete = it;
      it++;
      finals.erase(to_delete);
      if(printDebuggingInformationFlag)
        std::cout << "Filtering compound analysis with " << boundary_counts[i] << " boundary(s)" << std::endl;
    }
    else
      it++;
  }
}

LookupPathSet
OutputFormatter::preprocess_finals(const LookupPathSet& finals) const
{
  LookupPathSet new_finals = LookupPathSet(finals);
  if(do_compound_filtering)
  {
    filter_compound_analyses(new_finals);
    if(printDebuggingInformationFlag)
    {
      if(new_finals.size() < finals.size())
        std::cout << "Filtered " << finals.size()-new_finals.size() << " compound analyses" << std::endl;
    }
  }
  
  if(new_finals.size() > (unsigned int)maxAnalyses)
  {
    LookupPathSet clipped_finals(LookupPath::compare_pointers);
    LookupPathSet::const_iterator it=new_finals.begin();
    for(int i=0;i<maxAnalyses;i++,it++)
      clipped_finals.insert(*it);
    return clipped_finals;
  }
  else
    return new_finals;
}

//////////Function definitions for ApertiumOutputFormatter

std::set<std::string>
ApertiumOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  std::set<std::string> results;
  LookupPathSet new_finals = preprocess_finals(finals);
  
  for(LookupPathSet::const_iterator it=new_finals.begin(); it!=new_finals.end(); it++)
  {
    std::ostringstream res;
    res << token_stream.get_alphabet().symbols_to_string((*it)->get_output_symbols(), caps);
    if(dynamic_cast<const LookupPathW*>(*it) != NULL && displayWeightsFlag)
      res << '~' << dynamic_cast<const LookupPathW*>(*it)->get_weight() << '~';
    
    results.insert(res.str());
  }
  return results;
}

void
ApertiumOutputFormatter::print_word(const TokenVector& surface_form, 
                                  std::set<std::string> const &analyzed_forms) const
{
  // any superblanks in the surface form should not be printed as part of the
  // analysis output, but should be output directly afterwards
  TokenVector output_surface_form;
  std::vector<unsigned int> superblanks;
  for(TokenVector::const_iterator it=surface_form.begin(); it!=surface_form.end(); it++)
  {
    if(it->type == Superblank)
    {
      output_surface_form.push_back(Token::as_symbol(token_stream.to_symbol(*it)));
      superblanks.push_back(it->superblank_index);
    }
    else
      output_surface_form.push_back(*it);
  }

  if(printDebuggingInformationFlag)
    std::cout << "surface_form consists of " << output_surface_form.size() << " tokens" << std::endl;
  
  token_stream.ostream() << '^';
  token_stream.write_escaped(output_surface_form);
  for(std::set<std::string>::const_iterator it=analyzed_forms.begin(); it!=analyzed_forms.end(); it++)
    token_stream.ostream() << "/" << *it;
  token_stream.ostream() << "$";
  
  for(size_t i=0;i<superblanks.size();i++)
    token_stream.ostream() << token_stream.get_superblank(superblanks[i]);
}
void
ApertiumOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  token_stream.ostream() << '^';
  token_stream.write_escaped(surface_form);
  token_stream.ostream() << "/*";
  token_stream.write_escaped(surface_form);
  token_stream.ostream() << '$';
}


//////////Function definitions for CGOutputFormatter

std::string
CGOutputFormatter::process_final(const SymbolNumberVector& symbols, CapitalizationState caps) const
{
  std::ostringstream res;
  size_t start_pos = 0;
  
  res << '"'; // before start of lexical form
  
  while(start_pos < symbols.size())
  {
    size_t tag_start = symbols.size();
    size_t compound_split = symbols.size();
    for(size_t i=start_pos; i<symbols.size(); i++)
    {
      if(tag_start == symbols.size() &&
         token_stream.get_alphabet().is_tag(symbols[i]))
        tag_start = i;
      
      if(compound_split == symbols.size())
      {
        std::string s = token_stream.get_alphabet().symbol_to_string(symbols[i]);
        if(s == "#" || s == "+" || s[s.length()-1] == '+')
          compound_split = i;
      }
      
      if(tag_start != symbols.size() && compound_split != symbols.size())
        break;
    }
    
    // grab the base form without tags
    res << token_stream.get_alphabet().symbols_to_string(
      SymbolNumberVector(symbols.begin()+start_pos,symbols.begin()+tag_start), caps);
    
    // look for compounding. Don't output the tags for non-final segments
    if(compound_split != symbols.size())
    {
      res << "#";
      start_pos = compound_split+1;
    }
    else
    {
      res << '"'; // after end of lexical form, now come any tags
      if(tag_start != symbols.size())
      {
        for(size_t i=tag_start; i<symbols.size(); i++)
        {
          std::string tag = token_stream.get_alphabet().symbol_to_string(symbols[i]);
          // remove the < and >
          if(tag.size() > 0 && tag[0] == '<')
          {
            tag = tag.substr(1);
            if(tag.size() > 0 && tag[tag.length()-1] == '>')
              tag = tag.substr(0,tag.length()-1);
          }
          
          res << (i==tag_start?"\t":" ") << tag;
        }
      }
      
      break;
    }
  }
  
  return res.str();
}

std::set<std::string>
CGOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  std::set<std::string> results;
  LookupPathSet new_finals = preprocess_finals(finals);
  
  for(LookupPathSet::const_iterator it=new_finals.begin(); it!=new_finals.end(); it++)
    results.insert(process_final((*it)->get_output_symbols(), caps));
  
  return results;
}

void
CGOutputFormatter::print_word(const TokenVector& surface_form,
                                std::set<std::string> const &analyzed_forms) const
{
  token_stream.ostream() << "\"<"
                         << token_stream.tokens_to_string(clear_superblanks(surface_form))
                         << ">\"" << std::endl;
  
  for(std::set<std::string>::const_iterator it=analyzed_forms.begin(); it!=analyzed_forms.end(); it++)
    token_stream.ostream() << "\t" << *it << std::endl;
}

void
CGOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  std::string form = token_stream.tokens_to_string(clear_superblanks(surface_form));
  token_stream.ostream() << "\"<" << form << ">\"" << std::endl
                         << "\t\"*" << form << "\"" << std::endl;
}


//////////Function definitions for XeroxOutputFormatter

std::set<std::string>
XeroxOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  std::set<std::string> results;
  LookupPathSet new_finals = preprocess_finals(finals);
  
  for(LookupPathSet::const_iterator it=new_finals.begin(); it!=new_finals.end(); it++)
  {
    std::ostringstream res;
    res << token_stream.get_alphabet().symbols_to_string((*it)->get_output_symbols(), caps);
    if(dynamic_cast<const LookupPathW*>(*it) != NULL && displayWeightsFlag)
      res << "\t" << dynamic_cast<const LookupPathW*>(*it)->get_weight();
    
    results.insert(res.str());
  }
  return results;
}

void
XeroxOutputFormatter::print_word(const TokenVector& surface_form,
                                std::set<std::string> const &analyzed_forms) const
{
  std::string surface = token_stream.tokens_to_string(clear_superblanks(surface_form));
  
  for(std::set<std::string>::const_iterator it=analyzed_forms.begin(); it!=analyzed_forms.end(); it++)
    token_stream.ostream() << surface << "\t" << *it << std::endl;
  token_stream.ostream() << std::endl;
}

void
XeroxOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  token_stream.ostream() << token_stream.tokens_to_string(clear_superblanks(surface_form))
                         << "\t+?" << std::endl << std::endl;
}

