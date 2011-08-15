//! @file hfst-remove-epsilons.cc
//!
//! @brief Transducer epsilon removal tool
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
#include <getopt.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstTransitionGraph.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;


// add tools-specific variables here

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Remove epsilons from a transducer\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
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
          HFST_GETOPT_UNARY_LONG,
          // add tool-specific options here 
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here 
        char c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT,
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int
process_stream(HfstInputStream& instream, HfstOutputStream& outstream)
{
  //instream.open();
  //outstream.open();
    
    size_t transducer_n=0;
    while(instream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(instream);
        char* inputname = strdup(trans.get_name().c_str());
        if (strlen(inputname) <= 0)
          {
            inputname = strdup(inputfilename);
          }
        if (transducer_n==1)
        {
          verbose_printf("Removing epsilons %s...\n", inputname); 
        }
        else
        {
          verbose_printf("Removing epsilons %s...%zu\n", inputname, transducer_n); 
        }
        trans.remove_epsilons();
        if (transducer_n==1)
        {
          verbose_printf("Rebuilding and fixing %s...\n", inputname); 
        }
        else
        {
          verbose_printf("Rebuilding and fisting %s...%zu\n", inputname, transducer_n); 
        }
        HfstBasicTransducer original(trans);
        HfstBasicTransducer replication;
        HfstState state_count = 1;
        std::map<HfstState,HfstState> rebuilt;
        rebuilt[0] = 0;
    HfstState source_state=0;
        for (HfstBasicTransducer::const_iterator state = original.begin();
             state != original.end();
             ++state)
          {
            if (rebuilt.find(source_state) == rebuilt.end())
              {
                replication.add_state(state_count);
                if (original.is_final_state(source_state))
                  {
                    replication.set_final_weight(state_count,
                                                 original.get_final_weight(source_state));
                  }
                rebuilt[source_state] = state_count;
                state_count++;
              }
            for (HfstBasicTransducer::HfstTransitions::const_iterator arc =
                 state->begin();
                 arc != state->end();
                 ++arc)
              {
                if (rebuilt.find(arc->get_target_state()) == rebuilt.end())
                  {
                    replication.add_state(state_count);
                    if (original.is_final_state(arc->get_target_state()))
                      {
                        replication.set_final_weight(state_count,
                                                     original.get_final_weight(arc->get_target_state()));
                      }
                    rebuilt[arc->get_target_state()] = state_count;
                    state_count++;
                  }
                HfstBasicTransition nu(rebuilt[arc->get_target_state()],
                                       arc->get_input_symbol(),
                                       arc->get_output_symbol(),
                                       arc->get_weight());
                replication.add_transition(rebuilt[source_state], nu);
              }
        source_state++;
          }
        trans = HfstTransducer(replication, trans.get_type());
        char* composed_name = static_cast<char*>(malloc(sizeof(char) * 
                                             (strlen(inputname) +
                                              strlen("hfst-fu=(%s)")) 
                                             + 1));
        if (sprintf(composed_name, "hfst-fu=(%s)", 
                    inputname) > 0)
          {
            trans.set_name(composed_name);
          }

        outstream << trans.remove_epsilons();
    }
    instream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
    hfst_set_program_name(argv[0], "0.1", 
                          "HfstPreprocessForOptimizedLookupFormat");
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
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s, writing to %s\n", 
        inputfilename, outfilename);
    // here starts the buffer handling part
    HfstInputStream* instream = NULL;
    try {
      instream = (inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream();
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    HfstOutputStream* outstream = (outfile != stdout) ?
        new HfstOutputStream(outfilename, instream->get_type()) :
        new HfstOutputStream(instream->get_type());
    
    retval = process_stream(*instream, *outstream);
    delete instream;
    delete outstream;
    free(inputfilename);
    free(outfilename);
    return retval;
}

