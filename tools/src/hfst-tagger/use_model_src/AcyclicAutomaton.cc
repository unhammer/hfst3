#include "AcyclicAutomaton.h"
#include "SequenceModelComponent.h"

#define FINAL_SYMBOL -1

#include <iostream>

float infinity = std::numeric_limits<float>::infinity();

AcyclicState::AcyclicState(size_t id):
  id(id),
  source_id(-1),
  symbol(-1),
  weight(std::numeric_limits<float>::infinity())
{}

void AcyclicState::add_transition(size_t source_id,Symbol symbol,Weight weight)
{
  if (this->source_id == static_cast<size_t>(-1) or weight < this->weight)
    {
      this->source_id = source_id;
      this->symbol = symbol;
      this->weight = weight;
    }
}

Symbol AcyclicState::get_symbol(void) const
{ return symbol; }

Weight AcyclicState::get_weight(void) const
{ return weight; }

void AcyclicState::set_weight(Weight w)
{ weight = w; }

size_t AcyclicState::get_source(void) const
{ return source_id; }

bool AcyclicState::is_initialized(void)
{ return source_id != static_cast<size_t>(-1); }

void AcyclicState::clear(void)
{ source_id = -1; }

AcyclicAutomaton::AcyclicAutomaton(void)
{ initialize(); }

void AcyclicAutomaton::initialize(void)
{
  current_position = 0;
  current_id       = 0;

  for (size_t i = 0; i < WORD_STATE_RESERVED_LENGTH; ++i)
    { default_position.push_back(AcyclicState(i)); }

  data.insert(data.end(),SENTENCE_RESERVED_LENGTH,default_position);

  // Add the start state.
  static_cast<void>(add_state());
  data[0][0].set_weight(0.0);

  // All future added states belong to position 1+.
  //finalize_position();
}

void AcyclicAutomaton::add_transition
(State source,Symbol symbol,Weight weight,State target)
{
  size_t source_id = state_to_id_map[source];
  size_t target_id = state_to_id_map[target];

  /*  std::cerr << "Position: " << current_position
    << "State: " << source << " id: " << source_id << " Target: " << target
    << " id: " << target_id << " Symbol: " 
    << SequenceModelComponent::get_string_symbol(symbol) << " Weight: " 
    << weight << " + " 
    << data[current_position - 1][source_id].get_weight() << std::endl;
  */
  
  data[current_position][target_id].add_transition
    (source_id,
     symbol,
     data[current_position - 1][source_id].get_weight() + weight);
}

State AcyclicAutomaton::add_state(void)
{
  //if (data[current_position].size() >= 1000)
  //  { return -1; }

  state_to_id_map.push_back(current_id);
  ++current_id;

  if (data[current_position].size() == current_id)
    { data[current_position].push_back(AcyclicState(current_id)); }

  return state_to_id_map.size() - 1;
}

void AcyclicAutomaton::finalize_position(void)
{
  ++current_position;
  if (data.size() == current_position)
    { data.push_back(default_position); }

  current_id = 0;
}

void AcyclicAutomaton::set_final_weight(State s, Weight weight)
{
  if (weight != infinity)
    {
      //State target = add_state();
      //add_transition(s,FINAL_SYMBOL,weight,target);
      size_t source_id = state_to_id_map[s];
      size_t target_id = 0;

      data[current_position + 1][target_id].add_transition
	(source_id,
	 FINAL_SYMBOL,
	 data[current_position][source_id].get_weight() + weight);
    }
}


void AcyclicAutomaton::clear(void)
{
  for (Data::iterator it = data.begin(); it != data.end(); ++it)
    {
      for (AcyclicStateVector::iterator jt = it->begin();
	   jt != it->end();
	   ++jt)
	{ 
	  if (not jt->is_initialized())
	    { break; }
	  jt->clear(); 
	}
    }
  
  current_position = 0;
  current_id = 0;

  static_cast<void>(add_state());
  data[0][0].set_weight(0.0);
  //finalize_position();

  state_to_id_map.clear();
}


WeightedSymbolVector AcyclicAutomaton::get_best_path(void)
{
  finalize_position();

  //Weight minimum = std::numeric_limits<float>::infinity();
  //size_t id = -1;

  return WeightedSymbolVector(data[current_position].at(0).get_weight(),get_path(0));
  /*
  for (size_t i = 0; i < data[current_position].size(); ++i)
    {
      const AcyclicState &state = data[current_position].at(i);

      if (state.get_weight() < minimum)
	{
	  minimum = state.get_weight();
	  id = i;
	}
    }
  */

}

SymbolVector AcyclicAutomaton::get_path(size_t id) const
{
  SymbolVector result(current_position);
  for (size_t i = current_position; i > 0; --i)
    {
      const AcyclicState &state = data[i][id];
      result.at(i - 1) = state.get_symbol();
      id = state.get_source();
    }
  return result;
}

#ifdef TEST_AcyclicAutomaton
#include <iostream>

int main(void)
{
  AcyclicAutomaton acyclic_automaton1;
  State s0 = 0;

  State s1 = acyclic_automaton1.add_state();
  State s1_5 = acyclic_automaton1.add_state();
  acyclic_automaton1.add_transition(s0,3,0.125,s1);
  acyclic_automaton1.add_transition(s0,1,0.5,s1);
  acyclic_automaton1.add_transition(s0,2,0.25,s1);
  acyclic_automaton1.add_transition(s0,2,0.1,s1_5);

  acyclic_automaton1.finalize_position();

  State s2 = acyclic_automaton1.add_state();
  State s3 = acyclic_automaton1.add_state();
  acyclic_automaton1.add_transition(s1,1,0.5,s2);
  acyclic_automaton1.add_transition(s1_5,2,0.425,s3);

  acyclic_automaton1.set_final_weight(s2,2.0);
  acyclic_automaton1.set_final_weight(s3,0.3);


  WeightedSymbolVector weight_2_0_path_1_1 = 
    acyclic_automaton1.get_best_path();

  std::cerr << weight_2_0_path_1_1.first << std::endl;
  for (SymbolVector::const_iterator it = weight_2_0_path_1_1.second.begin();
       it != weight_2_0_path_1_1.second.end();
       ++it)
    { std::cerr << "symbol: " << *it << std::endl; }
}
#endif // TEST_AcyclicAutomaton
