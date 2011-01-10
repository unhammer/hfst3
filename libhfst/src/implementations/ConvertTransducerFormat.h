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

#ifndef _CONVERT_TRANSDUCER_H_
#define _CONVERT_TRANSDUCER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif // HAVE_CONFIG_H

#include <map>
#include <iostream>
#include <vector>
#include <map>

#if HAVE_OPENFST
#include <fst/fstlib.h>
#endif // HAVE_OPENFST

#if HAVE_SFST
#include "sfst/fst.h"
#endif // HAVE_SFST

#if HAVE_FOMA
#ifndef _FOMALIB_H_
#define _FOMALIB_H_
#include <stdbool.h>
#include "fomalib.h"
#endif // _FOMALIB_H_
#endif // HAVE_FOMA

#if HAVE_MFSTL
#ifndef _MFSTLLIB_H_
#define _MFSTLLIB_H_
#include "mfstl/MyFst.h"
#endif // _MFSTLLIB_H_
#endif // HAVE_MFSTL

#include "HfstExceptions.h"
#include "optimized-lookup/transducer.h"

struct fsm;

/** @file ConvertTransducerFormat.h
    \brief Declarations of functions for converting between transducer backend formats. */

namespace hfst { 

  class HfstTransducer;

namespace implementations {

    template <class T, class W> class HfstTransitionGraph;
    class HfstNameThis;
    typedef HfstTransitionGraph<HfstNameThis, float> HfstBasicTransducer; 
    typedef unsigned int HfstState;

  using namespace hfst::exceptions;

#if HAVE_OPENFST
  typedef fst::StdArc::StateId StateId;
  typedef fst::ArcIterator<fst::StdVectorFst> StdArcIterator;

  typedef fst::ArcTpl<fst::LogWeight> LogArc;
  typedef fst::VectorFst<LogArc> LogFst;
#endif


  class ConversionFunctions {

  public:

    static HfstBasicTransducer * hfst_transducer_to_hfst_basic_transducer(const hfst::HfstTransducer &t);

#if HAVE_SFST
  static void sfst_to_hfst_basic_transducer( SFST::Node *node, SFST::NodeNumbering &index, 
				std::set<SFST::Node*> &visited_nodes, 
				HfstBasicTransducer *net, SFST::Alphabet &alphabet );

  static HfstBasicTransducer * sfst_to_hfst_basic_transducer(SFST::Transducer * t);

  static SFST::Transducer * hfst_basic_transducer_to_sfst(const HfstBasicTransducer * t);
#endif // HAVE_SFST
  
#if HAVE_FOMA
  static HfstBasicTransducer * foma_to_hfst_basic_transducer(struct fsm * t);

  static struct fsm * hfst_basic_transducer_to_foma(const HfstBasicTransducer * t);
#endif // HAVE_FOMA

#if HAVE_OPENFST
  static HfstBasicTransducer * tropical_ofst_to_hfst_basic_transducer
    (fst::StdVectorFst * t);
  
  static StateId hfst_state_to_state_id
    (HfstState s, std::map<HfstState, StateId> &state_map, 
     fst::StdVectorFst * t);

  static fst::StdVectorFst * hfst_basic_transducer_to_tropical_ofst
    (const HfstBasicTransducer * t);

  static HfstBasicTransducer * log_ofst_to_hfst_basic_transducer
    (LogFst * t);
  
  static LogFst * hfst_basic_transducer_to_log_ofst
    (const HfstBasicTransducer * t);
#endif // HAVE_OPENFST 
  

  static HfstBasicTransducer * hfst_ol_to_hfst_basic_transducer(hfst_ol::Transducer * t);

  static hfst_ol::Transducer * hfst_basic_transducer_to_hfst_ol
    (HfstBasicTransducer * t, bool weighted);


#if HAVE_MFSTL
  static HfstBasicTransducer * mfstl_to_hfst_basic_transducer(mfstl::MyFst * t);

  static mfstl::MyFst * hfst_basic_transducer_to_mfstl(const HfstBasicTransducer * t);
#endif // HAVE_MFSTL

  };


  } }
#endif // _CONVERT_TRANSDUCER_H_

