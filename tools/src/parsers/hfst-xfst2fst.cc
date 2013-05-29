//! @file hfst-xfst2fst.cc
//!
//! @brief Implemetation of command line program for parsing xfst commands

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

#if HAVE_CONFIG_H
#   include <config.h>
#endif

#include "XfstCompiler.h"

#ifdef HAVE_READLINE
  #include <readline/readline.h>
  #include <readline/history.h>
#endif

#include <getopt.h>
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "inc/globals-common.h"
#include "hfst-file-to-mem.h"

static hfst::ImplementationType output_format = hfst::UNSPECIFIED_TYPE;
static char* scriptfilename = NULL;
static char* startupfilename = NULL;
static std::vector<char*> execute_commands;
static bool pipemode = false;
#ifdef HAVE_READLINE
  static bool use_readline = true;
#else
  static bool use_readline = false;
#endif

void
print_usage()
{
  // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp                                                                                                                                                               
  // Usage line                                                                                                                                                                                                                            
  fprintf(message_out, "Usage: %s [OPTIONS...]\n"
          "XFST parser\n"
          "\n", program_name);
  
  print_common_program_options(message_out);
  fprintf(message_out, "\n");
  fprintf(message_out, "Xfst-specific options:\n");
  fprintf(message_out, 
          "  -e, --execute=CMD        Execute command CMD on startup\n" 
          "  -f, --format=FMT         Write result using FMT as backend format\n"
          "  -F. --scriptfile=FILE    Read commands from FILE, and quit\n"
          "  -l, --startupfile=FILE   Read commands from FILE on startup\n"
          "  -p, --pipe-mode          Pipe mode (non-interactive), reads from standard input\n"
          "  -r, --no-readline        Do not use readline library for input\n"
          "\n"
          "Option --execute can be invoked many times.\n"
          "If FMT is not given, OpenFst's tropical format will be used.\n"
          "The possible values for FMT are { foma, openfst-tropical, openfst-log,\n"
          "sfst, optimized-lookup-weighted, optimized-lookup-unweighted }\n"
          "Readline library, if enabled when configuring, is used for input be default.\n");
  fprintf(message_out, "\n");
  print_report_bugs();
  fprintf(message_out, "\n");
  print_more_info();
}

int
parse_options(int argc, char** argv)
{
  extend_options_getenv(&argc, &argv);
  // use of this function requires options are settable on global scope                                                                                                                                                                    
  while (true)
    {
        static const struct option long_options[] =
          {
            HFST_GETOPT_COMMON_LONG,
            // add tool-specific options here
            {"format", required_argument, 0, 'f'},
            {"scriptfile", required_argument, 0, 'F'},
            {"execute", required_argument, 0, 'e'},
            {"startupfile", required_argument, 0, 'l'},
            {"pipe-mode", no_argument, 0, 'p'},
            {"no-readline", no_argument, 0, 'r'},
            {0,0,0,0}
          };
        int option_index = 0;
        // add tool-specific options here
        char c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT "f:F:e:l:pr",
                             long_options, &option_index);
        if (-1 == c)
          {
            break;
          }

        switch (c)
          {
            //copied from "inc/getopt-cases-common.h"
          case 'd':
            debug = true;
            break;
          case 'h':
            print_usage();
            return EXIT_SUCCESS;
            break;
          case 'V':
            print_version();
            return EXIT_SUCCESS;
            break;
          case 'v':
            verbose = true;
            silent = false;
            break;
          case 'q':
          case 's':
            verbose = false;
            silent = true;
            break;
          case 'f':
            output_format = hfst_parse_format_name(optarg);
            break;
          case 'F':
            scriptfilename = hfst_strdup(optarg);
            break;
          case 'e':
            execute_commands.push_back(hfst_strdup(optarg));
            break;
          case 'l':
            startupfilename = hfst_strdup(optarg);
            break;
          case 'p':
            pipemode = true;
            use_readline = false;
            break;
          case 'r':
            use_readline = false;
            break;
#include "inc/getopt-cases-error.h"
          }
    }

  if (output_format == hfst::UNSPECIFIED_TYPE)
    {
      output_format = hfst::TROPICAL_OPENFST_TYPE;
      verbose_printf("Using default output format OpenFst "
                     "with tropical weight class\n");
    }

  return EXIT_CONTINUE;
}

// Parse file 'filename' using compiler 'comp'.
// Filename "<stdin>" uses stdin for reading.
int parse_file(const char* filename, hfst::xfst::XfstCompiler &comp)
{
  char* line = hfst_file_to_mem(filename);
  if (NULL == line) 
    {
      error(EXIT_FAILURE, 0, "error when reading file %s\n", filename);
      return EXIT_FAILURE;
    }
  if (0 != comp.parse_line(line))
    {
      error(EXIT_FAILURE, 0, "error when parsing file %s\n", filename);
      return EXIT_FAILURE;
    }
  free(line);
  return 0;
}

void insert_zeroes(char * array, unsigned int number)
{
  for (unsigned int i=0; i<number; i++)
    {
      array[i] = '\0'; 
    }
}

int main(int argc, char** argv)
{

  hfst_set_program_name(argv[0], "0.1", "HfstXfst2Fst");
  int retval = parse_options(argc, argv);
  if (retval != EXIT_CONTINUE)
    {
      return retval;
    }

  switch (output_format)
    {
    case hfst::SFST_TYPE:
      verbose_printf("Using SFST as output handler\n");
      break;
    case hfst::TROPICAL_OPENFST_TYPE:
      verbose_printf("Using OpenFst's tropical weights as output\n");
      break;
    case hfst::LOG_OPENFST_TYPE:
      verbose_printf("Using OpenFst's log weight output\n");
      break;
    case hfst::FOMA_TYPE:
      verbose_printf("Using foma as output handler\n");
      break;
    case hfst::HFST_OL_TYPE:
      verbose_printf("Using optimized lookup output\n");
      break;
    case hfst::HFST_OLW_TYPE:
      verbose_printf("Using optimized lookup weighted output\n");
      break;
    default:
      error(EXIT_FAILURE, 0, "Unknown format cannot be used as output\n");
      return EXIT_FAILURE;
    }
  
  if (pipemode && (scriptfilename != NULL))
    {
      error(EXIT_FAILURE, 0 , "--pipe-mode and --scriptfile cannot be used simultaneously\n");
      return EXIT_FAILURE;
    }

  if ((startupfilename != NULL) && (scriptfilename != NULL))
    {
      error(EXIT_FAILURE, 0, "--startupfile and --scriptfile cannot be used simultaneously\n");
      return EXIT_FAILURE;
    }


  // Create XfstCompiler
  hfst::xfst::XfstCompiler comp(output_format);
#ifdef HAVE_READLINE
  comp.setReadline(use_readline);
#else
  comp.setReadline(false);
#endif
  comp.setVerbosity(!silent);

  // If needed, execute scripts given in command line
  for (std::vector<char*>::const_iterator cmd = execute_commands.begin();
       cmd != execute_commands.end(); cmd++)
    {
      verbose_printf("Executing xfst command '%s' given on command line...\n", *cmd);
      if (0 != comp.parse_line(*cmd))
        {
          error(EXIT_FAILURE, 0, "command '%s' could not be parsed\n", *cmd);
          return EXIT_FAILURE;
        }
    }
  // If needed, execute script in startup file
  if (startupfilename != NULL)
    {
      verbose_printf("Executing startup file '%s'...\n", startupfilename);
      if (parse_file(startupfilename, comp) == EXIT_FAILURE)
        {
          return EXIT_FAILURE;
        }
    }

  if (pipemode) 
    {
      verbose_printf("Reading from standard input...\n");
      comp.setReadInteractiveTextFromStdin(false);
      if (parse_file("<stdin>", comp) == EXIT_FAILURE)
        //if (0 != comp.parse(stdin)) segfaults with scriptfiles..
        {
          return EXIT_FAILURE;
        }
    }
  else if (scriptfilename != NULL)
    {
      verbose_printf("Reading from script file '%s'\n", scriptfilename);
      if (parse_file(scriptfilename, comp) == EXIT_FAILURE)
        {
          return EXIT_FAILURE;
        }
    }
  // Use interactive mode
  else if (! use_readline)
    {
      verbose_printf("Starting interactive mode...\n");
      comp.setPromptVerbosity(!silent);
      comp.setReadInteractiveTextFromStdin(true);
      if (!silent)
        comp.prompt();
      // support for backspace

      std::string expression = "";
      unsigned int lines = 0;
      const unsigned int MAX_LINE_LENGTH = 1024;

      char line [MAX_LINE_LENGTH];
      insert_zeroes(line, MAX_LINE_LENGTH);

      while (cin.getline(line, MAX_LINE_LENGTH))
        {
          std::string linestr(line);
          expression += linestr;

          if (expression.size() > 0 && expression.at(expression.size()-1) == '\\')
            {
              expression.at(expression.size()-1) = '\n';
              if (!silent)
                comp.prompt();
              continue;
            }

          if (0 != comp.parse_line((expression + "\n").c_str()))
            {
              fprintf(stderr, "expression '%s' could not be parsed\n", expression.c_str());
            }

          expression = "";
        }
    }
  else
    {
#ifdef HAVE_READLINE
      // support for backspace and Up/Down keys, needs readline library

      verbose_printf("Starting interactive mode...\n");
      comp.setPromptVerbosity(false); // prompts handled manually
      comp.setReadInteractiveTextFromStdin(true);
      char *buf = NULL;               // result from readline
      rl_bind_key('\t',rl_abort);     // disable auto-complet
      std::string expression = "";    // the whole expression (possibly several lines)

      char* promptline = (!silent) ? comp.get_prompt() : strdup("");

      while((buf = readline(promptline)) != NULL)
        {
          std::string bufstr(buf);
          expression += bufstr;

          if (expression.size() > 0 && expression.at(expression.size()-1) == '\\')
            {
              expression.at(expression.size()-1) = '\n';
              free(promptline);
              promptline = (!silent) ? comp.get_prompt() : strdup("");
              continue;
            }

          if (buf[0] != 0) {
            add_history(expression.c_str()); }

          if (0 != comp.parse_line((expression + "\n").c_str()))
            {
              fprintf(stderr, "expression '%s' could not be parsed\n", expression.c_str());
            }
          
          expression = "";
          free(promptline);
          promptline = (!silent) ? comp.get_prompt() : strdup("");
        }
      free(buf);
      free(promptline);
#else
      fprintf(stderr, "ERROR: missing readline library\n");
      return EXIT_FAILURE;
#endif
    }

  return EXIT_SUCCESS;
}

