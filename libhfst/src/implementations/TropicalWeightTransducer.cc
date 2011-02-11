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

#include "TropicalWeightTransducer.h"

#ifndef DEBUG_MAIN
namespace hfst { namespace implementations
{
  float tropical_seconds_in_harmonize=0;

    float TropicalWeightTransducer::get_profile_seconds() {
      return tropical_seconds_in_harmonize;
    }

  bool openfst_tropical_use_hopcroft=false;

  void openfst_tropical_set_hopcroft(bool value) {
    openfst_tropical_use_hopcroft=value;
  }

  void initialize_symbol_tables(StdVectorFst *t);

  TropicalWeightInputStream::TropicalWeightInputStream(void):
    i_stream(),input_stream(cin)
  {}
  TropicalWeightInputStream::TropicalWeightInputStream
    (const std::string &filename_):
    filename(std::string(filename_)),i_stream(filename.c_str()),
    input_stream(i_stream)
  {}

  char TropicalWeightInputStream::stream_get() {
    return (char) input_stream.get(); }

  void TropicalWeightInputStream::stream_unget(char c) {
    input_stream.putback(c); }

  void TropicalWeightTransducer::remove_symbol_table(StdVectorFst *t)
  {
    t->SetInputSymbols(NULL);
  }

  void TropicalWeightTransducer::insert_to_alphabet
    (StdVectorFst *t, const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    fst::SymbolTable * st = t->InputSymbols()->Copy();
    st->AddSymbol(symbol);
    t->SetInputSymbols(st);
    delete st;
  }

  StringSet TropicalWeightTransducer::get_alphabet(StdVectorFst *t)
  {
    assert(t->InputSymbols() != NULL);
    StringSet s;
    for ( fst::SymbolTableIterator it 
	    = fst::SymbolTableIterator(*(t->InputSymbols()));
	  not it.Done(); it.Next() ) {
      s.insert( std::string(it.Symbol()) );
    }
    return s;
  }

  /* Find the number-to-number mappings needed to be performed to t1 
     so that it will follow the same symbol-to-number encoding as t2.
     @pre t2's symbol table must contain all symbols in t1's symbol table. 
  */
  NumberNumberMap TropicalWeightTransducer::create_mapping
    (fst::StdVectorFst *t1, fst::StdVectorFst *t2)
  {
    NumberNumberMap km;
    // find the number-to-number mappings for transducer t1
    for ( fst::SymbolTableIterator it 
	    = fst::SymbolTableIterator(*(t1->InputSymbols()));
	  not it.Done(); it.Next() ) {    
      km [ (unsigned int)it.Value() ] 
	= (unsigned int) t2->InputSymbols()->Find( it.Symbol() );
    }
    return km;
  }

  /* Recode the symbol numbers in this transducer as indicated in KeyMap km. */
  void TropicalWeightTransducer::recode_symbol_numbers
    (StdVectorFst *t, NumberNumberMap &km) 
  {
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	for (fst::MutableArcIterator<StdVectorFst> aiter(t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    StdArc new_arc;
	    new_arc.ilabel = km[arc.ilabel];
	    new_arc.olabel = km[arc.olabel];
	    new_arc.weight = arc.weight;
	    new_arc.nextstate = arc.nextstate;
	    aiter.SetValue(new_arc);
	  }
      }
    return;
  }

  StdVectorFst * TropicalWeightTransducer::set_final_weights
    (StdVectorFst * t, float weight)
  {
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if ( t->Final(s) != TropicalWeight::Zero() )
	  t->SetFinal(s, weight);
      }
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::push_weights
    (StdVectorFst * t, bool to_initial_state)
  {
    assert (t->InputSymbols() != NULL);
    fst::StdVectorFst * retval = new fst::StdVectorFst();
    if (to_initial_state)
      fst::Push<StdArc, REWEIGHT_TO_INITIAL>(*t, retval, fst::kPushWeights);
    else
      fst::Push<StdArc, REWEIGHT_TO_FINAL>(*t, retval, fst::kPushWeights);
    retval->SetInputSymbols(t->InputSymbols());
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::transform_weights
    (StdVectorFst * t,float (*func)(float f))
  {
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if ( t->Final(s) != TropicalWeight::Zero() )
	  t->SetFinal( s, func(t->Final(s).Value()) );
	for (fst::MutableArcIterator<StdVectorFst> aiter(t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    StdArc new_arc;
	    new_arc.ilabel = arc.ilabel;
	    new_arc.olabel = arc.olabel;
	    new_arc.weight = func(arc.weight.Value());
	    new_arc.nextstate = arc.nextstate;
	    aiter.SetValue(new_arc);
	  }
      }
    return t;
  }

  void TropicalWeightTransducer::write_in_att_format
    (StdVectorFst *t, FILE *ofile)
  {

    const SymbolTable *sym = t->InputSymbols();
    assert(sym != NULL);

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	if (s == initial_state) {
	int origin;  // how origin state is printed, see the first comment
	if (s == 0)
	  origin = zero_print;
	else if (s == initial_state)
	  origin = 0;
	else
	  origin = (int)s;
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;
	    fprintf(ofile, "%i\t%i\t%s\t%s\t%f\n", origin, target,
		    sym->Find(arc.ilabel).c_str(), 
		    sym->Find(arc.olabel).c_str(),
		    arc.weight.Value());
	  }
	if (t->Final(s) != TropicalWeight::Zero())
	  fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
	break;
	}
      }

    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if (s != initial_state) {
	  int origin;  // how origin state is printed, see the first comment
	  if (s == 0)
	    origin = zero_print;
	  else if (s == initial_state)
	    origin = 0;
	  else
	    origin = (int)s;
	  for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const StdArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;
	      fprintf(ofile, "%i\t%i\t%s\t%s\t%f\n", origin, target,
		      sym->Find(arc.ilabel).c_str(), 
		      sym->Find(arc.olabel).c_str(),
		      arc.weight.Value());
	    }
	  if (t->Final(s) != TropicalWeight::Zero())
	    fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
	}
      }
  }


  void TropicalWeightTransducer::write_in_att_format_number
    (StdVectorFst *t, FILE *ofile)
  {

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	if (s == initial_state) {
	int origin;  // how origin state is printed, see the first comment
	if (s == 0)
	  origin = zero_print;
	else if (s == initial_state)
	  origin = 0;
	else
	  origin = (int)s;
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;
	    fprintf(ofile, "%i\t%i\t\\%i\t\\%i\t%f\n", origin, target,
		    arc.ilabel, arc.olabel,
		    arc.weight.Value());
	  }
	if (t->Final(s) != TropicalWeight::Zero())
	  fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
	break;
	}
      }

    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if (s != initial_state) {
	  int origin;  // how origin state is printed, see the first comment
	  if (s == 0)
	    origin = zero_print;
	  else if (s == initial_state)
	    origin = 0;
	  else
	    origin = (int)s;
	  for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const StdArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;
	      fprintf(ofile, "%i\t%i\t\\%i\t\\%i\t%f\n", origin, target,
		      arc.ilabel, arc.olabel,
		      arc.weight.Value());
	    }
	  if (t->Final(s) != TropicalWeight::Zero())
	    fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
	}
      }
  }


  void TropicalWeightTransducer::write_in_att_format
    (StdVectorFst *t, std::ostream &os)
  {

    const SymbolTable *sym = t->InputSymbols();
    assert(sym != NULL);

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	if (s == initial_state) {
	int origin;  // how origin state is printed, see the first comment
	if (s == 0)
	  origin = zero_print;
	else if (s == initial_state)
	  origin = 0;
	else
	  origin = (int)s;
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;
	    os << origin << "\t" 
	       << target << "\t" 
	       << sym->Find(arc.ilabel).c_str() << "\t"
	       << sym->Find(arc.olabel).c_str() << "\t" 
	       << arc.weight.Value() << "\n";
	  }
	if (t->Final(s) != TropicalWeight::Zero())
	  os << origin << "\t"
	     << t->Final(s).Value() << "\n";
	break;
	}
      }

    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if (s != initial_state) {
	  int origin;  // how origin state is printed, see the first comment
	  if (s == 0)
	    origin = zero_print;
	  else if (s == initial_state)
	    origin = 0;
	  else
	    origin = (int)s;
	  for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const StdArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;
	      os << origin << "\t" 
		 << target << "\t"
		 << sym->Find(arc.ilabel).c_str() << "\t"
		 << sym->Find(arc.olabel).c_str() << "\t"
		 << arc.weight.Value() << "\n";
	    }
	  if (t->Final(s) != TropicalWeight::Zero())
	    os << origin << "\t"
	       << t->Final(s).Value() << "\n";
	}
      }
  }


  void TropicalWeightTransducer::write_in_att_format_number
    (StdVectorFst *t, std::ostream &os)
  {

    //const SymbolTable* sym = t->InputSymbols();

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	if (s == initial_state) {
	int origin;  // how origin state is printed, see the first comment
	if (s == 0)
	  origin = zero_print;
	else if (s == initial_state)
	  origin = 0;
	else
	  origin = (int)s;
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;
	    os << origin << "\t" 
	       << target << "\t" 
	       << "\\" << arc.ilabel << "\t"
	       << "\\" << arc.olabel << "\t" 
	       << arc.weight.Value() << "\n";
	  }
	if (t->Final(s) != TropicalWeight::Zero())
	  os << origin << "\t"
	     << t->Final(s).Value() << "\n";
	break;
	}
      }

    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	StateId s = siter.Value();
	if (s != initial_state) {
	  int origin;  // how origin state is printed, see the first comment
	  if (s == 0)
	    origin = zero_print;
	  else if (s == initial_state)
	    origin = 0;
	  else
	    origin = (int)s;
	  for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const StdArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;
	      os << origin << "\t" 
		 << target << "\t"
		 << "\\" << arc.ilabel << "\t"
		 << "\\" << arc.olabel << "\t"
		 << arc.weight.Value() << "\n";
	    }
	  if (t->Final(s) != TropicalWeight::Zero())
	    os << origin << "\t"
	       << t->Final(s).Value() << "\n";
	}
      }
  }
  

  // AT&T format is handled here ------------------------------

  /* Maps state numbers in AT&T text format to state ids used by 
     OpenFst transducers. */
  typedef std::map<int, StateId> StateMap;

  /* A method used by function 'read_in_att_format'.
     Returns the state id of state number state_number and adds a new
     state to t if state_number is encountered for the first time and
     updates state_map accordingly. */
  StateId TropicalWeightTransducer::
  add_and_map_state(StdVectorFst *t, int state_number, 
		    StateMap &state_map)
  {
    StateMap::iterator it = state_map.find(state_number);
    if (it == state_map.end()) {
      StateId retval  = t->AddState();
      state_map.insert( std::pair<int, StateId>(state_number, retval) );
      return retval;
    }
    else
      return it->second;
  }

  // FIXME: atof and atoi are not necessarily portable...
  // FIXME: sscanf skips spaces
  /* Reads a description of a transducer in AT&T text format and returns 
     a corresponding binary transducer. 
     @note The initial state must be numbered as zero. */
  StdVectorFst * TropicalWeightTransducer::read_in_att_format(FILE * ifile)
  {
    StdVectorFst *t = new StdVectorFst();
    SymbolTable st = create_symbol_table("");

    char line [255];
    StateMap state_map;

    // Add initial state that is numbered as zero.
    StateId initial_state = add_and_map_state(t, 0, state_map);
    t->SetStart(initial_state);


    while ( fgets(line, 255, ifile) != NULL ) 
      {
	if (*line == '-') // transducer separator
	  return t;
	//printf("read line: %s", line);
	char a1 [100]; char a2 [100]; char a3 [100]; 
	char a4 [100]; char a5 [100];
	int n = sscanf(line, "%s\t%s\t%s\t%s\t%s", a1, a2, a3, a4, a5);
	//printf("number of arguments: (%i)\n", n);

	// set value of weight
	float weight = 0;
	if (n == 2)
	  weight = atof(a2);
	if (n == 5)
	  weight = atof(a5);

	if (n == 1 || n == 2)  // final state line
	  {
	    int final_number = atoi(a1);
	    StateId final_state = add_and_map_state(t, final_number, state_map);
	    t->SetFinal(final_state, weight);
	  }

	else if (n == 4 || n == 5)  // transition line
	  {
	    int origin_number = atoi(a1);
	    int target_number = atoi(a2);
	    StateId origin_state 
	      = add_and_map_state(t, origin_number, state_map);
	    StateId target_state 
	      = add_and_map_state(t, target_number, state_map);

	    int input_number = st.AddSymbol(std::string(a3));
	    int output_number = st.AddSymbol(std::string(a4));

	    t->AddArc(origin_state, 
		      StdArc(input_number, output_number, 
			     weight, target_state));
	  }

	else  // line could not be parsed
	  {
	    printf("ERROR: in AT&T file: line: \"%s\"\n", line);
	    throw NotValidAttFormatException();
	  }

      }

    t->SetInputSymbols(&st);
    return t;
  }

  /* 
     Create a copy of this transducer where all transitions of type 
     "?:?", "?:x" and "x:?" are expanded according to the StringSymbolSet 
     'unknown' that lists all symbols previously unknown to this transducer.
  */
  StdVectorFst * TropicalWeightTransducer::expand_arcs
  (StdVectorFst * t, StringSet &unknown,
   bool unknown_symbols_in_use)
  {

    StdVectorFst * result = new StdVectorFst();

    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      result->AddState();

    // go through all states in this
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      {
	// create new state in result, if needed
	StateId s = siter.Value();
	StateId result_s=s;

	// make the new state initial, if needed
	if (t->Start() == s)
	  result->SetStart(result_s);

	// make the new state final, if needed
	if (t->Final(s) != TropicalWeight::Zero())
	  result->SetFinal(result_s, t->Final(s).Value());


	// go through all the arcs in this
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();

	    // find the corresponding target state in result or, if not found,
	    // create a new state
	    StateId result_nextstate=arc.nextstate;

	    // expand the transitions, if needed

	    if (unknown_symbols_in_use) {

	    const fst::SymbolTable *is = t->InputSymbols(); 

	    if ( arc.ilabel == 1 &&       // cross-product "?:?"
		 arc.olabel == 1 )
	      {
		for (StringSet::iterator it1 = unknown.begin(); 
		     it1 != unknown.end(); it1++) 
		  {
		    int64 inumber = is->Find(*it1);
		    for (StringSet::iterator it2 = unknown.begin(); 
			 it2 != unknown.end(); it2++) 
		      {
			int64 onumber = is->Find(*it2);
			if (inumber != onumber)
			  result->AddArc(result_s, 
					 StdArc(inumber, onumber, 
						arc.weight, result_nextstate));
		      }
		    result->AddArc(result_s, 
				   StdArc(inumber, 1, arc.weight, 
					  result_nextstate));
		    result->AddArc(result_s, 
				   StdArc(1, inumber, arc.weight, 
					  result_nextstate));
		  }
	      }
	    else if (arc.ilabel == 2 &&   // identity "?:?"
		     arc.olabel == 2 )       
	      {
		for (StringSet::iterator it = unknown.begin(); 
		     it != unknown.end(); it++) 
		  {
		    int64 number = is->Find(*it);
		    result->AddArc(result_s, 
				   StdArc(number, number, 
					  arc.weight, result_nextstate));
		  }
	      }
	    else if (arc.ilabel == 1)  // "?:x"
	      {
		for (StringSet::iterator it = unknown.begin(); 
		     it != unknown.end(); it++) 
		  {
		    int64 number = is->Find(*it);
		    result->AddArc(result_s, 
				   StdArc(number, arc.olabel, 
					  arc.weight, result_nextstate));
		  }
	      }
	    else if (arc.olabel == 1)  // "x:?"
	      {
		for (StringSet::iterator it = unknown.begin(); 
		     it != unknown.end(); it++) 
		  {
		    int64 number = is->Find(*it);
		    result->AddArc(result_s, 
				   StdArc(arc.ilabel, number, 
					  arc.weight, result_nextstate));
		  }
	      }
	    }

	    // the original transition is copied in all cases
	    result->AddArc(result_s, StdArc(arc.ilabel, arc.olabel, 
					    arc.weight, result_nextstate));    
	  }
      }

    //result->SetInputSymbols(t->InputSymbols());
    return result;
  }

  unsigned int TropicalWeightTransducer::number_of_states(const StdVectorFst *t)
  {
    unsigned int retval=0;
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next())
      retval++;
    return retval;
  }

  std::pair<StdVectorFst*, StdVectorFst*> TropicalWeightTransducer::harmonize
  (StdVectorFst *t1, StdVectorFst *t2, bool unknown_symbols_in_use)
  {
    clock_t startclock = clock();

    // 1. Calculate the set of unknown symbols for transducers t1 and t2.

    StringSet unknown_t1;    // symbols known to another but not this
    StringSet unknown_t2;    // and vice versa

    StringSet t1_symbols = get_alphabet(t1);
    StringSet t2_symbols = get_alphabet(t2);
    collect_unknown_sets(t1_symbols, unknown_t1,
			 t2_symbols, unknown_t2);
    
    // 2. Add new symbols from transducer t1 to the symbol table of 
    //    transducer t2...

    SymbolTable * st2 = t2->InputSymbols()->Copy();
    for ( StringSet::const_iterator it = unknown_t2.begin();
	  it != unknown_t2.end(); it++ ) {
	st2->AddSymbol(*it);
    }
    t2->SetInputSymbols(st2);

    // ...calculate the number mappings needed in harmonization...
    NumberNumberMap km = create_mapping(t1, t2);

    // ... replace the symbol table of t1 with a copy of t2's symbol table
    t1->SetInputSymbols(st2);
    delete st2;

    // ...and recode the symbol numbers of transducer t1 so that
    //    it follows the new symbol table.
    recode_symbol_numbers(t1, km);

    // 3. Calculate the set of symbol pairs to which a non-identity "?:?"
    //    transition is expanded for both transducers.
    
    fst::StdVectorFst *harmonized_t1;
    fst::StdVectorFst *harmonized_t2;

    if (not unknown_symbols_in_use)
      harmonized_t1 = t1;
    else {
      harmonized_t1 = expand_arcs(t1, unknown_t1, unknown_symbols_in_use);
      harmonized_t1->SetInputSymbols(t1->InputSymbols());
    }

    if (not unknown_symbols_in_use)
      harmonized_t2 = t2;
    else {
      harmonized_t2 = expand_arcs(t2, unknown_t2, unknown_symbols_in_use);
      harmonized_t2->SetInputSymbols(t2->InputSymbols());
    }

    clock_t endclock = clock();

    tropical_seconds_in_harmonize = tropical_seconds_in_harmonize + 
      ( (float)(endclock - startclock) / CLOCKS_PER_SEC);

    return std::pair<StdVectorFst*, StdVectorFst*>
      (harmonized_t1, harmonized_t2);

  }



  /* Skip the identifier string "TROPICAL_OFST_TYPE" */
  void TropicalWeightInputStream::skip_identifier_version_3_0(void)
  { input_stream.ignore(19); }

  void TropicalWeightInputStream::skip_hfst_header(void)
  {
    input_stream.ignore(6);
    //char c;
    //i_stream.get(c);
    //switch (c)
    //{
    //case 0:
    skip_identifier_version_3_0();
    //break;
    //default:
    //assert(false);
    //}
  }
  
  void TropicalWeightInputStream::close(void)
  {
    if (filename != string())
      { i_stream.close(); }
  }
  bool TropicalWeightInputStream::is_eof(void) const
  {
    return input_stream.peek() == EOF;
  }
  bool TropicalWeightInputStream::is_bad(void) const
  {
    if (filename == string())
      { return std::cin.bad(); }
    else
      { return input_stream.bad(); }    
  }
  bool TropicalWeightInputStream::is_good(void) const
  {
    if(is_eof())
      return false;
    if (filename == string())
      { return std::cin.good(); }
    else
      { return input_stream.good(); }
  }
  
  bool TropicalWeightInputStream::is_fst(void) const
  {
    return is_fst(input_stream);
  }
  
  bool TropicalWeightInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    int c = getc(f);
    ungetc(c, f);
    return c == 0xd6;
  }
  
  bool TropicalWeightInputStream::is_fst(istream &s)
  {
    return s.good() && (s.peek() == 0xd6);
  }

  bool TropicalWeightInputStream::operator() (void) const
  { return is_good(); }

  void TropicalWeightInputStream::ignore(unsigned int n)
  { input_stream.ignore(n); }

  StdVectorFst * TropicalWeightInputStream::read_transducer()
  {
    if (is_eof())
      { throw StreamIsClosedException(); }
    StdVectorFst * t;
    FstHeader header;
    try 
      {
	if (filename == string())
	  {
	    header.Read(input_stream,"STDIN");			    
	    t = static_cast<StdVectorFst*>
	      (StdVectorFst::Read(input_stream,
				  FstReadOptions("STDIN",
						 &header)));
	  }
	else
	  {
	    header.Read(input_stream,filename);			    
	    t = static_cast<StdVectorFst*>
	      (StdVectorFst::Read(input_stream,
				  FstReadOptions(filename,
						 &header)));
	  }
	if (t == NULL)
	  { throw TransducerHasWrongTypeException(); }
      }
    catch (TransducerHasWrongTypeException e)
      { throw e; }

    try
      {
	return t;
      }
    catch (HfstInterfaceException e)
      { throw e; }
  }


  TropicalWeightStateIterator::TropicalWeightStateIterator(StdVectorFst * t):
    iterator(new StateIterator<StdVectorFst>(*t))
  {}

  TropicalWeightStateIterator::~TropicalWeightStateIterator(void)
  { delete iterator; }

  void TropicalWeightStateIterator::next(void)
  {
    iterator->Next();
  }

  bool TropicalWeightStateIterator::done(void)
  {
    return iterator->Done();
  }

  TropicalWeightState TropicalWeightStateIterator::value(void)
  {
    return iterator->Value();
  }



  TropicalWeightTransition::TropicalWeightTransition
  (const StdArc &arc, StdVectorFst *t):
    arc(arc), t(t)
  {}

  TropicalWeightTransition::~TropicalWeightTransition(void)
  {}

  std::string TropicalWeightTransition::get_input_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.ilabel);
  }

  std::string TropicalWeightTransition::get_output_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.olabel);
  }

  TropicalWeightState TropicalWeightTransition::get_target_state(void) const
  {
    return arc.nextstate;
  }

  TropicalWeight TropicalWeightTransition::get_weight(void) const
  {
    return arc.weight;
  }



  TropicalWeightTransitionIterator::TropicalWeightTransitionIterator
  (StdVectorFst *t, StateId state):
    arc_iterator(new ArcIterator<StdVectorFst>(*t, state)),
    t(t)
  {}

  TropicalWeightTransitionIterator::~TropicalWeightTransitionIterator(void)
  {}

  void TropicalWeightTransitionIterator::next()
  {
    arc_iterator->Next();
  }

  bool TropicalWeightTransitionIterator::done()
  {
    return arc_iterator->Done();
  }

  TropicalWeightTransition TropicalWeightTransitionIterator::value()
  {
    return TropicalWeightTransition(arc_iterator->Value(), this->t);
  }


  fst::SymbolTable TropicalWeightTransducer::create_symbol_table
  (std::string name) {
    fst::SymbolTable st(name);
    st.AddSymbol("@_EPSILON_SYMBOL_@", 0);
    st.AddSymbol("@_UNKNOWN_SYMBOL_@", 1);
    st.AddSymbol("@_IDENTITY_SYMBOL_@", 2);
    return st;
  }
  
  void TropicalWeightTransducer::initialize_symbol_tables(StdVectorFst *t) {
    SymbolTable st = create_symbol_table("");
    t->SetInputSymbols(&st);
  }

  StdVectorFst * TropicalWeightTransducer::create_empty_transducer(void)
  { 
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::create_epsilon_transducer(void)
  { 
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    t->SetFinal(s,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (unsigned int number)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(number,number,0,s2));
    return t;
  }
  StdVectorFst * TropicalWeightTransducer::define_transducer
    (unsigned int inumber, unsigned int onumber)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(inumber,onumber,0,s2));
    return t;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::string &symbol)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(st.AddSymbol(symbol),
			st.AddSymbol(symbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
    (const std::string &isymbol, const std::string &osymbol)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(st.AddSymbol(isymbol),
			st.AddSymbol(osymbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  bool TropicalWeightTransducer::are_equivalent
  (StdVectorFst *a, StdVectorFst *b) 
  {
    StdVectorFst * mina = minimize(a);
    StdVectorFst * minb = minimize(b);
    //write_in_att_format_number(a, stdout);
    //std::cerr << "--\n";
    //write_in_att_format_number(b, stdout);
    //std::cerr << "\n\n";
    EncodeMapper<StdArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<StdArc> enca(*mina, &encode_mapper);
    EncodeFst<StdArc> encb(*minb, &encode_mapper);
    StdVectorFst A(enca);
    StdVectorFst B(encb);
    return Equivalent(A, B);
  }
  
  bool TropicalWeightTransducer::is_cyclic(StdVectorFst * t)
  {
    return t->Properties(kCyclic, true) & kCyclic;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const StringPairVector &spv)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (StringPairVector::const_iterator it = spv.begin();
	 it != spv.end();
	 ++it)
      {
	StateId s2 = t->AddState();
	t->AddArc(s1,StdArc(st.AddSymbol(it->first),
			    st.AddSymbol(it->second),0,s2));
	s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const StringPairSet &sps, bool cyclic)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();  // start state
    t->SetStart(s1);
    StateId s2=s1;               // final state

    if (not sps.empty()) {
      if (not cyclic)
	s2 = t->AddState();
      for (StringPairSet::const_iterator it = sps.begin();
	   it != sps.end();
	   ++it)
	{
	  t->AddArc(s1,StdArc(st.AddSymbol(it->first),
			      st.AddSymbol(it->second),0,s2));
	}
    }
    t->SetFinal(s2,0);
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::vector<StringPairSet> &spsv)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (std::vector<StringPairSet>::const_iterator it = spsv.begin();
	 it != spsv.end();
	 ++it)
      {
	StateId s2 = t->AddState();
	for (StringPairSet::const_iterator it2 = (*it).begin(); 
	     it2 != (*it).end(); it2++ ) {
	  t->AddArc(s1,StdArc(st.AddSymbol(it2->first),
			      st.AddSymbol(it2->second),0,s2));
	}
	s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const NumberPairVector &npv)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (NumberPairVector::const_iterator it = npv.begin();
	 it != npv.end();
	 ++it)
      {
	StateId s2 = t->AddState();
	t->AddArc(s1,StdArc(it->first,it->second,0,s2));
	s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const NumberPairSet &nps, bool cyclic)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();  // start state
    t->SetStart(s1);
    StateId s2=s1;               // final state

    if (not nps.empty()) {
      if (not cyclic)
	s2 = t->AddState();
      for (NumberPairSet::const_iterator it = nps.begin();
	   it != nps.end();
	   ++it)
	{
	  t->AddArc(s1,StdArc(it->first,it->second,0,s2));
	}
    }
    t->SetFinal(s2,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::vector<NumberPairSet> &npsv)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (std::vector<NumberPairSet>::const_iterator it = npsv.begin();
	 it != npsv.end();
	 ++it)
      {
	StateId s2 = t->AddState();
	for (NumberPairSet::const_iterator it2 = (*it).begin(); 
	     it2 != (*it).end(); it2++ ) {
	  t->AddArc(s1,StdArc(it2->first,it2->second,0,s2));
	}
	s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }


  StdVectorFst * 
  TropicalWeightTransducer::copy(StdVectorFst * t)
  { return new StdVectorFst(*t); }


  StdVectorFst * 
  TropicalWeightTransducer::determinize(StdVectorFst * t)
  {
    RmEpsilon<StdArc>(t);
    EncodeMapper<StdArc> encode_mapper(kEncodeLabels|kEncodeWeights,ENCODE);
    Encode(t, &encode_mapper);
    StdVectorFst * det = new StdVectorFst();
    Determinize<StdArc>(*t, det);
    Decode(det, encode_mapper);
    return det;
  }


  StdVectorFst * TropicalWeightTransducer::minimize
  (StdVectorFst * t)
  {
    RmEpsilon<StdArc>(t);
    EncodeMapper<StdArc> encode_mapper(kEncodeLabels|kEncodeWeights,ENCODE);
    Encode(t, &encode_mapper);
    StdVectorFst * det = new StdVectorFst();
    Determinize<StdArc>(*t, det);
    Minimize<StdArc>(det);
    Decode(det, encode_mapper);
    return det;
  }


  void print_att_number(StdVectorFst *t, FILE * ofile) {
    fprintf(ofile, "initial state: %i\n", t->Start());
    for (fst::StateIterator<StdVectorFst> siter(*t); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	if ( t->Final(s) != TropicalWeight::Zero() )
	  fprintf(ofile, "%i\t%f\n", s, t->Final(s).Value());
	for (fst::ArcIterator<StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    fprintf(ofile, "%i\t%i\t%i\t%i\t%f\n", 
		    s, arc.nextstate, arc.ilabel, arc.olabel, 
		    arc.weight.Value());
	  }
      }
  }

  void TropicalWeightTransducer::test_minimize(void)
  {
    StdVectorFst * t = new StdVectorFst();
    StateId initial = t->AddState();
    t->SetStart(initial);
    StateId state = t->AddState();
    t->SetFinal(state, 0.5);
    t->AddArc(initial, StdArc(1,1,0.5,state));
    t->AddArc(state, StdArc(2,2,0.5,initial));
 
    // print t before minimize
    print_att_number(t, stderr);

    // epsilon removal
    RmEpsilonFst<StdArc> t_rm_eps(*t);

    // encode mapping
    EncodeMapper<StdArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<StdArc> t_rm_eps_enc(t_rm_eps,
				   &encode_mapper);

    // determinization
    DeterminizeFst<StdArc> t_DET(t_rm_eps_enc);

    // minimization
    DecodeFst<StdArc> dec(t_DET,
			  encode_mapper);
    StdVectorFst t_det_std(t_DET);

    Minimize<StdArc>(&t_det_std);

    // print t after minimize
    print_att_number(&t_det_std, stderr);

    // decoding
    //Decode<StdArc>(&t_det, encode_mapper);  COMMENTED
  }

  /* For HfstMutableTransducer */

  StateId 
  TropicalWeightTransducer::add_state(StdVectorFst *t)
  { 
    StateId s = t->AddState();
    if (s == 0)
      t->SetStart(s);
    return s;
  }

  void 
  TropicalWeightTransducer::set_final_weight
  (StdVectorFst *t, StateId s, float w)
  {
    t->SetFinal(s, w);
    return;
  }

  void 
  TropicalWeightTransducer::add_transition
  (StdVectorFst *t, StateId source, std::string &isymbol, 
   std::string &osymbol, float w, StateId target)
  {
    SymbolTable *st = t->InputSymbols()->Copy();
    /*if (t->InputSymbols() != t->OutputSymbols()) {
      fprintf(stderr, "ERROR:  TropicalWeightTransducer::add_transition:
      input and output symbols are not the same\n"); 
      throw hfst::exceptions::ErrorException(); 
      }*/
    unsigned int ilabel = st->AddSymbol(isymbol);
    unsigned int olabel = st->AddSymbol(osymbol);
    t->AddArc(source, StdArc(ilabel, olabel, w, target));
    t->SetInputSymbols(st);
    delete st;
    return;
  }

  float 
  TropicalWeightTransducer::get_final_weight(StdVectorFst *t, StateId s)
  {
    return t->Final(s).Value();
  }

  float 
  TropicalWeightTransducer::is_final(StdVectorFst *t, StateId s)
  {
    return ( t->Final(s) != TropicalWeight::Zero() );
  }

  StateId
  TropicalWeightTransducer::get_initial_state(StdVectorFst *t)
  {
    return t->Start();
  }

  StdVectorFst * 
  TropicalWeightTransducer::remove_epsilons(StdVectorFst * t)
  { return new StdVectorFst(RmEpsilonFst<StdArc>(*t)); }

  StdVectorFst * 
  TropicalWeightTransducer::n_best(StdVectorFst * t, unsigned int n)
  { 
    StdVectorFst * n_best_fst = new StdVectorFst(); 
    fst::ShortestPath(*t,n_best_fst,(size_t)n);
    return n_best_fst;
  }

  StdVectorFst * 
  TropicalWeightTransducer::repeat_star(StdVectorFst * t)
  { return new StdVectorFst(ClosureFst<StdArc>(*t,CLOSURE_STAR)); }

  StdVectorFst * 
  TropicalWeightTransducer::repeat_plus(StdVectorFst * t)
  { return new StdVectorFst(ClosureFst<StdArc>(*t,CLOSURE_PLUS)); }

  StdVectorFst *
  TropicalWeightTransducer::repeat_n(StdVectorFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    StdVectorFst * repetition = create_epsilon_transducer();
    for (int i = 0; i < n; ++i)
      { Concat(repetition,*t); }
    repetition->SetInputSymbols(t->InputSymbols());
    return repetition;
  }

  StdVectorFst *
  TropicalWeightTransducer::repeat_le_n(StdVectorFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    StdVectorFst * repetition = create_epsilon_transducer();
    for (int i = 0; i < n; ++i)
      {
	StdVectorFst * optional_t = optionalize(t);
	Concat(repetition,*optional_t);
	delete optional_t;
      }
    repetition->SetInputSymbols(t->InputSymbols());
    return repetition;
  }

  StdVectorFst * 
  TropicalWeightTransducer::optionalize(StdVectorFst * t)
  {
    StdVectorFst * eps = create_epsilon_transducer();
    Union(eps,*t);
    eps->SetInputSymbols(t->InputSymbols());
    return eps;
  }


  StdVectorFst * 
  TropicalWeightTransducer::invert(StdVectorFst * t)
  {
    StdVectorFst * inverse = copy(t);
    Invert(inverse);
    inverse->SetInputSymbols(t->InputSymbols());
    return inverse;
  }

  /* Makes valgrind angry... */
  StdVectorFst * 
  TropicalWeightTransducer::reverse(StdVectorFst * t)
  {
    StdVectorFst * reversed = new StdVectorFst;
    Reverse<StdArc,StdArc>(*t,reversed);
    reversed->SetInputSymbols(t->InputSymbols());
    return reversed;
  }

  StdVectorFst * TropicalWeightTransducer::extract_input_language
  (StdVectorFst * t)
  { StdVectorFst * retval =  new StdVectorFst(ProjectFst<StdArc>
					      (*t,PROJECT_INPUT)); 
    retval->SetInputSymbols(t->InputSymbols());
    return retval; }

  StdVectorFst * TropicalWeightTransducer::extract_output_language
  (StdVectorFst * t)
  { StdVectorFst * retval = new StdVectorFst(ProjectFst<StdArc>
					     (*t,PROJECT_OUTPUT)); 
    retval->SetInputSymbols(t->InputSymbols());
    return retval; }
  
  typedef std::pair<int,int> LabelPair;
  typedef std::vector<LabelPair> LabelPairVector;



  static StdVectorFst * substitute(StdVectorFst * t,
				   StringPair old_symbol_pair,
				   StringPair new_symbol_pair);
  static StdVectorFst * substitute(StdVectorFst * t,
				   const StringPair old_symbol_pair,
				   StdVectorFst *transducer);

  StdVectorFst * TropicalWeightTransducer::insert_freely
  (StdVectorFst * t, const StringPair &symbol_pair)
  {
    SymbolTable * st = t->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<fst::StdFst> siter(*t); 
	 !siter.Done(); siter.Next()) {
      StateId state_id = siter.Value();
      t->AddArc(state_id, 
		fst::StdArc(st->AddSymbol(symbol_pair.first), 
			    st->AddSymbol(symbol_pair.second), 0, state_id));
    }
    t->SetInputSymbols(st);
    delete st;
    return t;
  }

  static StdVectorFst * insert_freely
  (StdVectorFst * t, const NumberPair &number_pair) 
  {
    for (fst::StateIterator<fst::StdFst> siter(*t); 
	 !siter.Done(); siter.Next()) {
      StateId state_id = siter.Value();
      t->AddArc(state_id, 
		fst::StdArc(number_pair.first, number_pair.second, 
			    0, state_id));
    }
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst * t,unsigned int old_key,unsigned int new_key)
  {
    LabelPairVector v;
    v.push_back(LabelPair(old_key,new_key));
    RelabelFst<StdArc> t_subst(*t,v,v);
    return new StdVectorFst(t_subst);
  }
  
  void TropicalWeightTransducer::set_symbol_table
  (StdVectorFst * t, 
   std::vector<std::pair<unsigned short, std::string> > symbol_mappings)
  {
    SymbolTable st = create_symbol_table("");
    for (unsigned int i=0; i<symbol_mappings.size(); i++)
      {
	st.AddSymbol(symbol_mappings[i].second, 
		     symbol_mappings[i].first);
      }
    t->SetInputSymbols(&st);
  }


  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst * t,
   pair<unsigned int, unsigned int> old_key_pair,
   pair<unsigned int, unsigned int> new_key_pair)
  {
    EncodeMapper<StdArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<StdArc> enc(*t,&encode_mapper);

    StdArc old_pair_code = 
      encode_mapper(StdArc(old_key_pair.first,old_key_pair.second,0,0));
    StdArc new_pair_code =
      encode_mapper(StdArc(new_key_pair.first,new_key_pair.second,0,0));

    // First cast up, then cast down... 
    // For some reason dynamic_cast<StdVectorFst*>
    // doesn't work although both EncodeFst<StdArc> and StdVectorFst 
    // extend Fst<StdArc>. 
    // reinterpret_cast worked, but that is apparently unsafe...
    StdVectorFst * subst = 
      substitute(static_cast<StdVectorFst*>(static_cast<Fst<StdArc>*>(&enc)),
		 static_cast<unsigned int>(old_pair_code.ilabel),
		 static_cast<unsigned int>(new_pair_code.ilabel));

    DecodeFst<StdArc> dec(*subst,encode_mapper);
    delete subst;
    return new StdVectorFst(dec);
  }

  StdVectorFst * TropicalWeightTransducer::substitute
    (StdVectorFst *t,
     StringPair old_symbol_pair,
     StringPairSet new_symbol_pair_set)
  {
    fst::StdVectorFst * tc = t->Copy();
    fst::SymbolTable * st = tc->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<fst::StdVectorFst> siter(*tc); 
	 not siter.Done(); siter.Next()) 
      {
	StateId s = siter.Value();
	for (fst::MutableArcIterator<StdVectorFst> aiter(tc,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const StdArc &arc = aiter.Value();
	    if ( strcmp( st->Find(arc.ilabel).c_str(), 
			 old_symbol_pair.first.c_str() ) == 0 &&
		 strcmp( st->Find(arc.olabel).c_str(), 
			 old_symbol_pair.second.c_str() ) == 0 )
	      {
		bool first_substitution=true;
		for (StringPairSet::iterator it = new_symbol_pair_set.begin(); 
		     it != new_symbol_pair_set.end(); it++)
		  {
		    if (first_substitution) {
		      StdArc new_arc;
		      new_arc.ilabel = st->AddSymbol(it->first);
		      new_arc.olabel = st->AddSymbol(it->second);
		      new_arc.weight = arc.weight.Value();
		      new_arc.nextstate = arc.nextstate;
		      aiter.SetValue(new_arc); 
		      first_substitution=false; }
		    else
		      tc->AddArc(s, StdArc(st->AddSymbol(it->first), 
					   st->AddSymbol(it->second), 
					   arc.weight.Value(), 
					   arc.nextstate));
		  }
	      }
	  }
      }
    tc->SetInputSymbols(st);
    delete st;
    return tc;
  }

  StdVectorFst * TropicalWeightTransducer::substitute(StdVectorFst *t,
						      std::string old_symbol,
						      std::string new_symbol)
  {
    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();
    StdVectorFst * retval 
      = substitute(t, st->AddSymbol(old_symbol), st->AddSymbol(new_symbol));
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   StringPair old_symbol_pair,
   StringPair new_symbol_pair)
  {
    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();
    pair<unsigned int, unsigned int> old_pair
      (st->AddSymbol(old_symbol_pair.first),
       st->AddSymbol(old_symbol_pair.second));
    pair<unsigned int, unsigned int> new_pair
      (st->AddSymbol(new_symbol_pair.first),
       st->AddSymbol(new_symbol_pair.second));
    StdVectorFst * retval = substitute(t, old_pair, new_pair);
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   const StringPair old_symbol_pair,
   StdVectorFst *transducer)
  {

    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();

    int states = t->NumStates();
    for( int i = 0; i < states; ++i ) {

      for (fst::MutableArcIterator<fst::StdVectorFst> it(t,i);
	   not it.Done();
	   it.Next()) {

	fst::StdArc arc = it.Value();

	// find arcs that must be replaced
	if ( arc.ilabel == st->AddSymbol(old_symbol_pair.first) && 
	     arc.olabel == st->AddSymbol(old_symbol_pair.second) ) 
	  {

	  StateId destination_state = arc.nextstate;
	  StateId start_state = t->AddState();

	  // change the label of the arc to epsilon and point 
	  // the arc to a new state
	  arc.ilabel = 0;
	  arc.olabel = 0;
	  arc.nextstate = start_state;  
	  // weight remains the same
	  it.SetValue(arc);


	  // add rest of the states to transducer t
	  int states_to_add = transducer->NumStates();
	  for (int j=1; j<states_to_add; j++)
	    t->AddState();


	  // go through all states and arcs in replace transducer tr
	  for (fst::StateIterator<fst::StdFst> siter(*transducer); 
	       !siter.Done(); siter.Next()) {

	    StateId tr_state_id = siter.Value();

	    // final states in tr correspond in t to a non-final state which has
	    // an epsilon transition to original destination state of arc that
	    // is being replaced
	    if ( is_final( transducer, tr_state_id ) )
	      t->AddArc( tr_state_id + start_state,
			 fst::StdArc( 0,
				      0,
				      // final weight is copied to 
				      // the epsilon transition
				      transducer->Final(tr_state_id),  
				      destination_state
				      )
			 );  

	    for (fst::ArcIterator<fst::StdFst> aiter(*transducer, tr_state_id);
		 !aiter.Done(); aiter.Next()) {

	      const fst::StdArc &tr_arc = aiter.Value();

	      // adding arc from state 'tr_state_id+start_state' 
	      // to state 'tr_arc.nextstate'
	      // copy arcs from tr to t
	      t->AddArc( tr_state_id + start_state, 
			 fst::StdArc( tr_arc.ilabel, 
				      tr_arc.olabel, 
				      // weight remains the same 
				      tr_arc.weight,  
				      tr_arc.nextstate + start_state 
				      ) 
			 );

	    }
	  }
	}
      }
    }

    t->SetInputSymbols(st);
    delete st;
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   const NumberPair old_number_pair,
   StdVectorFst *transducer)
  {

    int states = t->NumStates();
    for( int i = 0; i < states; ++i ) {

      for (fst::MutableArcIterator<fst::StdVectorFst> it(t,i);
	   not it.Done();
	   it.Next()) {

	fst::StdArc arc = it.Value();

	// find arcs that must be replaced
	if ( arc.ilabel == old_number_pair.first && 
	     arc.olabel == old_number_pair.second ) 
	  {

	  StateId destination_state = arc.nextstate;
	  StateId start_state = t->AddState();

	  // change the label of the arc to epsilon and point 
	  // the arc to a new state
	  arc.ilabel = 0;
	  arc.olabel = 0;
	  arc.nextstate = start_state;  
	  // weight remains the same
	  it.SetValue(arc);


	  // add rest of the states to transducer t
	  int states_to_add = transducer->NumStates();
	  for (int j=1; j<states_to_add; j++)
	    t->AddState();


	  // go through all states and arcs in replace transducer tr
	  for (fst::StateIterator<fst::StdFst> siter(*transducer); 
	       !siter.Done(); siter.Next()) {

	    StateId tr_state_id = siter.Value();

	    // final states in tr correspond in t to a non-final state which has
	    // an epsilon transition to original destination state of arc that
	    // is being replaced
	    if ( is_final( transducer, tr_state_id ) )
	      t->AddArc( tr_state_id + start_state,
			 fst::StdArc( 0,
				      0,
				      // final weight is copied to 
				      // the epsilon transition
				      transducer->Final(tr_state_id),  
				      destination_state
				      )
			 );  

	    for (fst::ArcIterator<fst::StdFst> aiter(*transducer, tr_state_id);
		 !aiter.Done(); aiter.Next()) {

	      const fst::StdArc &tr_arc = aiter.Value();

	      // adding arc from state 'tr_state_id+start_state' 
	      // to state 'tr_arc.nextstate'
	      // copy arcs from tr to t
	      t->AddArc( tr_state_id + start_state, 
			 fst::StdArc( tr_arc.ilabel, 
				      tr_arc.olabel, 
				      // weight remains the same 
				      tr_arc.weight,  
				      tr_arc.nextstate + start_state 
				      ) 
			 );
	      
	    }
	  }
	}
      }
    }

    return t;
  }

  StdVectorFst * TropicalWeightTransducer::compose(StdVectorFst * t1,
			 StdVectorFst * t2)
  {
    StringSet foo;
    // a copy of t2 is created so that its symbol table check sum 
    // is the same as t1's
    // (else OpenFst complains about non-matching check sums... )
    StdVectorFst * t2_ = expand_arcs(t2, foo, false);
    t2_->SetInputSymbols(t1->InputSymbols());
    t1->SetOutputSymbols(t1->InputSymbols());
    
    ArcSort(t1, StdOLabelCompare());
    ArcSort(t2_, StdILabelCompare());

    StdVectorFst *result = new StdVectorFst();
    Compose(*t1, *t2_, result);
    delete t2_;

    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

#ifdef FOO
  StdVectorFst * TropicalWeightTransducer::compose_intersect
  (StdVectorFst * t, Grammar * grammar)
  {
    fst::ArcSort<StdArc,fst::OLabelCompare<StdArc> > 
      (t,OLabelCompare<StdArc>());    

    ComposeIntersectFst cif(t,*grammar);
    StdVectorFst * result = cif();
    result->SetInputSymbols(NULL);
    result->SetOutputSymbols(NULL);
    result->SetInputSymbols(t->InputSymbols() );
    return result;
  }
#endif

  StdVectorFst * TropicalWeightTransducer::concatenate(StdVectorFst * t1,
						       StdVectorFst * t2)
  {
    StdVectorFst * result = new StdVectorFst(*t1);
    Concat(result,*t2);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct(StdVectorFst * t1,
						    StdVectorFst * t2)
  {
    StdVectorFst * result = new StdVectorFst(*t1);
    Union(result,*t2);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct
  (StdVectorFst * t, const StringPairVector &spv)
  {
    SymbolTable * st = t->InputSymbols()->Copy();
    assert(st != NULL);

    StateId s = t->Start();

    for (StringPairVector::const_iterator it = spv.begin(); 
	 it != spv.end(); it++) 
      {
	unsigned int inumber = st->AddSymbol(it->first.c_str());
	unsigned int onumber = st->AddSymbol(it->second.c_str());

	bool transition_found=false;
	for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
	  {
	    const StdArc& a = it.Value();
	    if (a.ilabel == inumber && a.olabel == onumber) {
	      transition_found=true;
	      s = a.nextstate;
	      break;
	    }
	  }

	if (not transition_found) {
	  StateId new_state = t->AddState();
	  t->AddArc(s, StdArc(inumber,onumber,0,new_state));
	  s = new_state;
	}
      }

    t->SetFinal(s,0);
    
    t->SetInputSymbols(st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct
  (StdVectorFst * t, const NumberPairVector &npv)
  {
    StateId s = t->Start();

    for (NumberPairVector::const_iterator it = npv.begin(); 
	 it != npv.end(); it++) 
      {
	unsigned int inumber = it->first;
	unsigned int onumber = it->second;

	bool transition_found=false;
	for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
	  {
	    const StdArc& a = it.Value();
	    if (a.ilabel == inumber && a.olabel == onumber) {
	      transition_found=true;
	      s = a.nextstate;
	      break;
	    }
	  }

	if (not transition_found) {
	  StateId new_state = t->AddState();
	  t->AddArc(s, StdArc(inumber,onumber,0,new_state));
	  s = new_state;
	}
      }

    t->SetFinal(s,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::intersect(StdVectorFst * t1,
			   StdVectorFst * t2)
  {
    if (t1->OutputSymbols() == NULL)
      t1->SetOutputSymbols(t1->InputSymbols());
    if (t2->OutputSymbols() == NULL)
      t2->SetOutputSymbols(t2->InputSymbols());

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2, ILabelCompare<StdArc>());

    RmEpsilon(t1);
    RmEpsilon(t2);

    EncodeMapper<StdArc> encoder(0x0001,ENCODE);
    EncodeFst<StdArc> enc1(*t1, &encoder);
    EncodeFst<StdArc> enc2(*t2, &encoder);
    DeterminizeFst<StdArc> det1(enc1);
    DeterminizeFst<StdArc> det2(enc2);

    IntersectFst<StdArc> intersect(det1,det2);
    StdVectorFst *foo = new StdVectorFst(intersect);
    DecodeFst<StdArc> decode(*foo, encoder);
    delete foo;
    StdVectorFst *result = new StdVectorFst(decode);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::subtract(StdVectorFst * t1,
			  StdVectorFst * t2)
  {
    bool DEBUG=false;

    if (DEBUG) printf("Tropical subtract...\n");

    if (t1->OutputSymbols() == NULL)
      t1->SetOutputSymbols(t1->InputSymbols());
    if (t2->OutputSymbols() == NULL)
      t2->SetOutputSymbols(t2->InputSymbols());

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2, ILabelCompare<StdArc>());

    //RmEpsilonFst<StdArc> rm1(*t1);
    //RmEpsilonFst<StdArc> rm2(*t2);

    RmEpsilon(t1);
    RmEpsilon(t2);

    if (DEBUG) printf("  ..epsilons removed\n");

    EncodeMapper<StdArc> encoder(0x0003,ENCODE); // t2 must be unweighted
    //EncodeFst<StdArc> enc1(rm1, &encoder);
    //EncodeFst<StdArc> enc2(rm2, &encoder);
    EncodeFst<StdArc> enc1(*t1, &encoder);
    EncodeFst<StdArc> enc2(*t2, &encoder);
    DeterminizeFst<StdArc> det1(enc1);
    DeterminizeFst<StdArc> det2(enc2);

    if (DEBUG) printf("  ..determinized\n");

    StdVectorFst *difference = new StdVectorFst();
    Difference(det1, det2, difference);
    DecodeFst<StdArc> subtract(*difference, encoder);
    delete difference;

    if (DEBUG) printf("  ..subtracted\n");

    //DifferenceFst<StdArc> subtract(enc1,enc2);
    StdVectorFst *result = new StdVectorFst(subtract); 
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::set_weight(StdVectorFst * t,float f)
  {
    StdVectorFst * t_copy = new StdVectorFst(*t);
    for (fst::StateIterator<StdVectorFst> iter(*t); 
	 not iter.Done(); iter.Next())
      {
	if (t_copy->Final(iter.Value()) != fst::TropicalWeight::Zero())
	  { t_copy->SetFinal(iter.Value(),f); }
      }
    return t_copy;
  }
  

  // ----- TRIE FUNCTIONS BEGINS -----

  int TropicalWeightTransducer::has_arc(fst::StdVectorFst &t,
	      StdArc::StateId sourcestate,			  
	      StdArc::Label ilabel, 
	      StdArc::Label olabel)
  {
    for (fst::ArcIterator<StdVectorFst> aiter(t,sourcestate);
	 !aiter.Done();
	 aiter.Next())
      {
	if ((aiter.Value().ilabel == ilabel) and 
	    (aiter.Value().olabel == olabel))
	  { return aiter.Position(); }
      }

    return -1;    
  }

  void TropicalWeightTransducer::disjunct_as_tries(fst::StdVectorFst &t1,
			 StateId t1_state,
			 const fst::StdVectorFst * t2,
			 StateId t2_state)
  {
    if (t2->Final(t2_state) != fst::TropicalWeight::Zero())
      {
	t1.SetFinal(t1_state,
		    Plus(t1.Final(t1_state),
			 t2->Final(t2_state)));
      }
    for (fst::ArcIterator<StdVectorFst> aiter(*t2,t2_state);
	 !aiter.Done();
	 aiter.Next())
      {
	const StdArc &arc = aiter.Value();
	int arc_index = has_arc(t1,t1_state,arc.ilabel,arc.olabel);
	if (arc_index == -1)
	  {
	    StdArc::StateId new_state = t1.AddState();
	    t1.AddArc(t1_state,StdArc(arc.ilabel,
				      arc.olabel,
				      arc.weight,
				      new_state));
	    add_sub_trie(t1,new_state,t2,arc.nextstate);
	  }
	else
	  {
	    MutableArcIterator<fst::StdVectorFst> ajter(&t1,t1_state);
	    ajter.Seek(arc_index);
	    disjunct_as_tries(t1,ajter.Value().nextstate,t2,arc.nextstate);
	  }
      }
  }
  
  void TropicalWeightTransducer::add_sub_trie(fst::StdVectorFst &t1,
		  StateId t1_state,
		  const fst::StdVectorFst * t2,
		  StateId t2_state)
  {
    if (t2->Final(t2_state) != fst::TropicalWeight::Zero())
      {
	t1.SetFinal(t1_state,
		    Plus(t1.Final(t1_state),
			 t2->Final(t2_state)));
      }
    for (fst::ArcIterator<fst::StdVectorFst> aiter(*t2,t2_state);
	 !aiter.Done();
	 aiter.Next())
      {
	const StdArc &arc = aiter.Value();
	StdArc::StateId new_state = t1.AddState();
	t1.AddArc(t1_state,fst::StdArc(arc.ilabel,
				  arc.olabel,
				  arc.weight,
				  new_state));
	add_sub_trie(t1,new_state,t2,arc.nextstate);
      }
  }

  // ----- TRIE FUNCTIONS END -----


  static bool extract_strings
  (StdVectorFst * t, StdArc::StateId s,
   std::map<StateId,unsigned short> all_visitations, 
   std::map<StateId, unsigned short> path_visitations,
   std::vector<char>& lbuffer, int lpos, 
   std::vector<char>& ubuffer, int upos, float weight_sum,
   hfst::ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<int64> >* fd_state_stack, 
   bool filter_fd, 
   bool include_spv, StringPairVector &spv)
  { 
    if(cycles >= 0 && path_visitations[s] > cycles)
      return true;
    all_visitations[s]++;
    path_visitations[s]++;
    
    if(lpos > 0 && upos > 0)
    {
      lbuffer[lpos]=0;
      ubuffer[upos]=0;
      bool final = t->Final(s) != TropicalWeight::Zero();
      hfst::WeightedPath<float> path
	(&lbuffer[0],&ubuffer[0],weight_sum+(final?t->Final(s).Value():0));
      if (include_spv) {
	path.spv = spv;
	path.is_spv_in_use = true;
      }
      hfst::ExtractStringsCb::RetVal ret = callback(path, final);
      if(!ret.continueSearch || !ret.continuePath)
      {
        path_visitations[s]--;
        return ret.continueSearch;
      }
    }
    
    // sort arcs by number of visitations
    vector<const StdArc*> arcs;
    for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
    {
      const StdArc& a = it.Value();
      size_t i;
      for( i=0; i<arcs.size(); i++ )
        if (all_visitations[a.nextstate] < all_visitations[arcs[i]->nextstate])
          break;
      arcs.push_back(NULL);
      for( size_t k=arcs.size()-1; k>i; k-- )
        arcs[k] = arcs[k-1];
      arcs[i] = &a;
    }
    
    bool res = true;
    for( size_t i=0; i<arcs.size() && res == true; i++ )
    {
      const StdArc &arc = *(arcs[i]);
      bool added_fd_state = false;
    
      if (fd_state_stack) {
        if(fd_state_stack->back().get_table().get_operation(arc.ilabel) 
	   != NULL) {
          fd_state_stack->push_back(fd_state_stack->back());
          if(fd_state_stack->back().apply_operation(arc.ilabel))
            added_fd_state = true;
          else {
            fd_state_stack->pop_back();
            continue; // don't follow the transition
          }
        }
      }
      
      int lp=lpos;
      int up=upos;
      
      if ((!filter_fd || 
	   fd_state_stack->back().get_table().get_operation(arc.ilabel)==NULL))
      {
        std::string str = t->InputSymbols()->Find(arc.ilabel);
	if(str.compare("@_EPSILON_SYMBOL_@") != 0) {
	  if(lpos+str.length() >= lbuffer.size())
	    lbuffer.resize(lbuffer.size()*2, 0);
	  strcpy(&lbuffer[lpos], str.c_str());
	  lp += str.length();
	}
      }
      if ( (!filter_fd || 
	    fd_state_stack->back().get_table().get_operation(arc.olabel)==NULL))
      {
        std::string str = t->InputSymbols()->Find(arc.olabel);
	if(str.compare("@_EPSILON_SYMBOL_@") != 0) {
	  if(upos+str.length() > ubuffer.size())
	    ubuffer.resize(ubuffer.size()*2, 0);
	  strcpy(&ubuffer[upos], str.c_str());
	  up += str.length();
	}
      }
      
      /* Handle spv here. Special symbols (flags, epsilons) 
	 are always inserted. */
      if (include_spv) {
	StringPair string_pair(t->InputSymbols()->Find(arc.ilabel),
			       t->InputSymbols()->Find(arc.olabel));
	spv.push_back(string_pair);
      }

      res = extract_strings
	(t, arc.nextstate, all_visitations, path_visitations,
	 lbuffer,lp, ubuffer,up, weight_sum+arc.weight.Value(), 
	 callback, cycles, fd_state_stack, filter_fd,
	 include_spv, spv);
      
      if (include_spv)
	spv.pop_back();

      if(added_fd_state)
        fd_state_stack->pop_back();
    }
    
    path_visitations[s]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
  void TropicalWeightTransducer::extract_strings
  (StdVectorFst * t, hfst::ExtractStringsCb& callback,
   int cycles, FdTable<int64>* fd, bool filter_fd, bool include_spv)
  {
    if (t->Start() == -1)
      return;
    
    vector<char> lbuffer(BUFFER_START_SIZE, 0);
    vector<char> ubuffer(BUFFER_START_SIZE, 0);
    map<StateId, unsigned short> all_visitations;
    map<StateId, unsigned short> path_visitations;
    std::vector<hfst::FdState<int64> >* fd_state_stack 
      = (fd==NULL) ? NULL : new std::vector<hfst::FdState<int64> >
      (1, hfst::FdState<int64>(*fd));
    
    StringPairVector spv;
    hfst::implementations::extract_strings
      (t,t->Start(),all_visitations,path_visitations,
       lbuffer,0,ubuffer,0,0.0f,callback,cycles,fd_state_stack,filter_fd, 
       include_spv, spv);
  }
  
  FdTable<int64>* TropicalWeightTransducer::get_flag_diacritics
  (StdVectorFst * t)
  {
    FdTable<int64>* table = new FdTable<int64>();
    const fst::SymbolTable* symbols = t->InputSymbols();
    for(fst::SymbolTableIterator it=fst::SymbolTableIterator(*symbols); 
	!it.Done(); it.Next())
    {
      if(FdOperation::is_diacritic(it.Symbol()))
        table->define_diacritic(it.Value(), it.Symbol());
    }
    return table;
  }

  void TropicalWeightTransducer::
  represent_empty_transducer_as_having_one_state(StdVectorFst *t)
  {
    if (t->Start() == fst::kNoStateId || t->NumStates() == 0) {
      delete t;
      t = create_empty_transducer();
    }
    return;
  }



  TropicalWeightOutputStream::TropicalWeightOutputStream(bool hfst_format):
    filename(std::string()), output_stream(std::cout), hfst_format(hfst_format)
  {
    if (!output_stream)
      fprintf(stderr, "TropicalWeightOutputStream: ERROR: failbit set (3).\n");
  }

  TropicalWeightOutputStream::TropicalWeightOutputStream
  (const std::string &str, bool hfst_format):
    filename(std::string(str)),o_stream(str.c_str(),std::ios::out),
    output_stream(o_stream),hfst_format(hfst_format)
  {}

  void TropicalWeightOutputStream::write(const char &c)
  {
    output_stream.put(char(c));
  }

  void TropicalWeightOutputStream::write_transducer(StdVectorFst * transducer) 
  { 
    if (!output_stream)
      fprintf(stderr, "TropicalWeightOutputStream: ERROR: failbit set (1).\n");
    /* When writing a transducer in the backend format,
       both input and output symbol tables are included. */
    fst::SymbolTable *output_st=NULL;
    if (hfst_format) {
      output_st = new fst::SymbolTable(*(transducer->InputSymbols()));
      transducer->SetOutputSymbols(output_st);
    }
    transducer->Write(output_stream,FstWriteOptions()); 
    if (output_st != NULL)
      delete output_st;
  }


  void TropicalWeightOutputStream::close(void) 
  {
    if (filename != string())
      { o_stream.close(); }
  }
  }
}

  




#else
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace hfst::implementations;

int main(void) 
{
    std::cout << "Unit tests for " __FILE__ ":";
  TropicalWeightTransducer ofst;
  StdVectorFst * t = ofst.create_empty_transducer();
  delete t;
  t = ofst.create_epsilon_transducer();
  delete t;
    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif
