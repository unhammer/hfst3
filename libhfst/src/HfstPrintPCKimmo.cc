//! @file hfst-fst2txt.cc
//!
//! @brief Transducer array printing command line tool
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

#include "HfstTransducer.h"
using hfst::HfstTransducer;
using hfst::HfstBasicTransducer;
using hfst::implementations::HfstState;

namespace hfst {

void
print_pckimmo(FILE* out, HfstTransducer& t)
  {
    HfstBasicTransducer* mutt = new HfstBasicTransducer(t);
    HfstState s = 0;
    HfstState last = 0;
    std::set<std::pair<std::string,std::string> > pairs;
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        for (HfstBasicTransducer::HfstTransitions::const_iterator arc = 
             state->begin();
             arc != state->end();
             ++arc)
          {
            std::string first = arc->get_input_symbol();
            std::string second = arc->get_output_symbol();
            pairs.insert(pair<std::string,std::string>(first, second));
          }
        ++last;
      }
    // width of the first column
    unsigned int numwidth = 0;
    for (unsigned int i = 1; i < last; i *= 10, numwidth++)
      {}
    // first line is input symbols per pair 
    // (left corner is digit width + 2)
    fprintf(out, "%*s  ", numwidth, " ");
    for (std::set<pair<std::string, std::string> >::const_iterator p = pairs.begin();
         p != pairs.end();
         ++p)
      {
        if (p->first == hfst::internal_epsilon)
          {
            fprintf(out, "%.*s ", numwidth, "0");
          }
        else if (p->first == hfst::internal_unknown)
          {
            fprintf(out, "%.*s ", numwidth, "@");
          }
        else
          {
            fprintf(out, "%.*s ", numwidth, p->first.c_str());
          }
      }
    // second line is output symbols per pair
    fprintf(out, "\n");
    // (left corner is digit width + 2)
    fprintf(out, "%*s  ", numwidth, " ");
    for (std::set<pair<std::string, std::string> >::const_iterator p = pairs.begin();
         p != pairs.end();
         ++p)
      {
        if (p->second == hfst::internal_epsilon)
          {
            fprintf(out, "%.*s ", numwidth, "0");
          }
        else if (p->second == hfst::internal_unknown)
          {
            fprintf(out, "%.*s ", numwidth, "@");
          }
        else
          {
            fprintf(out, "%.*s ", numwidth, p->second.c_str());
          }

      }
    // the transition table per state
    fprintf(out, "\n");
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        if (mutt->is_final_state(s))
          {
            fprintf(out, "%.*d. ", numwidth, s + 1);
          }
        else
          {
            fprintf(out, "%.*d: ", numwidth, s + 1);
          }
        // map everything to sink state 0 first
        std::map<std::pair<std::string,std::string>,HfstState> transitions;
        for(std::set<pair<std::string,std::string> >::const_iterator p = pairs.begin();
            p != pairs.end();
            ++p)
          {
            transitions[*p] = -1;
          }
        for (HfstBasicTransducer::HfstTransitions::const_iterator arc = 
             state->begin();
             arc != state->end();
             ++arc)
          {
            std::string first = arc->get_input_symbol();
            std::string second = arc->get_output_symbol();
            transitions[pair<std::string,std::string>(first,second)] = 
                arc->get_target_state();
          }
        for(std::map<std::pair<std::string,std::string>,HfstState>::const_iterator trans = 
            transitions.begin();
            trans != transitions.end();
            ++trans)
          {
            fprintf(out, "%.*d ", numwidth, trans->second + 1);
          }
        fprintf(out, "\n");
        ++s;
      } // for each state
  }

}
