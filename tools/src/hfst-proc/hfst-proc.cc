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

#include <getopt.h>
#include <fstream>
#include <cstdlib>
#include "hfst-proc.h"
#include "transducer.h"
#include "formatter.h"
#include "applicators.h"

bool verboseFlag = false;

bool displayWeightsFlag = false;
bool displayUniqueFlag = false;
int maxAnalyses = std::numeric_limits<int>::max();
bool preserveDiacriticRepresentationsFlag = false;
bool printDebuggingInformationFlag = false;
bool rawMode = false;

static bool handle_hfst3_header(std::istream& is)
{
  const char* header1 = "HFST";
  int header_loc = 0; // how much of the header has been found
  int c;
  for(header_loc = 0; header_loc < strlen(header1) + 1; header_loc++)
  {
    c = is.get();
    if(c != header1[header_loc])
      break;
  }
  if(header_loc == strlen(header1) + 1) // we found it
  {
      unsigned short remaining_header_len;
      is.read(reinterpret_cast<char*>(&remaining_header_len),
	      sizeof(remaining_header_len));
      if(is.get() != '\0') {
	  HFST_THROW(HfstException);
	  return false;
      }
      char * headervalue = new char[remaining_header_len];
      while(remaining_header_len > 0) {
	  is.getline(headervalue, remaining_header_len + 1, '\0');
	  remaining_header_len -= strlen(headervalue) + 1;
	  if (!strcmp(headervalue, "type")) {
	      is.getline(headervalue, remaining_header_len + 1, '\0');
	      remaining_header_len -= strlen(headervalue) + 1;
	      if (strcmp(headervalue, "HFST_OL") and
		  strcmp(headervalue, "HFST_OLW")) {
		  delete headervalue;
		  HFST_THROW(TransducerHasWrongTypeException);
		  return false;
	      }
      }
      }
      delete headervalue;
      if (remaining_header_len == 0) {
	  return true;
      } else {
	  HFST_THROW(HfstException);
	  return false;
      }
  } else // nope. put back what we've taken
  {
    is.putback(c); // first the non-matching character
    for(int i=header_loc-1; i>=0; i--) // then the characters that did match (if any)
      is.putback(header1[i]);
    
    return false;
  }
}

void stream_error(const char* e)
{
  throw std::ios_base::failure((std::string("Error: malformed input stream: ")+e+"\n").c_str());
}
void stream_error(std::string e) {stream_error(e.c_str());}


bool print_usage(void)
{
  std::cout <<
    "\n" <<
    "Usage: hfst-proc " <<
    "[-a [-p|-C|-x] [-k]|-g|-n|-d|-t] [-W] [-n N] [-c|-w] [-z] [-v|-q|]\n" <<
    "    transducer_file [input_file [output_file]]\n" <<
    "Perform a transducer lookup on a text stream, tokenizing on the fly\n" <<
    "\n" <<
    "  -a, --analysis          Morphological analysis (default)\n" <<
    "  -g, --generation        Morphological generation\n" <<
    "  -n, --non-marked-gen    Morph. generation without unknown word marks\n" <<
    "  -d, --debugged-gen      Morph. generation with everything printed\n" <<
    "  -t  --tokenize          Tokenize the input stream into symbols (for debugging)\n" <<
    "  -p  --apertium          Apertium output format for analysis (default)\n" <<
    "  -C  --cg                Constraint Grammar output format for analysis\n" <<
    "  -x, --xerox             Xerox output format for analysis\n" <<
    "  -k, --keep-compounds    Retain compound analyses even when analyses with fewer\n" <<
    "                          compound-boundaries are available\n" <<
    "  -W, --show-weights      Print final analysis weights (if any)\n" <<
    "  -N N, --analyses=N      Output no more than N analyses\n" <<
    "                          (if the transducer is weighted, the N best analyses)\n" <<
    "  -c, --case-sensitive    Perform lookup using the literal case of the input\n" <<
    "                          characters\n" <<
    "  -w  --dictionary-case   Output results using dictionary case instead of\n" <<
    "                          surface case\n" <<
    "  -z  --null-flush        Flush output on the null character\n" <<
    "  -v, --verbose           Be verbose\n" <<
    "  -q, --quiet             Don't be verbose (default)\n" <<
    "  -V, --version           Print version information\n" <<
    "  -h, --help              Print this help message\n" <<
    "  -X, --raw               Do not perform any mangling to:\n"
    "                          case, ``superblanks'' or anything else!!!\n"
    "\n" <<
    "Report bugs to " << PACKAGE_BUGREPORT << "\n" <<
    "\n";
  return true;
}

bool print_version(void)
{
  std::cout <<
    "\n" <<
    "hfst-proc 0.0 (" << 
    PACKAGE_STRING << ")" << std::endl <<
    __DATE__ << " " __TIME__ << std::endl <<
    "copyright (C) 2009 University of Helsinki\n";
  return true;
}

bool print_short_help(void)
{
  print_usage();
  return true;
}

int main(int argc, char **argv)
{
  int cmd = 0;
  int output_type = 0;
  int capitalization = 0;
  bool filter_compound_analyses = true;
  bool null_flush = false;
  
  while (true)
  {
    static struct option long_options[] =
    {
      // first the hfst-mandated options
      {"help",           no_argument,       0, 'h'},
      {"version",        no_argument,       0, 'V'},
      {"verbose",        no_argument,       0, 'v'},
      {"quiet",          no_argument,       0, 'q'},
      {"silent",         no_argument,       0, 's'},
      // the hfst-proc-specific options
      {"analysis",       no_argument,       0, 'a'},
      {"generation",     no_argument,       0, 'g'},
      {"non-marked-gen", no_argument,       0, 'n'},
      {"debugged-gen",   no_argument,       0, 'd'},
      {"tokenize",       no_argument,       0, 't'},
      {"apertium",       no_argument,       0, 'p'},
      {"xerox",          no_argument,       0, 'x'},
      {"cg",             no_argument,       0, 'C'},
      {"keep-compounds", no_argument,       0, 'k'},
      {"show-weights",   no_argument,       0, 'W'},
      {"analyses",       required_argument, 0, 'N'},
      {"case-sensitive", no_argument,       0, 'c'},
      {"dictionary-case",no_argument,       0, 'w'},
      {"null-flush",     no_argument,       0, 'z'},
      {"raw",            no_argument,       0, 'X'},
      {0,                0,                 0,  0 }
    };
    
    int option_index = 0;
    int c = getopt_long(argc, argv, "hVvqsagndtpxCkWN:cwzX", long_options, &option_index);

    if (c == -1) // no more options to look at
      break;

    switch (c)
    {
    case 'h':
      print_usage();
      return EXIT_SUCCESS;
      break;
      
    case 'V':
      print_version();
      return EXIT_SUCCESS;
      break;
      
    case 'v':
  #ifdef DEBUG
      printDebuggingInformationFlag = true;
      preserveDiacriticRepresentationsFlag = true;
  #endif
      
      verboseFlag = true;
      break;
      
    case 'q':
    case 's':
  #ifdef DEBUG
      printDebuggingInformationFlag = false;
      preserveDiacriticRepresentationsFlag = false;
  #endif
      verboseFlag = false;
      displayWeightsFlag = true;
      break;
    
    case 'a':
    case 'g':
    case 'n':
    case 'd':
    case 't':
      if(cmd==0)
        cmd = c;
      else
      {
        std::cerr << "Multiple operation modes given" << std::endl;
        print_short_help();
        return EXIT_FAILURE;
      }
      break;
      
    case 'p':
    case 'C':
    case 'x':
      if(output_type == 0)
        output_type = c;
      else
      {
        std::cerr << "Multiple output modeds given" << std::endl;
        print_short_help();
        return EXIT_FAILURE;
      }
      break;
      
    case 'k':
      filter_compound_analyses = false;
      break;
      
    case 'W':
      displayWeightsFlag = true;
      break;
      
    case 'N':
      maxAnalyses = atoi(optarg);
      if (maxAnalyses < 1)
        {
          std::cerr << "Invalid or no argument for analyses count\n";
          return EXIT_FAILURE;
        }
      break;
    
    case 'c':
    case 'w':
    case 'X':
      if(capitalization==0)
        capitalization=c;
      else
      {
        std::cerr << "Multiple capitalization modes given" << std::endl;
        print_short_help();
        return EXIT_FAILURE;
      }
      break;
    
    case 'z':
      null_flush = true;
      break;
      
    default:
      std::cerr << "Invalid option\n\n";
      print_short_help();
      return EXIT_FAILURE;
      break;
    }
  }
  
  std::istream* input = &std::cin;
  std::ostream* output = &std::cout;
  int fst_arg = optind, in_arg = optind+1, out_arg = optind+2;
  
  if(optind == (argc-2))
    out_arg = -1;
  else if(optind == (argc-1))
    in_arg = out_arg = -1;
  else if(optind != (argc-3))
  {
    print_short_help();
    return EXIT_FAILURE;
  }
  
  std::ifstream in(argv[(fst_arg)], std::ios::in | std::ios::binary);
  if(!in)
  {
    std::cerr << "Could not open transducer file " << argv[(optind)] << std::endl;
    return EXIT_FAILURE;
  }
  
  if(in_arg != -1)
  {
    input = new std::ifstream(argv[in_arg], std::ios::in | std::ios::binary);
    if(!(*input))
    {
      std::cerr << "Could not open input file " << argv[in_arg] << std::endl;
      return EXIT_FAILURE;
    }
  }
  
  if(out_arg != -1)
  {
    output = new std::ofstream(argv[out_arg], std::ios::out | std::ios::binary);
    if(!(*output))
    {
      std::cerr << "Could not open output file " << argv[out_arg] << std::endl;
      return EXIT_FAILURE;
    }
  }
  
  CapitalizationMode capitalization_mode;
  switch(capitalization)
  {
    case 'c':
      capitalization_mode = CaseSensitive;
      break;
    case 'w':
      capitalization_mode = DictionaryCase;
      break;
    case 'X':
      capitalization_mode = CaseSensitiveDictionaryCase;
      rawMode = true;
      break;
    default:
      if(capitalization == 0 && output_type == 'C')
        capitalization_mode = DictionaryCase;
      else
        capitalization_mode = IgnoreCase;
  }
  try
  {
    handle_hfst3_header(in);
    ProcTransducer t(in);
    if(verboseFlag)
      std::cout << "Transducer successfully loaded" << std::endl;
    in.close();
    TokenIOStream token_stream(*input, *output, t.get_alphabet(), null_flush,
                               rawMode);
    Applicator* applicator = NULL;
    OutputFormatter* output_formatter = NULL;
    switch(cmd)
    {
      case 't':
        applicator = new TokenizationApplicator(t, token_stream);
        break;
      case 'g':
        applicator = new GenerationApplicator(t, token_stream, gm_unknown, capitalization_mode);
        break;
      case 'n':
        applicator = new GenerationApplicator(t, token_stream, gm_clean, capitalization_mode);
        break;
      case 'd':
        applicator = new GenerationApplicator(t, token_stream, gm_all, capitalization_mode);
        break;
      case 'a':
      default:
        switch(output_type)
        {
          case 'C':
            output_formatter = (OutputFormatter*)new CGOutputFormatter(token_stream, filter_compound_analyses);
            break;
          case 'x':
            output_formatter = (OutputFormatter*)new XeroxOutputFormatter(token_stream, filter_compound_analyses);
            break;
          default:
            output_formatter = (OutputFormatter*)new ApertiumOutputFormatter(token_stream, filter_compound_analyses);
        }
        applicator = new AnalysisApplicator(t, token_stream, *output_formatter, capitalization_mode);
        break;
    }
    
    applicator->apply();
    
    delete applicator;
    if(output_formatter != NULL)
      delete output_formatter;
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  
  if(in_arg != -1)
  {
    dynamic_cast<std::ifstream*>(input)->close();
    delete input;
  }
  if(out_arg != -1)
  {
    dynamic_cast<std::ofstream*>(output)->close();
    delete output;
  }
  
  return EXIT_SUCCESS;
}

