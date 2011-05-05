//! @file hfst-strings2fst.cc
//!
//! @brief string compiling command line tool
//!
//! @author HFST Team


//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <getopt.h>
#include <limits>
#include <math.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

#include "HfstStrings2FstTokenizer.h"
#include "HfstSymbolDefs.h"

static char*  pair_test_file_name;
static FILE*  pair_test_file;
static size_t linen = 0;
static bool   pair_test_given = false;
static bool   positive_test = true;

using hfst::HfstInputStream;
using hfst::HfstTransducer;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransition;
using hfst::TROPICAL_OPENFST_TYPE;
using hfst::ImplementationType;

typedef std::vector<HfstBasicTransducer> BasicTransducerVector;
typedef std::vector<std::string> StringVector;
typedef std::set<std::string> SymbolSet;

ImplementationType rule_transducer_type = TROPICAL_OPENFST_TYPE;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "pair test for a twolc rule file.\n"
        "\n", program_name);

    print_common_program_options(message_out);
    fprintf(message_out, 
        "Input/Output options:\n"
        "  -i, --input=INFILE     Read input rule file from INFILE\n"
        "  -o, --output=OUTFILE   Write test output to OUTFILE\n"
	"  -N  --negative-test    Test fails if any of the pair strings is\n"
	"                         accepted.\n");

    fprintf(message_out, "Pair test options:\n"
            "  -I, --input-strings=SFILE        Read pair test strings from\n"
	    "                                   SFILE\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "If SFILE is missing, the test pair strings are read from STDIN.\n"
	    "If OUTFILE is missing, test output is written to STDOUT.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "The rule file is tested using correspondences given as\n"
	    "pair strings, e.g. \"e a r l y:i e r\". Every pair string is\n"
	    "tested using every rule and the program prints information\n"
	    "about correspondences that are incorrectly allowed or\n"
	    "disallowed.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "The test pair string files contain one pair string/line. Lines\n"
	    "where the first non-white-space character is \"!\" are\n"
	    "considered comment lines and skipped.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "There are two test modes positive and negative. In positive\n"
	    "mode, all of the pair strings should be allowed and in negative\n"
	    "mode they should be disallowed.\n"
	    );
    fprintf(message_out, "\n");
    fprintf(message_out,
	   "Ordinarily, positive test mode is in use. Option -N switches to\n" 
	    "negative test mode. The exit code for a successful test is 0. \n"
	    "The exit code is 1 otherwise. A successful test will print\n"
	    "\"Test passed\". A failing test prints \"Test failed\" and\n"
	    "information about pair strings that are handled incorrectly.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "In positive test mode (i.e. without option -N), if a pair\n"
	    "string is not accepted, the names of the rules that reject\n"
	    "it are printed as well as the positions in the string where the\n"
	    "rules run out of possible transitions. In negative mode, only\n"
	    "the strings that are allowed are printed.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
	    "In silent mode (-s), the program won't print anything. Only the\n"
	    "exit code tells whether the test was successful or not.\n");
    fprintf(message_out, "\n");
    print_report_bugs();
    fprintf(message_out, "\n");
    print_more_info();
}

int
parse_options(int argc, char** argv)
{
    // use of this function requires options are settable on global scope
    while (true)
    {
        static const struct option long_options[] =
        {
        HFST_GETOPT_COMMON_LONG,
        HFST_GETOPT_UNARY_LONG,
          // add tool-specific options here
            {"input-strings", required_argument, 0, 'I'},
            {"negative-test", no_argument, 0, 'N'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here 
        char c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "I:N",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
          // add tool-specific cases here
        case 'I':
            pair_test_file_name = hfst_strdup(optarg);
            pair_test_file = hfst_fopen(pair_test_file_name, "r");
            pair_test_given = true;
            break;
        case 'N':
	    positive_test = false;
	    break;
#include "inc/getopt-cases-error.h"
        }
    }

    if (!pair_test_given)
      {
        pair_test_file = stdin;
        pair_test_file_name = strdup("<stdin>");
      }
#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"

    if (inputfilename == std::string("<stdin>"))
      { 
        error(EXIT_FAILURE, 0, 
	      "The rule transducer file needs to be given using option -i.",
              inputfilename);
      }
    return EXIT_CONTINUE;
}

HfstState get_target(const std::string &isymbol,
		     const std::string &osymbol,
		     HfstState s,const HfstBasicTransducer &t,
		     const SymbolSet &known_symbols)
{
  HfstState identity_target = -1;

  for (HfstBasicTransducer::HfstTransitions::const_iterator it = t[s].begin();
       it != t[s].end();
       ++it)
    {
      if (it->get_input_symbol() == isymbol and 
	  it->get_output_symbol() == osymbol)
	{ return it->get_target_state(); }
      if (it->get_input_symbol() == "@_IDENTITY_SYMBOL_@" and
	  it->get_output_symbol() == "@_IDENTITY_SYMBOL_@")
	{ identity_target = it->get_target_state(); }
    }

  if 
    (isymbol == osymbol and known_symbols.find(isymbol) == known_symbols.end())
    { return identity_target; }
  else
    { return -1; }
}

bool is_final_state(HfstState s,const HfstBasicTransducer &t)
{ return t.is_final_state(s); }

int test(const StringPairVector &tokenized_pair_string,
	 const HfstBasicTransducer &t,
	 bool positive,
	 FILE * outfile,
	 const SymbolSet &known_symbols)
{
  HfstState s = 0;
  for (StringPairVector::const_iterator it = tokenized_pair_string.begin();
       it != tokenized_pair_string.end();
       ++it)
    {
      s = get_target(it->first,it->second,s,t,known_symbols);
      if (s == -1)
	{ 
	  if (positive)
	    { return 1; }
	  else
	    { return 0; }
	}
    }

  if (is_final_state(s,t) and positive)
    { return 0; }
  else if (positive)
    { return 1; }
  else if (not is_final_state(s,t))
    { return 0; }
  else 
    { return 1; }
}

HfstTransducer get_transducer(const StringPairVector &tokenized_pair_string)
{
  HfstBasicTransducer t;
  HfstState s = 0;
  for (StringPairVector::const_iterator it = tokenized_pair_string.begin();
       it != tokenized_pair_string.end();
       ++it)
    {
      HfstState target = t.add_state();
      t.add_transition
	(s,HfstBasicTransition(target,it->first,it->second,0.0));
      s = target;
    }
  t.set_final_weight(s,0.0);
  return HfstTransducer(t,TROPICAL_OPENFST_TYPE);
}

std::string unescape(std::string symbol)
{
  if (hfst::is_epsilon(symbol))
    { return "0"; }
  if (symbol == "@#@")
    { return "#"; }
  return symbol;
}

void print_recognized_prefix(const StringPairVector &tokenized_pair_string,
			     const HfstBasicTransducer &str_transducer,
			     const std::string &name,
			     FILE * outfile,
			     const SymbolSet &known_symbols)
{
  if (silent)
    { return; }

  fprintf(outfile,"Rule %s fails:\n",name.c_str());

  HfstState s = 0;
  StringPairVector::const_iterator it = tokenized_pair_string.begin();
  for ( ; it != tokenized_pair_string.end(); ++it)
    {
      s = get_target(it->first,it->second.c_str(),s,str_transducer,
		     known_symbols);

      if (s == -1)
	{ break; }

      if (it->first == it->second)
	{ fprintf(outfile,"%s ",unescape(it->first).c_str()); }
      else
	{ fprintf(outfile,"%s:%s ",unescape(it->first).c_str(),
		  unescape(it->second).c_str()); }

    }
  
  fprintf(outfile,"HERE ---> ");

  for ( ; it != tokenized_pair_string.end(); ++it)
    {
      if (it->first == it->second)
	{ fprintf(outfile,"%s ",unescape(it->first).c_str()); }
      else
	{ fprintf(outfile,"%s:%s ",unescape(it->first).c_str(),
		  unescape(it->second).c_str()); }
    }
  fprintf(outfile,"\n\n");
}

void print_failure_info(const StringPairVector &tokenized_pair_string,
			const HfstBasicTransducer &t,const std::string &name,
			FILE * outfile,
			const SymbolSet &known_symbols)
{
  HfstTransducer str_transducer = get_transducer(tokenized_pair_string);
  HfstTransducer tt(t,TROPICAL_OPENFST_TYPE);
  str_transducer.input_project().compose(tt).minimize();
  print_recognized_prefix(tokenized_pair_string,str_transducer,name,outfile,
			  known_symbols);
}

int test(const StringPairVector &tokenized_pair_string,
	 const std::string pair_string,
	 const BasicTransducerVector &grammar,
	 const StringVector &names,
	 bool positive,
	 FILE * outfile,
	 const SymbolSet &known_symbols)
{
  int positive_exit_code = 0;
  int negative_exit_code = 1;

  size_t ind = 0;

  for (BasicTransducerVector::const_iterator it = grammar.begin();
       it != grammar.end();
       ++it)
    {
      int new_exit_code = test(tokenized_pair_string,*it,positive,outfile,
			       known_symbols);

      if (positive and new_exit_code == 1)
	{ print_failure_info
	    (tokenized_pair_string,*it,names.at(ind),outfile,known_symbols); }
      
      if (positive and positive_exit_code == 0)
	{ positive_exit_code = new_exit_code; }
      
      if (not positive and negative_exit_code == 1)
	{ negative_exit_code = new_exit_code; }

      ++ind;
    }

  if (positive)
    {
      if (positive_exit_code == 1 and not silent)
	{ fprintf(outfile,"%s DISALLOWED\n\n",pair_string.c_str()); }
      if (positive_exit_code == 0 and verbose)
	{ fprintf(outfile,"%s ALLOWED\n\n",pair_string.c_str()); }
      return positive_exit_code;
    }
  else
    {
      if (negative_exit_code == 1 and not silent)
	{ fprintf(outfile,"%s ALLOWED\n\n",pair_string.c_str()); }
      if (negative_exit_code == 0 and verbose)
	{ fprintf(outfile,"%s DISALLOWED\n\n",pair_string.c_str()); }
      return negative_exit_code;
    }
}

std::string demangle(std::string name)
{
  size_t pos = 0;
  std::string space_subst = "__HFST_TWOLC_SPACE";
  std::string name_subst = "__HFST_TWOLC_RULE_NAME=";

  while ((pos = name.find(name_subst)) != std::string::npos)
    { name.replace(pos,name_subst.size(),""); }

  while ((pos = name.find(space_subst)) != std::string::npos)
    { name.replace(pos,space_subst.size()," "); }

  return name;
}

bool is_empty_or_comment(const char * line)
{
  while (*line == ' ' or *line == '\t')
    { ++line; }
  if (*line == 0 or *line == '!')
    { return true; }
  return false;
}

void get_symbols(HfstBasicTransducer &t,SymbolSet &known_symbols)
{
  for (HfstBasicTransducer::const_iterator it = t.begin(); it != t.end(); ++it)
    {
      for (HfstBasicTransducer::HfstTransitions::const_iterator jt = 
	     it->second.begin();
	   jt != it->second.end();
	   ++jt)
	{ 
	  known_symbols.insert(jt->get_input_symbol());
	  known_symbols.insert(jt->get_output_symbol());
	}
    }
}

int
process_stream(HfstInputStream& inputstream, FILE* outstream)
{
    BasicTransducerVector grammar;
    StringVector rule_names;

    // Read transducers in rule file.
    size_t transducer_n=0;
    while (inputstream.is_good())
      {
        transducer_n++;
        if (transducer_n==1)
          {
            verbose_printf("Reading %s...\n", inputfilename); 
          }
        else
          {
            verbose_printf("Reading %s...%zu\n", inputfilename,
                           transducer_n); 
          }
        HfstTransducer trans(inputstream);
	rule_transducer_type = trans.get_type();
        grammar.push_back(trans);
	rule_names.push_back(demangle(trans.get_name()));
      }

    inputstream.close();

    SymbolSet known_symbols;
    if (not grammar.empty())
      {
	verbose_printf("Defining known symbols.\n");
	get_symbols(grammar[0],known_symbols);
	for (SymbolSet::const_iterator it = known_symbols.begin();
	     it != known_symbols.end();
	     ++it)
	  { verbose_printf("Symbol %s\n",it->c_str()); }
      }

    char* line = 0;
    size_t llen = 0;

    // Define tokenizer with no multi character symbols and an
    // empty epsilon representation.
    StringVector empty_v;
    HfstStrings2FstTokenizer input_tokenizer
      (empty_v, std::string("0"));

    int exit_code = 0;
    while (hfst_getline(&line, &llen, pair_test_file) != -1)
      {
        linen++;
        char *p = line;
        while (*p != '\0')
          {
            if (*p == '\n')
              {
                *p = '\0';
                break;
              }
            p++;
          }
	if (is_empty_or_comment(line))
	  { continue; }
        verbose_printf("Pair test on %s...\n", line);

	int new_exit_code = 0;

	try
	  {
	    StringPairVector tokenized_pair_string =
	      input_tokenizer.tokenize_pair_string(line,true);
	    
	    tokenized_pair_string.insert
	      (tokenized_pair_string.begin(),
	       StringPair("@#@",hfst::internal_epsilon));
	    tokenized_pair_string.insert
	      (tokenized_pair_string.end(),
	       StringPair("@#@",hfst::internal_epsilon));

	    int new_exit_code = 
	      test(tokenized_pair_string,line,grammar,rule_names,
		   positive_test,outfile,known_symbols);
	    
	  }
	catch (const UnescapedColsFound &e)
	  {
	    error(EXIT_FAILURE, 0, 
		  "The correspondence %s contains unquoted colon-symbols. If "
		  "you want to input pairs where either symbol is epsilon, "
		  "use 0 e.g. \"m a s s 0:e s\".\n",
		  line);
	    
	  }

	
	if (exit_code == 0)
	  { exit_code = new_exit_code; }

      } // while lines in input
    free(line);	

    return exit_code;
}

int main( int argc, char **argv ) {
    hfst_set_program_name(argv[0], "0.6", "HfstPairTest");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (inputfile != stdin)
    {
        fclose(inputfile);
    }
    verbose_printf("Reading from %s, writing to %s\n", 
        inputfilename, outfilename);

    // here starts the buffer handling part
    HfstInputStream* instream = NULL;
    try 
      {
        instream = (inputfile != stdin) ?
          new HfstInputStream(inputfilename) :
          new HfstInputStream();
      } 
    catch(const HfstException e)
      {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
      }
    int exit_code = process_stream(*instream, outfile);

    if (not silent)
      {
	if (exit_code == 0)
	  { fprintf(outfile,"Test passed.\n"); }
	else
	  { fprintf(outfile,"Test failed.\n"); }
      }

    if (outfile != stdout)
    {
        fclose(outfile);
    }
    free(inputfilename);
    free(outfilename);

    return exit_code;
}

