//! @file hfst-lexc-compiler.cc
//!
//! @brief Lexc compilation command line tool
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

#ifdef WINDOWS
#include <io.h>
#endif


#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "HfstTransducer.h"
#include "HfstOutputStream.h"
#include "parsers/LexcCompiler.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::ImplementationType;

using hfst::lexc::LexcCompiler;

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"

#include "inc/globals-common.h"
static char** lexcfilenames = 0;
static FILE** lexcfiles = 0;
static unsigned int lexccount = 0;
static bool is_input_stdin = true;
static ImplementationType format = hfst::UNSPECIFIED_TYPE;
static bool with_flags = false;
static bool minimize_flags = false;
static bool treat_warnings_as_errors = false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE1...]]\n"
             "Compile lexc files into transducer\n"
        "\n", program_name );
        print_common_program_options(message_out);
        fprintf(message_out, "Input/Output options:\n"
               "  -f, --format=FORMAT     compile into FORMAT transducer\n"
               "  -o, --output=OUTFILE    write result into OUTFILE\n");
        fprintf(message_out, "Lexc options:\n"
               "  -F, --withFlags         use flags to hyperminimize result\n"
               "  -M, --minimizeFlags     if --withFlags is used, minimize the number of flags\n"
               "  -W, --Werror            treat warnings as errors\n");
        fprintf(message_out, "\n");
        fprintf(message_out,
                "If INFILE or OUTFILE are omitted or -, standard streams will "
                "be used\n"
                "The possible values for FORMAT are { sfst, openfst-tropical, "
                "openfst-log,\n"
               "foma, optimized-lookup-unweighted, optimized-lookup-weighted }.\n");
        fprintf(message_out,
            "\n"
            "Examples:\n"
            "  %s -o cat.hfst cat.lexc               Compile single-file "
            "lexicon\n"
            "  %s -o L.hfst Root.lexc 2.lexc 3.lexc  Compile multi-file "
            "lexicon\n"
            "\n"
            "Using weights:\n"
            "  LEXICON Root\n"
            "  cat # \"weight: 2\" ;    Define weight for a word\n"
            "  <[dog::1]+> # ;        Use weights in regular expressions\n"
            "\n"
            "Using weights has an effect only if FORMAT is weighted, i.e.\n"
            "{ openfst-tropical, openfst-log, optimized-lookup-weighted }.\n"    
            "\n",
            program_name, program_name );
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
          {"format", required_argument, 0, 'f'},
          {"output", required_argument, 0, 'o'},
          {"withFlags", no_argument,    0, 'F'},
          {"minimizeFlags", no_argument,    0, 'M'},
          {"Werror", no_argument,    0, 'W'},
          {0,0,0,0}
        };
        int option_index = 0;
        char c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             "f:o:FMW",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
#include "inc/getopt-cases-common.h"
        case 'f':
          format = hfst_parse_format_name(optarg);
          break;
        case 'F':
          with_flags = true;
          break;
        case 'M':
          minimize_flags = true;
          break;
        case 'W':
          treat_warnings_as_errors = true;
          break;

#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
    if (format == hfst::UNSPECIFIED_TYPE)
      {
        if (!silent)
          {
            warning(0, 0, "Defaulting to OpenFst tropical type");
          }
        format = hfst::TROPICAL_OPENFST_TYPE;
      }
    if (argc - optind > 0)
      {
        lexcfilenames = static_cast<char**>(malloc(sizeof(char*)*(argc-optind)));
        lexcfiles = static_cast<FILE**>(malloc(sizeof(char*)*(argc-optind)));
        while (optind < argc)
          {
            lexcfilenames[lexccount] = hfst_strdup(argv[optind]);
            lexcfiles[lexccount] = hfst_fopen(argv[optind], "r");
            lexccount++;
            optind++;
          }
        is_input_stdin = false;
      }
    else
      {
        lexcfilenames = static_cast<char**>(malloc(sizeof(char*)));
        lexcfiles = static_cast<FILE**>(malloc(sizeof(FILE*)));
        lexcfilenames[0] = hfst_strdup("<stdin>");
        lexcfiles[0] = stdin;
        is_input_stdin = true;
        lexccount++;
      }
    return EXIT_CONTINUE;
}

int
lexc_streams(LexcCompiler& lexc, HfstOutputStream& outstream)
{
    for (unsigned int i = 0; i < lexccount; i++)
      {
        verbose_printf("Parsing lexc file %s\n", lexcfilenames[i]);
        if (lexcfiles[i] == stdin)
          {
            lexc.parse(stdin);
          }
        else
          {
            lexc.parse(lexcfilenames[i]);
          }
      }
    verbose_printf("Compiling... ");
    HfstTransducer* res = lexc.compileLexical();

    if (0 == res)
      {
        if (lexccount == 1)
          {
            error(EXIT_FAILURE, 0, "The file %s did not compile cleanly.\n"
                  "(if there are no error messages above, try -v or -d to "
                  "get more info)",
                  lexcfilenames[0]);
          }
        else
          {
            error(EXIT_FAILURE, 0, "The files %s... did not compile cleanly.\n"
                  "(if there are no error messages above, try -v or -d to "
                  "get more info)",
                  lexcfilenames[0]);
          }
        return EXIT_FAILURE;
      }
    hfst_set_name(*res, lexcfilenames[0], "lexc");
    hfst_set_formula(*res, lexcfilenames[0], "L");
    verbose_printf("\nWriting... ");
    outstream << *res;
    verbose_printf("done\n");
    delete res;
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1", "HfstLexc");

    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    for (unsigned int i = 0; i < lexccount; i++)
      {
        if (lexcfiles[i] != stdin)
          {
            fclose(lexcfiles[i]);
          }
      }
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from ");
    for (unsigned int i = 0; i < lexccount; i++)
      {
        verbose_printf("%s, ", lexcfilenames[i]);
      }
    verbose_printf("writing to %s\n", outfilename);
    // here starts the buffer handling part
    HfstOutputStream* outstream = (outfile != stdout) ?
        new HfstOutputStream(outfilename, format) :
        new HfstOutputStream(format);
    LexcCompiler lexc(format, with_flags);
    lexc.setMinimizeFlags(minimize_flags);
   // lexc.with_flags_ = with_flags;
    if (silent)
      {
        lexc.setVerbosity(false);
      }
    else
      {
        lexc.setVerbosity(verbose);
      }
    if (treat_warnings_as_errors)
      {
        lexc.setTreatWarningsAsErrors(true);
      }
    retval = lexc_streams(lexc, *outstream);
    delete outstream;
    free(lexcfilenames);
    free(outfilename);
    return retval;
}

