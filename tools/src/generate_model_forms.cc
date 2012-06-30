#include "generate_model_forms.h"

#ifndef MAIN_TEST

#include "guessify_fst.h"

#include <algorithm>
#include <fstream>

using hfst::HfstOneLevelPaths;
using hfst::HfstOneLevelPath;

using hfst::TROPICAL_OPENFST_TYPE;

InvalidModelLine::InvalidModelLine(const std::string &line):
  line(line)
{}

std::ostream &operator<<(std::ostream &out,const StringVector &v)
{
  for (StringVector::const_iterator it = v.begin();
       it != v.end();
       ++it)
    { 
      out << *it;
    }

  return out;
}

HfstTokenizer get_alphabet_string_tokenizer(HfstTransducer &fst)
{
  // FIXME: temporary until optimized lookup transducers implement
  // get_alphabet.
  HfstTransducer temp(fst);
  temp.convert(TROPICAL_OPENFST_TYPE);

  StringSet alphabet = temp.get_alphabet();
  
  HfstTokenizer tokenizer;

  for (StringSet::const_iterator it = alphabet.begin();
       it != alphabet.end();
       ++it)
    { tokenizer.add_multichar_symbol(*it); }

  return tokenizer;
}

StringVector get_analysis_prefix(const StringVector &reversed_analysis)
{
  StringVector prefix;

  // We want a prefix of a reversed string, so we iterate from end to
  // beginning.
  for (StringVector::const_reverse_iterator it = reversed_analysis.rbegin();
       it != reversed_analysis.rend();
       ++it)
    {
      prefix.push_back(*it);
      
      if (is_cathegory_symbol(*it))
	{ 
	  // When we return, we have to reverse the result.
	  return StringVector(prefix.rbegin(), prefix.rend()); 
	}
    }
  
  // It should actually be impossible to get here, since valid
  // analyses contain at least one cathegory symbol.
  return prefix;
}

StringVector join(StringVector sv1, const StringVector &sv2)
{
  sv1.insert(sv1.end(), sv2.begin(), sv2.end());
  return sv1;
}

bool contains_analysis_symbols(const StringVector &word_form)
{
  for (StringVector::const_iterator it = word_form.begin();
       it != word_form.end();
       ++it)
    {
      const std::string &symbol = *it;
      if (symbol.size() > 1 and 
	  symbol[0] == '[' and 
	  symbol[symbol.size() - 1] == ']')
	{ return true; }
    }
  
  return false;
}

StringVector generate_word_forms(const StringVector &analysis,
				 HfstTransducer &form_generator)
{
  HfstOneLevelPaths * word_forms = form_generator.lookup(analysis);

  StringVector results;

  bool first_form = true;

  size_t num = 1;
  
  for (HfstOneLevelPaths::const_iterator it = word_forms->begin();
       it != word_forms->end();
       ++it)
    {
      if (num > MAX_GENERATED_FORMS)
	{ break; }

      const StringVector &word_form = it->second;
      
      if (contains_analysis_symbols(word_form))
	{ continue; }

      if (not first_form)
	{ results.push_back(", "); }
            
      // The word form is reversed, so we start from the end and
      // iterate to the beginning.
      results.insert(results.end(), word_form.rbegin(), word_form.rend());
      
      first_form = false;

      ++num;
    }

  delete word_forms;

  if (results.empty())
    { 
      results.push_back("<no word forms>"); 
    }
  


  return results;
}

StringVectorVector get_model_forms(const StringVector &reversed_analysis,
				   const StringVectorVector &model_forms,
				   HfstTransducer &form_generator)
{
  StringVector reversed_analysis_prefix = 
    get_analysis_prefix(reversed_analysis);

  StringVectorVector results;

  for (StringVectorVector::const_iterator it  = model_forms.begin();
       it != model_forms.end();
       ++it)
    {
      StringVector model_analysis = join(*it, reversed_analysis_prefix);

      results.push_back(generate_word_forms(model_analysis,
					    form_generator));
    }

  return results;
}

StringPair split(const std::string &line,const std::string &separator)
{
  size_t separator_pos = line.find(separator);
  
  return StringPair(line.substr(0,separator_pos),
		    line.substr(separator_pos + 1));
}

StringVector read_model_form(std::istream &in, HfstTokenizer &tokenizer)
{
  std::string line;
  std::getline(in,line);

  if (line.find(MODEL_FORM_PREFIX) != 0)
    { throw InvalidModelLine(line); }

  std::string model_form = line.substr(std::string(MODEL_FORM_PREFIX).size());

  StringVector tokenized_model_form = tokenizer.tokenize_one_level(model_form);
  std::reverse(tokenized_model_form.begin(),tokenized_model_form.end());

  return tokenized_model_form;
}

StringVectorVector read_model_forms(const std::string &model_form_filename,
				    HfstTokenizer &tokenizer)
{
  std::ifstream in(model_form_filename.c_str());

  if (in.peek() == EOF)
    { throw InvalidModelFile(); }

  StringVectorVector results;
  
  while (in.peek() != EOF)
    {
      results.push_back(read_model_form(in, tokenizer));
    }
  
  return results;
}


StringVectorVector get_guesses(const std::string &word_form,
			       HfstTransducer &guesser,
			       size_t number_of_guesses,
			       HfstTokenizer &tokenizer)
{
  StringVector tokenized_line = tokenizer.tokenize_one_level(word_form);
  std::reverse(tokenized_line.begin(),tokenized_line.end());

  HfstOneLevelPaths * paths = guesser.lookup_fd(tokenized_line);

  StringVectorVector results;

  size_t num = 1;

  for (HfstOneLevelPaths::const_iterator it = paths->begin();
       it != paths->end();
       ++it)
    { 
      if (num > number_of_guesses)
	{ break; }
      results.push_back(it->second); 
      ++num;
    }

  return results;
}

StringVectorVector get_paradigms(const std::string &word_form,
				 const StringVectorVector &guesses,
				 HfstTransducer &generator,
				 const StringVectorVector &model_forms,
				 size_t number_of_generated_forms)
{
  StringVectorVector paradigm_guesses;

  for (StringVectorVector::const_iterator it = guesses.begin();
       it != guesses.end();
       ++it)
    { 
      StringVector analysis_guess = *it;

      StringVectorVector results = get_model_forms(analysis_guess,
						   model_forms,
						   generator);
	  
      StringVector paradigm;
      paradigm.push_back(word_form);
      paradigm.push_back("\t");

      StringVector rev_analysis_guess(analysis_guess);
      std::reverse(rev_analysis_guess.begin(), rev_analysis_guess.end());
      paradigm.insert(paradigm.end(), 
		      rev_analysis_guess.begin(), 
		      rev_analysis_guess.end());

      for (StringVectorVector::const_iterator jt = results.begin();
	   jt != results.end();
	   ++jt)
	    {
	      const StringVector &model_form = *jt;

	      paradigm.push_back("\t");
	      paradigm.insert(paradigm.end(), 
			      model_form.begin(), 
			      model_form.end());
	    }
      
      paradigm_guesses.push_back(paradigm);
    }

  return paradigm_guesses;
}

#else // MAIN_TEST

#include <cstdio>
#include <iostream>
#include <exception>

using hfst::HfstInputStream;

int main(int argc, char * argv[])
{
  // Open input fst-file for reading the guesser and generator.
  std::cerr << "Reading guesser and generator from file "
	    << argv[1] << std::endl;

  HfstInputStream in(argv[1]);

  HfstTransducer guesser(in);
  HfstTransducer generator(in);

  // Use the generator alphabet (same as the guesser alphabet) to
  // create a tokenizer.
  HfstTokenizer tokenizer = get_alphabet_string_tokenizer(generator);

  // Read the model analyses, that are used to generate word forms.
  std::cerr << "Reading model analyses from file " << argv[2] << std::endl;

  StringVectorVector model_forms = read_model_forms(argv[2],tokenizer);

  // Read input words and generate model forms. Input words are
  // tokenized and reversed for guessing. The MAX_ANALYSES first
  // analysis guesses for each word are split into a prefix consisting
  // of the lemma, pos-tag and categroy symbol. -each of the prefixes
  // is combined with each of the model analyses and the generator is
  // used to generate the corresponding word-forms, which are
  // displayed for the user.
  std::cerr << "Reading unknown word forms from STDIN." << std::endl;
  while (std::cin.peek() != EOF)
    {
      std::string line;
      std::getline(std::cin,line);
      
      StringVectorVector guesses = get_guesses(line,
					       guesser,
					       MAX_ANALYSES,
					       tokenizer);

      StringVectorVector guess_model_forms = get_paradigms(line,
							   guesses,
							   generator,
							   model_forms,
							   MAX_GENERATED_FORMS)
	;
      

      for (StringVectorVector::const_iterator it = guess_model_forms.begin();
	   it != guess_model_forms.end();
	   ++it)
	{
	  std::cout << *it << std::endl;
	}
    }
}
#endif // MAIN_TEST

