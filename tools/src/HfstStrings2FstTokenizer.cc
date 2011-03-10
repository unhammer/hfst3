#include "HfstStrings2FstTokenizer.h"
#include <algorithm>

HfstStrings2FstTokenizer::HfstStrings2FstTokenizer
(StringVector &multichar_symbols,const std::string &eps):
  eps(eps)
{
  // \: \\ \<space> and eps are special cases.
  tokenizer.add_multichar_symbol( BACKSLASH COL );
  tokenizer.add_multichar_symbol( BACKSLASH SPACE );
  tokenizer.add_multichar_symbol( BACKSLASH BACKSLASH );
  tokenizer.add_multichar_symbol(eps);
  add_multichar_symbol_head(eps);

  for (StringVector::const_iterator it = multichar_symbols.begin();
       it != multichar_symbols.end();
       ++it)
    {
      add_multichar_symbol_head(*it);
      add_multichar_symbol(*it);
    }

}

void HfstStrings2FstTokenizer::add_multichar_symbol
(const std::string &multichar_symbol)
{ tokenizer.add_multichar_symbol(multichar_symbol); }

void HfstStrings2FstTokenizer::add_multichar_symbol_head
(const std::string &multichar_symbol)
{
  if (multichar_symbol.empty())
    { throw EmptyMulticharSymbol(); }
  StringVector tokenized_multichar_symbol = 
    tokenizer.tokenize_one_level(multichar_symbol);
  std::string multichar_symbol_head(*tokenized_multichar_symbol.begin());
  tokenizer.add_multichar_symbol
    (std::string(BACKSLASH) + multichar_symbol_head);
}

StringPairVector HfstStrings2FstTokenizer::tokenize_pair_string
(const std::string &str,bool spaces)
{
  StringVector tokenized_str;
  if (spaces)
    { tokenized_str = split_at_spaces(str); }
  else
    {
      tokenized_str = tokenizer.tokenize_one_level(str);
      StringVector::iterator new_end = 
	std::remove(tokenized_str.begin(),tokenized_str.end(),BACKSLASH);
      tokenized_str.erase(new_end,tokenized_str.end());
    }
  return make_pair_vector(tokenized_str,spaces);
}

StringPairVector HfstStrings2FstTokenizer::tokenize_string_pair
(const std::string &str,bool spaces)
{
  StringVector tokenized_str;
  if (spaces)
    { tokenized_str = split_at_spaces(str); }
  else
    { tokenized_str = tokenizer.tokenize_one_level(str); }
  StringVector::iterator it;
  if ((it = find(tokenized_str.begin(),tokenized_str.end(),COL)) ==
      tokenized_str.end())
    { return make_pair_vector(tokenized_str,tokenized_str); }
  else
    { return make_pair_vector
	(StringVector(tokenized_str.begin(),it),
	 StringVector(it+1,tokenized_str.end())); }
}

StringPairVector HfstStrings2FstTokenizer::make_pair_vector
(const StringVector &v,bool spaces)
{
  StringPairVector spv;
  if (spaces)
    {
      for (StringVector::const_iterator it = v.begin();
	    it != v.end();
	    ++it)
	 {
	   int pos;
	   if ((pos = get_col_pos(*it)) >= 0)
	     { 
	       std::string input = it->substr(0,pos);
	       input = (input.empty() or input == eps ? 
			EPSILON_SYMBOL : unescape(input));
	       std::string output = it->substr(pos+1);
 	       output = (output.empty() or output == eps ? 
			 EPSILON_SYMBOL : unescape(output));
	       spv.push_back(StringPair(input,output));
	     }
	   else
	     { 
	       std::string symbol = 
		 (it->empty() or *it == eps ? EPSILON_SYMBOL : 
		  unescape(*it));
	       spv.push_back(StringPair(symbol,symbol)); 
	     }
	 }
    }
  else
    {
       for (StringVector::const_iterator it = v.begin();
	    it != v.end();
	    ++it)
	 {
	   if (not is_pair_input_symbol(it,v.end()))
	     { 
	       std::string symbol = unescape(*it);
	       symbol = (symbol.empty() or symbol == eps ? 
			 EPSILON_SYMBOL : symbol);
	       spv.push_back(StringPair(symbol,symbol)); }
	   else
	     {
	       std::string input = (it->empty() or *it == eps ? 
				    EPSILON_SYMBOL : unescape(*it));
	       ++(++it);
	       std::string output = (it->empty() or *it == eps ? 
				     EPSILON_SYMBOL : unescape(*it));
	       spv.push_back(StringPair(input,output));
	     }
	}
       if (not spv.empty())
	 {
	   if (spv.begin()->first == COL and spv.begin()->second == COL)
	     { spv[0] = StringPair(EPSILON_SYMBOL,EPSILON_SYMBOL);}
	   if (spv.back().first == COL and spv.back().second == COL)
	     { spv.back() = 
		 StringPair(EPSILON_SYMBOL,EPSILON_SYMBOL);}
	 }
    }
  return spv;
}

StringPairVector HfstStrings2FstTokenizer::make_pair_vector
(const StringVector &input, const StringVector &output)
{
  StringPairVector spv;
  StringVector::const_iterator input_it = input.begin();
  StringVector::const_iterator output_it = output.begin();
  while (input_it != input.end() and output_it != output.end())
    { 
      spv.push_back
	(StringPair(input_it->empty() or *input_it == eps ? 
		    EPSILON_SYMBOL : unescape(*input_it),
		    output_it->empty() or *output_it == eps ? 
		    EPSILON_SYMBOL : unescape(*output_it))); 
      ++input_it;
      ++output_it;
    }
  if (input_it == input.end())
    {
      for ( ; output_it != output.end(); ++output_it)
	{ spv.push_back
	    (StringPair(EPSILON_SYMBOL,
			output_it->empty() or *output_it == eps ? 
			EPSILON_SYMBOL : unescape(*output_it))); }
    }
  else
    {
      for ( ; input_it != input.end(); ++input_it)
	{ spv.push_back
	    (StringPair(input_it->empty() or *input_it == eps ? 
			EPSILON_SYMBOL : unescape(*input_it),
			EPSILON_SYMBOL)); }
    }
  return spv;
}

std::string HfstStrings2FstTokenizer::unescape(std::string symbol)
{
  if (symbol == (BACKSLASH BACKSLASH))
    { return BACKSLASH; }

  size_t pos = 0;
  while ((pos = symbol.find(BACKSLASH BACKSLASH)) != std::string::npos)
    { symbol.replace(pos,2,BACKSLASH_ESC); }

  pos = 0;
  while ((pos = symbol.find(BACKSLASH)) != std::string::npos)
    { symbol.replace(pos,1,EMPTY); }

  pos = 0;
  while ((pos = symbol.find(BACKSLASH_ESC)) != std::string::npos)
    { symbol.replace(pos,strlen(BACKSLASH_ESC),EMPTY); }
  
  return symbol;
}

bool HfstStrings2FstTokenizer::is_pair_input_symbol
(StringVector::const_iterator it,StringVector::const_iterator end)
{
  if (it == end)
    { return false; }
  ++it;
  if (it == end)
    { return false; }
  if (*it != COL)
    { return false; }
  ++it;
  if (it == end)
    { return false; }
  return true;
}

int HfstStrings2FstTokenizer::get_col_pos(const std::string &str)
{
  if (str.empty())
    { return -1; }
  if (str[0] == COL_CHAR)
    { return 0; }
  for (size_t i = 1; i < str.size(); ++i)
    {
      if (str[i] == COL_CHAR && str[i-1] != BACKSLASH_CHAR)
	{ return i; }
    }
  return -1;
}

StringVector HfstStrings2FstTokenizer::split_at_spaces(const std::string &str)
{
  std::string symbol;
  StringVector sv = tokenizer.tokenize_one_level(str);
  StringVector res;
  for (StringVector::const_iterator it = sv.begin(); it != sv.end(); ++it)
    {
      if (*it == SPACE and not symbol.empty())
	  { 
	    res.push_back(symbol);
	    while (it + 1 != sv.end() and *(it + 1) == SPACE)
	      { ++it; }
	    symbol = EMPTY;
	    if (it == sv.end())
	      { break; }
	  }
      else
	{ symbol += *it; }
    }
  if (not symbol.empty())
    { res.push_back(symbol); }
  return res;
}

#ifdef TEST_FST_2_STRINGS_TOKENIZER
void test_ps
(const std::string &input,HfstStrings2FstTokenizer &tokenizer,bool spaces)
{
  std::cout << "Tokenizing: " << input << std::endl;
  std::cout << "Tokenized:" << std::endl;
  StringPairVector spv = tokenizer.tokenize_pair_string(input,spaces);
  for (StringPairVector::const_iterator it = spv.begin();
       it != spv.end();
       ++it)
    { 
      if (it->first != it->second)	
	{ std::cout << it->first << " : " << it->second << std::endl; }
      else
	{ std::cout << it->first << std::endl; }
    }
  std::cout << std::endl;
}

void test_sp
(const std::string &input,HfstStrings2FstTokenizer &tokenizer,bool spaces)
{
  std::cout << "Tokenizing: " << input << std::endl;
  std::cout << "Tokenized:" << std::endl;
  StringPairVector spv = tokenizer.tokenize_string_pair(input,spaces);
  for (StringPairVector::const_iterator it = spv.begin();
       it != spv.end();
       ++it)
    { 
      if (it->first != it->second)	
	{ std::cout << it->first << " : " << it->second << std::endl; }
      else
	{ std::cout << it->first << std::endl; }
    }
  std::cout << std::endl;
}

int main(void)
{
  StringVector multichar_symbols;
  multichar_symbols.push_back("##");
  multichar_symbols.push_back("+NOM");
  multichar_symbols.push_back(":NOM:SG");
  HfstStrings2FstTokenizer tokenizer(multichar_symbols,"@_EPS_@");
  test_ps("@_EPS_@:xa:b\\::c\\\\d:\\e:NOM:SG:\\+NOM:",tokenizer,false);
  test_ps("@_EPS_@:x a:b   \\::c \\\\ d:\\e \\:NOM\\:SG\\::\\+ N O M : ",
	  tokenizer,true);
  test_sp("@_EPS_@koira@_EPS_@@_EPS_@:##koira:NOM:SG##",tokenizer,false);
  test_sp("@_EPS_@koira@_EPS_@@_EPS_@:##koira\\:NOM:SG##",tokenizer,false);
  test_sp("@_EPS_@koira@_EPS_@@_EPS_@:##koira\\\\:NOM:SG##",tokenizer,false);
  test_sp("@_EPS_@ k o i r  a @_EPS_@ @_EPS_@ : ## k o i   r  a :NOM:SG ##",tokenizer,true);
  test_sp("@_EPS_@ k o i r  a \\  @_EPS_@ @_EPS_@ : ## k o i   r  a :NOM:SG ##",tokenizer,true);
}
#endif // TEST_FST_2_STRINGS_TOKENIZER
