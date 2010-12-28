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
#  include <config.h>
#endif

#include "ConvertTransducerFormat.h"
#include "optimized-lookup/convert.h"

#ifndef DEBUG_MAIN
namespace hfst { namespace implementations
{

#if HAVE_SFST
  /* Recursively copy all transitions of \a node to \a net.
     Used by function sfst_to_hfst_net(SFST::Transducer * t).

     @param node  The current node in the SFST transducer
     @param index  A map that maps nodes to integers
     @param visited_nodes  Which nodes have already been visited
     @param net  The HfstFsm that is being created
     @param alphabet  The alphabet of the SFST transducer
  */
  void sfst_to_hfst_net( SFST::Node *node, SFST::NodeNumbering &index, 
			 std::set<SFST::Node*> &visited_nodes, 
			 HfstFsm *net, SFST::Alphabet &alphabet ) {
  
    // If node has not been visited before
    if (visited_nodes.find(node) == visited_nodes.end() ) { 
      visited_nodes.insert(node);
      SFST::Arcs *arcs=node->arcs();

      // Go through all transitions and copy them to \a net
      for( SFST::ArcsIter p(arcs); p; p++ ) {
	SFST::Arc *arc=p;
	net->add_transition(index[node], 
			    HfstArc
			    (index[arc->target_node()],
			     std::string(alphabet->code2symbol
					 (arc->label().lower_char())),
			     std::string(alphabet->code2symbol
					 (arc->label().upper_char())),
			     0));
      }

      if (node->is_final())
	net->set_final(index[node],0);

      // Call this function recursively for all target nodes
      // of the transitions
      for( SFST::ArcsIter p(arcs); p; p++ ) {
	SFST::Arc *arc=p;
	sfst_to_internal(arc->target_node(), index, 
			 visited_nodes, 
			 internal, alphabet);
      }
    }
  }

  /* Create an HfstFsm equivalent to an SFST transducer \a t. */
  HfstFsm * sfst_to_hfst_net(SFST::Transducer * t) {
  
    HfstFsm * net = new HfstFsm();
    // A map that maps nodes to integers
    SFST::NodeNumbering index(*t);
    // The set of nodes that have been visited
    std::set<SFST::Node*> visited_nodes;
   
    sfst_to_internal(t->root_node(), index, 
		     visited_nodes, 
		     net, t->alphabet);
    
    // Make sure that also symbols that occur in the alphabet of the
    // transducer t but not in its transitions are inserted to net
    SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it 
	   = cm.begin(); it != cm.end(); it++) 
      {
	if (it->first != 0) // The epsilon symbol "<>" is not inserted
	  {
	    net->alphabet->insert(std::string(it->second));
	  }
      }
    
    return net;
  }


  /* Create an SFST::Transducer equivalent to HfstFsm \a net. */
  SFST::Transducer * hfst_net_to_sfst(const HfstFsm * net) {

  SFST::Transducer * t = new SFST::Transducer();

  // Map that maps states of \a net to SFST nodes
  std::map<HfstState, SFST::Node*> state_map;
  state_map[0] = t->root_node();

  // Go through all states
  for (HfstFsm::iterator it = net->begin();
       it != net->end(); it++)
    {
      // Go through the set of transitions in each state
      for (HfstFsm::HfstTransitionSet::iterator tr_it = it->second.begin();
	   tr_it != it->second.end(); tr_it++)
	{
	  TransitionData data = tr_it->get_transition_data();

	  // Create new nodes, if needed
	  if (state_map.find(it->first) == state_map.end())
	    state_map[it->first] = t->new_node();

	  if (state_map.find(tr_it->get_target_state()) == state_map.end())
	    state_map[tr_it->get_target_state()] = t->new_node();

	  SFST::Label l
	    (t->alphabet.add_symbol(data.input_symbol.c_str()),
	     t->alphabet.add_symbol(data.output_symbol.c_str()));
	  
	  // Copy transition to node
	  state_map[it->first]->add_arc(l,
					state_map[tr_it->get_target_state()],
					t);
	}
    }

  // Go through the final states
  for (HfstFsm::FinalWeightMap::iterator it = net->final_weight_map.begin();
       it != net->final_weight_map.end(); it++) 
    {
      if (state_map.find(it->first) == state_map.end())
	state_map[it->first] = t->new_node();
      state_map[it->first]->set_final(1);
    }

  // Make sure that also symbols that occur in the alphabet of the
  // HfstFsm but not in its transitions are inserted to the SFST transducer
  for (HfstFsm::HfstNetAlphabet::iterator it = net->alphabet.begin();
       it != net->alphabet.end(); it++)
    t->alphabet.add_symbol(it->c_str());
  
  return t;
}
#endif // HAVE_SFST





#if HAVE_FOMA
// Both input and output symbol tables of internal format will contain
// all symbols in the sigma of the foma transducer
  HfstFsm * foma_to_hfst_net(struct fsm * t) {

  HfstFsm * net = new HfstFsm();
  struct fsm_state *fsm;
  fsm = t->states;
  int start_state_id = -1;
  bool start_state_found=false;

  // For every line in foma transducer:
  for (int i=0; (fsm+i)->state_no != -1; i++) {    

    // 1. If the source state is an initial state in foma:
    if ((fsm+i)->start_state == 1) {
      // If the start state has not yet been encountered.
      if (not start_state_found) {
	start_state_id = (fsm+i)->state_no;
	//	if (start_state_id != 0) {
	//  printf("ERROR: in foma transducer: start state is not numbered as zero\n");
	//  throw ErrorException();
	//}
	start_state_found=true;
      }
      // If the start state is encountered again, 
      else if ((fsm+i)->state_no == start_state_id) {
	// do nothing.
      }
      // If there are several initial states in foma transducer,
      else {
	// throw an exception.
	throw TransducerHasMoreThanOneStartStateException();
      }
    }

char *sigma_string(int number, struct sigma *sigma) {

    // 2. If there are transitions leaving from the state,
    if ((fsm+i)->target != -1) {
      net->add_transition((fsm+i)->state_no,
			  HfstArc
			  ((fsm+i)->target,
			   std::string (sigma_string((fsm+i)->in, t->sigma)), 
			   std::string (sigma_string((fsm+i)->out, t->sigma)),
			   0));      
    }
    
    // 3. If the source state is final in foma,
    if ((fsm+i)->final_state == 1) {
      net->set_final_weight((fsm+i)->state_no, 0);
    }

  }

  // If there was not an initial state in foma transducer,
  if (not start_state_found) {
    // throw an exception.
    throw TransducerHasNoStartStateException();
  }
  
  // If start state number (N) is not zero, swap state numbers N and zero 
  // in internal transducer. TODO
  if (start_state_id != 0)
    net->swap_state_numbers(start_state_id,0);

  struct sigma * p = t->sigma;
  while (p != NULL) {
    if (p->symbol == NULL)
      break;
    net->alphabet->insert(std::string(p->symbol));
    p = p->next;
  }

  return net;
}
#endif // HAVE_FOMA





#if HAVE_OPENFST
  HfstFsm * tropical_ofst_to_hfst_net
  (fst::StdVectorFst * t) {

  HfstFsm * net = new HfstFsm();

  // an empty transducer
  if (t->Start() == fst::kNoStateId)
    {      
      for ( fst::SymbolTableIterator it = 
	      fst::SymbolTableIterator(*(t->InputSymbols()));
	    not it.Done(); it.Next() ) {
	net->alphabet.insert( it.Symbol() );
      }    
      return net;
    }      

  // this takes care that initial state is always printed as number zero
  // and state number zero (if it is not initial) is printed as another number
  // (basically as the number of the initial state in that case, i.e.
  // the numbers of initial state and state number zero are swapped)
  StateId zero_print=0;
  StateId initial_state = t->Start();
  if (initial_state != 0) {
    zero_print = initial_state;
  }
  
  for (fst::StateIterator<fst::StdVectorFst> siter(*t); 
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
	for (fst::ArcIterator<fst::StdVectorFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const fst::StdArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;

	    net->add_transition(origin, 
				HfstArc
				(target,
				 t->InputSymbols()->Find(arc.ilabel),
				 t->InputSymbols()->Find(arc.olabel),
				 arc.weight.Value()
				 ));
	  }
	if (t->Final(s) != fst::TropicalWeight::Zero()) {
	  net->set_final_weight(origin, t->Final(s).Value());
	}
	break;
	}
      }

    for (fst::StateIterator<fst::StdVectorFst> siter(*t); 
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
	  for (fst::ArcIterator<fst::StdVectorFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const fst::StdArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;

	      net->add_transition(origin, 
				  HfstArc
				  (target,
				   t->InputSymbols()->Find(arc.ilabel),
				   t->InputSymbols()->Find(arc.olabel),
				   arc.weight.Value()
				   ));
	    }
	  if (t->Final(s) != fst::TropicalWeight::Zero())
	    net->set_final_weight(origin, t->Final(s).Value());
	}
      }

    for ( fst::SymbolTableIterator it = 
	    fst::SymbolTableIterator(*(t->InputSymbols()));
	  not it.Done(); it.Next() ) {
      net->alphabet->insert( it.Symbol() );
    }    

    return net;
}

HfstInternalTransducer * log_ofst_to_internal_hfst_format(LogFst * t) {

  HfstInternalTransducer * internal_transducer = new HfstInternalTransducer();

  // an empty transducer
  if (t->Start() == fst::kNoStateId)
    {      
      for ( fst::SymbolTableIterator it = fst::SymbolTableIterator(*(t->InputSymbols()));
	    not it.Done(); it.Next() ) {
	internal_transducer->alphabet->add_symbol( it.Symbol().c_str(), (unsigned int)it.Value() );
      }    
      return internal_transducer;
    }      

  // this takes care that initial state is always printed as number zero
  // and state number zero (if it is not initial) is printed as another number
  // (basically as the number of the initial state in that case, i.e.
  // the numbers of initial state and state number zero are swapped)
  StateId zero_print=0;
  StateId initial_state = t->Start();
  if (initial_state != 0) {
    zero_print = initial_state;
  }
  
  for (fst::StateIterator<LogFst> siter(*t); 
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
	for (fst::ArcIterator<LogFst> aiter(*t,s); 
	     !aiter.Done(); aiter.Next())
	  {
	    const fst::LogArc &arc = aiter.Value();
	    int target;  // how target state is printed, see the first comment
	    if (arc.nextstate == 0)
	      target = zero_print;
	    else if (arc.nextstate == initial_state)
	      target = 0;
	    else
	      target = (int)arc.nextstate;
	    internal_transducer->add_line(origin, target,
	      arc.ilabel, arc.olabel, arc.weight.Value() );
	  }
	if (t->Final(s) != fst::LogWeight::Zero())
	  internal_transducer->add_line(origin, t->Final(s).Value());
	break;
	}
      }

    for (fst::StateIterator<LogFst> siter(*t); 
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
	  for (fst::ArcIterator<LogFst> aiter(*t,s); 
	       !aiter.Done(); aiter.Next())
	    {
	      const fst::LogArc &arc = aiter.Value();
	      int target;  // how target state is printed, see the first comment
	      if (arc.nextstate == 0)
		target = zero_print;
	      else if (arc.nextstate == initial_state)
		target = 0;
	      else
		target = (int)arc.nextstate;
	      internal_transducer->add_line(origin, target,
		arc.ilabel, arc.olabel, arc.weight.Value());
	    }
	  if (t->Final(s) != fst::LogWeight::Zero())
	    internal_transducer->add_line(origin, t->Final(s).Value());
	}
      }

    for ( fst::SymbolTableIterator it = fst::SymbolTableIterator(*(t->InputSymbols()));
	  not it.Done(); it.Next() ) {
      internal_transducer->alphabet->add_symbol( it.Symbol().c_str(), (unsigned int)it.Value() );
    }    

    return internal_transducer;
}

#endif // HAVE_OPENFST

StateId hfst_ol_to_internal_add_state(hfst_ol::Transducer* t, 
                                      HfstInternalTransducer* internal_transducer,
                                      HfstOlToInternalStateMap& state_map,
                                      bool weighted,
                                      hfst_ol::TransitionTableIndex index,
				      unsigned int state_number)
{
  unsigned int new_state = state_number;
  state_map[index] = new_state;
  
  if(hfst_ol::indexes_transition_index_table(index))
  {
    const hfst_ol::TransitionIndex& transition_index = t->get_index(index);
    
    if(transition_index.final())
    {
      internal_transducer->add_line(new_state,
        weighted ? dynamic_cast<const hfst_ol::TransitionWIndex&>(transition_index).final_weight() : 0);
    }
  }
  else // indexes transition table
  {
    const hfst_ol::Transition& transition = t->get_transition(index);
    
    if(transition.final())
    {
      internal_transducer->add_line(new_state,
        weighted ? dynamic_cast<const hfst_ol::TransitionW&>(transition).get_weight() : 0);
    }
  }
  return new_state;
}

HfstInternalTransducer * hfst_ol_to_internal_hfst_format(hfst_ol::Transducer * t) 
{
  bool weighted = t->get_header().probe_flag(hfst_ol::Weighted);
  HfstInternalTransducer * internal_transducer = new HfstInternalTransducer();
  
  // This contains indices to either (1) the start of a set of entries in the
  // transition index table, or (2) the boundary before a set of entries in the
  // transition table; in this case, the following entries will all have the
  // same input symbol. In either case the index represents a state and may be final
  // The will already be an entry in state_map for each value in agenda
  std::vector<hfst_ol::TransitionTableIndex> agenda;
  HfstOlToInternalStateMap state_map;
  unsigned int state_number=0;
  
  hfst_ol_to_internal_add_state(t, internal_transducer, state_map, weighted, 0, state_number);
  agenda.push_back(0);
  while(!agenda.empty())
  {
    hfst_ol::TransitionTableIndex current_index = agenda.back();
    agenda.pop_back();
    
    unsigned int current_state = state_map[current_index];
    
    hfst_ol::TransitionTableIndexSet transitions = t->get_transitions_from_state(current_index);
    for(hfst_ol::TransitionTableIndexSet::const_iterator it=transitions.begin();it!=transitions.end();it++)
    {
      const hfst_ol::Transition& transition = t->get_transition(*it);
      
      if(state_map.find(transition.get_target()) == state_map.end())
      {
	state_number++;
        hfst_ol_to_internal_add_state(t, internal_transducer, state_map, weighted, transition.get_target(), state_number);
        agenda.push_back(transition.get_target());
      }
      internal_transducer->add_line(
        current_state, state_map[transition.get_target()],
	transition.get_input_symbol(), transition.get_output_symbol(),
	weighted ? dynamic_cast<const hfst_ol::TransitionW&>(transition).get_weight() : 0 );
    }
  }

  //transfer the symbol table
  HfstAlphabet * alpha = new HfstAlphabet();
  const hfst_ol::SymbolTable& symbols = t->get_alphabet().get_symbol_table();
  for(size_t i=0; i<symbols.size(); i++)
    alpha->add_symbol(symbols[i].c_str(), i);
  internal_transducer->alphabet = alpha;
  
  return internal_transducer;
}


#if HAVE_FOMA
// SymbolTable is converted to sigma, but the string-to-number
// relations are changed...
struct fsm * hfst_net_to_foma(const HfstFsm * hfst_fsm) {

  struct fsm_construct_handle *h;
  struct fsm *net;
  h = fsm_construct_init(strdup(std::string("").c_str()));

  // Go through all states
  for (HfstFsm::iterator it = hfst_fsm->begin();
       it != hfst_fsm->end(); it++)
    {
      // Go through the set of transitions in each state
      for (HfstFsm::HfstTransitionSet::iterator tr_it = it->second.begin();
	   tr_it != it->second.end(); tr_it++)
	{
	  TransitionData data = tr_it->get_transition_data();
	  
	  fsm_construct_add_arc(h, 
				(int)it->first, 
				(int)tr_it->get_target_state(),
				data.input_symbol.c_str(),
				data.output_symbol.c_str());
	}
    }

  // Go through the final states
  for (HfstFsm::FinalWeightMap::iterator it = hfst_fsm->final_weight_map.begin();
       it != hfst_fsm->final_weight_map.end(); it++) 
    {
      fsm_construct_set_final(h, (int)it->first);
    }

  // Add symbols that occur in HfstFsm's alphabet but not in
  // any of its transition to foma's alphabet
  for (HfstFsm::HfstNetAlphabet::iterator it = hfst_fsm->alphabet.begin();
       it != hfst_fsm->alphabet.end(); it++)
    {
      char *symbol = strdup(it->c_str());
      if ( fsm_construct_check_symbol(h,symbol) == -1 ) // not found
	fsm_construct_add_symbol(h,symbol);
      // free symbol?
    }

  fsm_construct_set_initial(h, 0);
  // not clear what happens if start state is not number zero...
  net = fsm_construct_done(h);
  fsm_count(net);
  net = fsm_topsort(net);

  return net;      
}
#endif // HAVE_FOMA


#if HAVE_OPENFST

 fst::StateId hfst_state_to_state_id
   (HfstState s, std::map<HfstState, fst::StateId> &state_map, fst::StdVectorFst * t)
 {
   std::map<HfstState, fst::StateId>::iterator it = state_map.find(s);
   if (it == state_map.end())
     {
       fst::StateId retval t->AddState();
       state_map[s] = retval;
       return retval;
     }
   return *it;
 }

fst::StdVectorFst * hfst_net_to_tropical_ofst
  (const HfstFsm * net) {

  fst::StdVectorFst * t = new fst::StdVectorFst();
  StateId start_state = t->AddState();
  t->SetStart(start_state);

  std::map<HfstState, fst::StateId> state_map;
  state_map[0] = start_state;

  fst::SymbolTable st("");
  st.AddSymbol("@_EPSILON_SYMBOL_@", 0);
  st.AddSymbol("@_UNKNOWN_SYMBOL_@", 1);
  st.AddSymbol("@_IDENTITY_SYMBOL_@", 2);

  // Go through all states
  for (HfstFsm::iterator it = net->begin();
       it != net->end(); it++)
    {
      // Go through the set of transitions in each state
      for (HfstFsm::HfstTransitionSet::iterator tr_it = it->second.begin();
	   tr_it != it->second.end(); tr_it++)
	{
	  TransitionData data = tr_it->get_transition_data();
	  
	  t->AddArc( hfst_state_to_state_id(it->first, state_map, t), 
		     fst::StdArc
		      ( st.AddSymbol(data.input_symbol),
			st.AddSymbol(data.output_symbol),
			data.weight,
			hfst_state_to_state_id
 			 (tr_it->get_target_state(), state_map, t)) );
	}
    }

  // Go through the final states
  for (HfstFsm::FinalWeightMap::iterator it = net->final_weight_map.begin();
       it != net->final_weight_map.end(); it++) 
    {
      t->SetFinal(it->first, it->second);
    }

  // Add symbols that do not occur in transitions
  for (HfstFsm::HfstNetAlphabet::iterator it = net->alphabet.begin();
       it != net->alphabet.end(); it++)
    {
      st.AddSymbol(*it);
    }

  t->SetInputSymbols(&st);
  return t;  
}

LogFst * hfst_internal_format_to_log_ofst(const HfstInternalTransducer * internal_transducer) {
  
  LogFst * t = new LogFst();
  StateId start_state = t->AddState();
  t->SetStart(start_state);

  if (internal_transducer->has_no_lines()) {
    fst::SymbolTable *st = new fst::SymbolTable("anonym_hfst3_symbol_table");
    HfstAlphabet::CharMap cm = internal_transducer->alphabet->get_char_map();
    for (HfstAlphabet::CharMap::const_iterator it = cm.begin(); it != cm.end(); it++)
      st->AddSymbol(std::string(it->second), (int64)it->first);    
    t->SetInputSymbols(st);
    delete st;
    return t;
  }

  std::map<unsigned int,unsigned int> state_map;
  state_map[0] = start_state;

  const std::set<InternalTransducerLine> *lines = internal_transducer->get_lines();
  for (std::set<InternalTransducerLine>::const_iterator it = lines->begin();
       it != lines->end(); it++)
    {
      if (it->final_line) 
	{
	  if (state_map.find(it->origin) == state_map.end())
	    state_map[it->origin] = t->AddState();	    
	  t->SetFinal(state_map[it->origin],it->weight);
	}
      else 
	{
	  if (state_map.find(it->origin) == state_map.end())
	    state_map[it->origin] = t->AddState();
	  if (state_map.find(it->target) == state_map.end())
	    state_map[it->target] = t->AddState();
	  t->AddArc(state_map[it->origin],
		    fst::LogArc(it->isymbol, it->osymbol,
				it->weight, state_map[it->target]));
	}
    }

  fst::SymbolTable *st = new fst::SymbolTable("anonym_hfst3_symbol_table");
  HfstAlphabet::CharMap cm = internal_transducer->alphabet->get_char_map();
  for (HfstAlphabet::CharMap::const_iterator it = cm.begin(); it != cm.end(); it++)
    st->AddSymbol(std::string(it->second), (int64)it->first);    
  t->SetInputSymbols(st);
  delete st;

  return t;  
}

hfst_ol::Transducer * hfst_internal_format_to_hfst_ol(HfstInternalTransducer * , bool ) {
  throw hfst::exceptions::FunctionNotImplementedException();
}
#endif // HAVE_OPENFST


#if HAVE_MFSTL

HfstInternalTransducer * mfstl_to_internal_hfst_format(mfstl::MyFst * t) {
  (void)t;
  throw hfst::exceptions::FunctionNotImplementedException();
}

mfstl::MyFst * hfst_internal_format_to_mfstl(const HfstInternalTransducer * t) {
  (void)t;
  throw hfst::exceptions::FunctionNotImplementedException();
}

#endif // HAVE_MFSTL


} }
#else
#include <cstdlib>
#include <cassert>

using namespace hfst;
using namespace hfst::implementations;

int main(void)
  {
    std::cout << "Unit tests for " __FILE__ ":";
    HfstInternalTransducer internal;
    internal.add_line(0, 1, 1, 1, 1);
    internal.add_line(1, 2, 2, 2, 2);
    internal.add_line(2, 3, 3, 3, 3);
    internal.add_line(3, 4);
    std::cout << std::endl << "Conversions: ";
#if HAVE_SFST
    std::cout << " internal->sfst...";
    SFST::Transducer* sfst = hfst_internal_format_to_sfst(&internal);
    std::cout << " sfst->internal...";
    assert(sfst_to_internal_hfst_format(sfst) != 0);
#endif
#if HAVE_OFST
    std::cout << " internal->ofst...";
    fst::StdVectorFst* ofst = hfst_internal_format_to_ofst(&internal);
    std::cout << " ofst->internal...";
    assert(ofst_to_internal_hfst_format(ofst) != 0);
#endif
#if HAVE_FOMA
    std::cout << " internal->foma...";
    struct fsm* foma = hfst_internal_format_to_foma(&internal);
    std::cout << " foma->internal...";
    assert(foma_to_internal_hfst_format(foma) != 0);
#endif
    std::cout << "ok!" << std::endl;
    return EXIT_SUCCESS;
  }
#endif
