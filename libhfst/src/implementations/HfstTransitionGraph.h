 #ifndef _HFST_TRANSITION_GRAPH_H_
 #define _HFST_TRANSITION_GRAPH_H_

 /** @file HfstTransitionGraph.h
     @brief Class HfstTransitionGraph */

 #include <cstdio>
 #include <string>
 #include <set>
 #include <cassert>
 #include <iostream>
 #include <algorithm>
 #include <stack>

 #include "../HfstSymbolDefs.h"
 #include "../HfstExceptionDefs.h"
 #include "../HfstDataTypes.h"
 #include "../HarmonizeUnknownAndIdentitySymbols.h"
 #include "../HfstFlagDiacritics.h"
 #include "../HfstEpsilonHandler.h"
 #include "ConvertTransducerFormat.h"
 #include "HfstTransition.h"
 #include "HfstTropicalTransducerTransitionData.h"
 #include "HfstFastTransitionData.h"

 namespace hfst {

   class HfstFile {
   private:
     FILE * file;
   public:
     HfstFile();
     ~HfstFile();
     void set_file(FILE * f);
     FILE * get_file();
     void close();
     void write(const char * str);
   };


   /** @brief A namespace for all code that forms a bridge between
       backend libraries and HFST.

       Most code in this namespace is not intended to be accessed
       by the user of the HFST interface. An exception is HFST's
       own transducer class hfst::HfstTransitionGraph and classes that
       are needed to use it.
   */
   namespace implementations {

     /** @brief The number of a state in an HfstTransitionGraph. */
     typedef unsigned int HfstState;

     /** @brief A simple transition graph format that consists of
         states and transitions between those states.

     Probably the easiest way to use this template is to choose
     the implementations #HfstBasicTransducer
     (HfstTransitionGraph<HfstTropicalTransducerTransitionData>)
     and #HfstBasicTransition
     (HfstTransition<HfstTropicalTransducerTransitionData>).
     The class HfstTropicalTransducerTransitionData contains an input string,
     an output string and a float weight. HfstBasicTransducer is the 
     implementation that is used as an example in this documentation.

     An example of creating a HfstBasicTransducer [foo:bar baz:baz] 
     with weight 0.4 from scratch:

 \verbatim
   // Create an empty transducer
   // The transducer has initially one start state (number zero) 
   // that is not final
   HfstBasicTransducer fsm;
   // Add two states to the transducer
   fsm.add_state(1);
   fsm.add_state(2);
   // Create a transition [foo:bar] leading to state 1 with weight 0.1 ...
   HfstBasicTransition tr(1, "foo", "bar", 0.1);
   // ... and add it to state zero
   fsm.add_transition(0, tr);
   // Add a transition [baz:baz] with weight 0 from state 1 to state 2 
   fsm.add_transition(1, HfstBasicTransition(2, "baz", "baz", 0.0));
   // Set state 2 as final with weight 0.3
   fsm.set_final_weight(2, 0.3);
 \endverbatim

        An example of iterating through a HfstBasicTransducer's states
        and transitions when printing it in AT&T format to stderr:

 \verbatim
   // The first state is always number zero.
   unsigned int source_state=0;

   // Go through all states
     for (HfstBasicTransducer::const_iterator it = fsm.begin();
      it != fsm.end(); it++ )
       {
         // Go through all transitions
     for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it 
            = it->begin(); tr_it != it->end(); tr_it++)
       {
         std::cerr << source_state << "\t"
               << tr_it->get_target_state() << "\t"
               << tr_it->get_input_symbol() << "\t"
               << tr_it->get_output_symbol() << "\t"
               << tr_it->get_weight() << std::endl;
       }

     if (fsm.is_final_state(source_state)) 
       {
         std::cerr << source_state << "\t"
               << fsm.get_final_weight(source_state) << std::endl;
       }

     // the next state is numbered source_state + 1  
     source_state++;
       }
 \endverbatim

 @see #HfstBasicTransducer HfstBasicTransition */
     template <class C> class HfstTransitionGraph 
       {

     // --- Datatypes and variables ---

       public:
         /** @brief Datatype for the states of a transition in a graph. */
         typedef std::vector<HfstTransition<C> > HfstTransitions;

     /** @brief Datatype for a symbol in a transition. */
     typedef typename C::SymbolType HfstSymbol;
     /** @brief Datatype for a symbol pair in a transition. */
     typedef std::pair<HfstSymbol, HfstSymbol> 
       HfstSymbolPair; 
     /** @brief A set of symbol pairs. */
     typedef std::set<HfstSymbolPair> HfstSymbolPairSet;
     /** @brief A set of symbol pairs. */
     typedef std::set<HfstSymbol> HfstSymbolSet;
     /** @brief A vector of symbol pairs. */
     typedef std::vector<HfstSymbolPair> HfstSymbolPairVector;
     /** @brief Datatype for the alphabet of a graph. */
         typedef std::set<HfstSymbol> HfstTransitionGraphAlphabet;

       protected:
     /* Datatype for the states of a graph and their transitions.
        Each index of the vector is a state and the transitions 
        on that index are the transitions of that state. */
         typedef std::vector<HfstTransitions> HfstStates;
     /* States of the graph and their transitions. */
         HfstStates state_vector;

     /* The initial state number. */
         static const HfstState INITIAL_STATE = 0;

     /* Datatype for the final states and their weights in a graph. */
         typedef std::map<HfstState,typename C::WeightType> FinalWeightMap;
     /* The final states and their weights in the graph. */
         FinalWeightMap final_weight_map;

     /* The alphabet of the graph. */
         HfstTransitionGraphAlphabet alphabet;

         /* Used by substitute function. */
         typedef unsigned int HfstNumber;
         typedef std::vector<HfstNumber> HfstNumberVector;
         typedef std::pair<HfstNumber, HfstNumber> HfstNumberPair;
         typedef std::map<HfstNumberPair, HfstNumberPair> HfstNumberPairSubstitutions;

       protected:
         /* @brief An iterator type that points to a state in a graph. 

            The value pointed by the iterator is of type HfstTransitions. */
         typedef typename HfstStates::iterator iterator;

       public:
         /** @brief A const iterator type that points a state in a graph.

             The value pointed by the iterator is of type HfstTransitions. */
         typedef typename HfstStates::const_iterator const_iterator;

         /** @brief The name of the graph. */
         std::string name;

         /** @brief The states of the graph. */
         std::vector<HfstState> states() const {
           std::vector<HfstState> retval(this->get_max_state()+1, 0);
           for (unsigned int i=0; i<(this->get_max_state()+1); i++)
             retval[i] = i;
           return retval;
         }

     // --------------------------------------------------------
     // --- Construction, assignment, copying and conversion ---
     // --------------------------------------------------------

         /** @brief Create a graph with one initial state that has state number
             zero and is not a final state, i.e. create an empty graph. */
       HfstTransitionGraph(void) {
           initialize_alphabet(alphabet);
           HfstTransitions tr;
           state_vector.push_back(tr);
         }

       HfstTransitionGraph(FILE *file) {
         initialize_alphabet(alphabet);
         HfstTransitions tr;
         state_vector.push_back(tr);
         unsigned int linecount=0;
         this->assign(read_in_att_format(file, "@0@", linecount));
       }

       HfstTransitionGraph(HfstFile &file) {
         initialize_alphabet(alphabet);
         HfstTransitions tr;
         state_vector.push_back(tr);
         unsigned int linecount=0;
         this->assign(read_in_att_format(file.get_file(), "@0@", linecount));
       }


     /** @brief The assignment operator. */
     HfstTransitionGraph &operator=(const HfstTransitionGraph &graph)
       {
         if (this == &graph)
           return *this;
         state_vector = graph.state_vector;
         final_weight_map = graph.final_weight_map;
         alphabet = graph.alphabet;
         assert(alphabet.count(HfstSymbol()) == 0);
         return *this;
       }

     HfstTransitionGraph &assign(const HfstTransitionGraph &graph)
       {
         return this->operator=(graph);
       }

     /** @brief Create a deep copy of HfstTransitionGraph \a graph. */
     HfstTransitionGraph(const HfstTransitionGraph &graph) {
       state_vector = graph.state_vector;
       final_weight_map = graph.final_weight_map;
       alphabet = graph.alphabet;
       assert(alphabet.count(HfstSymbol()) == 0);
     }

     /** @brief Create an HfstTransitionGraph equivalent to HfstTransducer 
         \a transducer. FIXME: move to a separate file */
       HfstTransitionGraph(const hfst::HfstTransducer &transducer) {
       HfstTransitionGraph<HfstTropicalTransducerTransitionData>
         *fsm = ConversionFunctions::
         hfst_transducer_to_hfst_basic_transducer(transducer);
       state_vector = fsm->state_vector;
       final_weight_map = fsm->final_weight_map;
       alphabet = fsm->alphabet;
       delete fsm;
     }


     // --------------------------------------------------
     // --- Initialization, optimization and debugging ---
     // --------------------------------------------------

       protected:
     /* Add epsilon, unknown and identity symbols to the alphabet 
        \a alpha. */
     void initialize_alphabet(HfstTransitionGraphAlphabet &alpha) {
       alpha.insert(C::get_epsilon());
       alpha.insert(C::get_unknown());
       alpha.insert(C::get_identity());
     }

     /* Check that all symbols that occur in the transitions of the graph
        are also in the alphabet. */
     bool check_alphabet() 
     {
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   if(alphabet.find(data.get_input_symbol()) 
              == alphabet.end()) {
             return false;
           }
                   if(alphabet.find(data.get_output_symbol()) 
              == alphabet.end()) {
             return false;
           }
                 }
             }
       return true;
     }

       public:
     /* Print the alphabet of the graph to standard error stream. */
     void print_alphabet() const 
     {
       for (typename HfstTransitionGraphAlphabet::const_iterator it 
          = alphabet.begin(); it != alphabet.end(); it++)
         {
           if (it != alphabet.begin())
         std::cerr << ", ";
           std::cerr << *it;
         }
       std::cerr << std::endl;
     }

       protected:
     /* Get the number of the \a symbol. */
     unsigned int get_symbol_number
       (const HfstSymbol &symbol) const {
       return C::get_number(symbol);
     }

     /* For internal optimization: Reserve space for 
        \a number_of_states states. */
     void initialize_state_vector
       (unsigned int number_of_states)
     {
       state_vector.reserve(number_of_states);
     }

     /* For internal optimization: Reserve space for
        \a number_of_transitions transitions for state number 
        \a state_number. */
     void initialize_transition_vector
       (unsigned int state_number, unsigned int number_of_transitions)
     {
       add_state(state_number);
       state_vector[state_number].reserve(number_of_transitions);
     }


     // -----------------------------------
     // ---------- The alphabet -----------
     // -----------------------------------

       public:
         /** @brief Explicitly add \a symbol to the alphabet of the graph.

             @note Usually the user does not have to take care of the alphabet
             of a graph. This function can be useful in some special cases. */
         void add_symbol_to_alphabet(const HfstSymbol &symbol) {
           alphabet.insert(symbol);
         }

     /** @brief Remove symbol \a symbol from the alphabet of the graph. 

         @note Use with care, removing symbols that occur in the transitions
         of the graph can have unexpected results. */
     void remove_symbol_from_alphabet(const HfstSymbol &symbol) {
       alphabet.erase(symbol);
     }

     void remove_symbols_from_alphabet(const HfstSymbolSet &symbols) {
       for (typename HfstSymbolSet::const_iterator it = symbols.begin();
            it != symbols.end(); it++)
         {
           alphabet.erase(*it);
         }
     }

     /** @brief Same as #add_symbol_to_alphabet for each symbol in
         \a symbols. */
     void add_symbols_to_alphabet(const HfstSymbolSet &symbols)
     {
       for (typename HfstSymbolSet::const_iterator it = symbols.begin();
            it != symbols.end(); it++)
         {
           alphabet.insert(*it);
         }
     }

     void add_symbols_to_alphabet(const HfstSymbolPairSet &symbols)
     {
       for (typename HfstSymbolPairSet::const_iterator it = symbols.begin();
            it != symbols.end(); it++)
         {
           alphabet.insert(it->first);
           alphabet.insert(it->second);
         }
     }

     /* Remove all symbols that are given in \a symbols but do not occur 
        in transitions of the graph from its alphabet. */
     void prune_alphabet_after_substitution(const std::set<unsigned int> &symbols)
     {
       if (symbols.size() == 0)
         return;

       std::vector<bool> symbols_found;
       symbols_found.resize
         (C::get_max_number()+1, false);

       // Go through all transitions
       for (iterator it = begin(); it != end(); it++)
         {
           for (typename HfstTransitions::iterator tr_it
                  = it->begin();
                tr_it != it->end(); tr_it++)
             {
               const C & data = tr_it->get_transition_data();
               symbols_found.at(data.get_input_number()) = true;
               symbols_found.at(data.get_output_number()) = true;
             }
         }

       // Remove symbols in \a symbols from the alphabet if they did not
       // occur in any transitions
       for (std::set<unsigned int>::const_iterator it = symbols.begin();
            it != symbols.end(); it++)
         {
           if (! symbols_found.at(*it))
             alphabet.erase(C::get_symbol(*it));
         }

     }

     HfstTransitionGraphAlphabet symbols_used()
     {
       HfstTransitionGraphAlphabet retval;
       for (iterator it = begin(); it != end(); it++)
         {
           for (typename HfstTransitions::iterator tr_it
                  = it->begin();
                tr_it != it->end(); tr_it++)
             {
               C data = tr_it->get_transition_data();
               
               retval.insert(data.get_input_symbol());
               retval.insert(data.get_output_symbol());
             }
         }
       return retval;
     }

         /** @brief Remove all symbols that do not occur in transitions of
             the graph from its alphabet. 

             @param force Whether unused symbols are removed even if
             unknown or identity symbols occur in transitions.

             Epsilon, unknown and identity \link hfst::String symbols\endlink
             are always included in the alphabet. */
         void prune_alphabet(bool force=true) {

           // Which symbols occur in the graph
           HfstTransitionGraphAlphabet symbols_found = symbols_used();

           // Whether unknown or identity symbols are used
           bool unknowns_or_identities_used = 
             ( (symbols_found.find("@_UNKNOWN_SYMBOL_@") != symbols_found.end()) ||
               (symbols_found.find("@_IDENTITY_SYMBOL_@") != symbols_found.end()) );

           // We cannot prune the transducer because unknowns or identities
           // are used in its transitions.
           if (!force && unknowns_or_identities_used)
             return;

           // Special symbols are always known
           symbols_found.insert("@_EPSILON_SYMBOL_@"); 
           symbols_found.insert("@_UNKNOWN_SYMBOL_@"); 
           symbols_found.insert("@_IDENTITY_SYMBOL_@"); 

           // Which symbols in the graph's alphabet did not occur in 
           // the graph
           HfstTransitionGraphAlphabet symbols_not_found;

           for (typename HfstTransitionGraphAlphabet::iterator it 
                  = alphabet.begin();
                it != alphabet.end(); it++) 
             {
               if (symbols_found.find(*it) == symbols_found.end())
                 symbols_not_found.insert(*it);
             }

           // Remove the symbols that did not occur in the graph
           // from its alphabet
           for (typename HfstTransitionGraphAlphabet::iterator it 
                  = symbols_not_found.begin();
                it != symbols_not_found.end(); it++)
             {
               alphabet.erase(*it);
             }
         }

         /** @brief Get the set of HfstSymbols in the alphabet 
             of the graph. 

             The HfstSymbols do not necessarily occur in any transitions
             of the graph. Epsilon, unknown and identity \link 
             hfst::String symbols\endlink are always included in the alphabet. */
         const HfstTransitionGraphAlphabet &get_alphabet() const {
           return alphabet;
         }



     // ----------------------------------------------------------------
     // --- Adding states and transitions and iterating through them ---
     // ----------------------------------------------------------------

         /** @brief Add a new state to this graph and return its number.

             @return The next (smallest) free state number. */
         HfstState add_state(void) {
       HfstTransitions tr;
       state_vector.push_back(tr);
       return state_vector.size()-1;
     }

         /** @brief Add a state \a s to this graph.

             If the state already exists, it is not added again.
         All states with state number smaller than \a s are also
         added to the graph if they did not exist before.
             @return \a s*/
         HfstState add_state(HfstState s) {
       while(state_vector.size() <= s) {
         HfstTransitions tr;
         state_vector.push_back(tr);
       }
           return s;
         }

     /** @brief Get the biggest state number in use. */
     HfstState get_max_state() const {
       return state_vector.size()-1;
     }

         /** @brief Add a transition \a transition to state \a s. 

             If state \a s does not exist, it is created. */
     void add_transition(HfstState s, const HfstTransition<C> & transition,
                         bool add_symbols_to_alphabet=true) {

           C data = transition.get_transition_data();

           add_state(s);
           add_state(transition.get_target_state());
           if (add_symbols_to_alphabet) {
             alphabet.insert(data.get_input_symbol());
             alphabet.insert(data.get_output_symbol());
           }
           state_vector[s].push_back(transition);
     }

     /** @brief Remove transition \a transition from state \a s.
                \a remove_symbols_from_alphabet defines whether
                symbols in \a transition are removed from the alphabet
                if they are no longer used in the graph.

         If \a state or \a transition does not exist, nothing is done. */
     void remove_transition(HfstState s, const HfstTransition<C> & transition,
                            bool remove_symbols_from_alphabet=false)
     {
       if (! (state_vector.size() > s))
         {
           return;
         }

       HfstTransitions & transitions = state_vector[s];
       // iterators to transitions to be removed
       // transitions must be removed in reverse order so that iterators
       // are not invalidated
       std::stack<typename HfstTransitions::iterator> elements_to_remove;

       // find the transitions to be removed
       for (typename HfstTransitions::iterator it = transitions.begin();
            it != transitions.end(); it++)
         {
           // weight is ignored
           if (it->get_input_symbol() == transition.get_input_symbol() &&
               it->get_output_symbol() == transition.get_output_symbol() &&
               it->get_target_state() == transition.get_target_state())
             {
               // schedule transition to be removed
               elements_to_remove.push(it); 
             }
         }
       // remove the transitions in reverse order
       while (!elements_to_remove.empty())
         {
           state_vector[s].erase(elements_to_remove.top());
           elements_to_remove.pop();
         }           
       
       if (remove_symbols_from_alphabet)
         {
           HfstTransitionGraphAlphabet alpha = this->symbols_used();
           if (alpha.find(transition.get_input_symbol()) == alpha.end())
             this->remove_symbol_from_alphabet(transition.get_input_symbol());
           if (alpha.find(transition.get_output_symbol()) == alpha.end())
             this->remove_symbol_from_alphabet(transition.get_output_symbol());
         }
     }

         /** @brief Whether state \a s is final. 
         FIXME: return positive infinity instead if not final. */
         bool is_final_state(HfstState s) const {
           return (final_weight_map.find(s) != final_weight_map.end());
         }

         /** Get the final weight of state \a s in this graph. */
         typename C::WeightType get_final_weight(HfstState s) const {
           if (final_weight_map.find(s) != final_weight_map.end())
             return final_weight_map.find(s)->second;
           HFST_THROW(StateIsNotFinalException);
         }

         /** @brief Set the final weight of state \a s in this graph 
             to \a weight. 

             If the state does not exist, it is created. */
         void set_final_weight(HfstState s, 
                   const typename C::WeightType & weight) {
       add_state(s);
           final_weight_map[s] = weight;
         }

         /** @brief Sort the arcs of this transducer according to input and
             output symbols. */
         HfstTransitionGraph &sort_arcs(void)
       {
         for (typename HfstStates::iterator it = state_vector.begin();
          it != state_vector.end();
          ++it)
           {
         HfstTransitions &transitions = *it;
         std::sort<typename HfstTransitions::iterator>
           (transitions.begin(),transitions.end());
           }
         return *this;
       }

         /** @brief Get an iterator to the beginning of the states in 
             the graph. 

             For an example, see #HfstTransitionGraph */
         iterator begin() { return state_vector.begin(); }

         /** @brief Get a const iterator to the beginning of 
             states in the graph. */
         const_iterator begin() const { return state_vector.begin(); }

         /** @brief Get an iterator to the end of states (last state + 1) 
         in the graph. */
         iterator end() { return state_vector.end(); }

         /** @brief Get a const iterator to the end of states (last state + 1)
         in the graph. */
         const_iterator end() const { return state_vector.end(); }


         /** @brief Get the set of transitions of state \a s in this graph. 

             If the state does not exist, a @a StateIndexOutOfBoundsException
             is thrown.
         */
         const HfstTransitions & operator[](HfstState s) const
         {
           if (s >= state_vector.size()) { 
         HFST_THROW(StateIndexOutOfBoundsException); }
           return state_vector[s];
         }        

         /** @brief Alternative name for operator[].

             Python interface uses this function as '[]' is not a legal name.

             @see operator[]
          */
         const HfstTransitions & transitions(HfstState s) const
         {
           return this->operator[](s);
         }

     // --------------------------------------------------
     // -----   Reading and writing in AT&T format   -----
     // --------------------------------------------------

       protected:
         /* Change state numbers s1 to s2 and vice versa. */
         void swap_state_numbers(HfstState s1, HfstState s2) {

           HfstTransitions s1_copy = state_vector[s1];
           state_vector[s1] = state_vector[s2];
           state_vector[s2] = s1_copy;

           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // Go through all transitions
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   HfstState new_target=tr_it.get_target_state();
                   if (tr_it.get_target_state() == s1)
                     new_target = s2;
                   if (tr_it.get_target_state() == s2)
                     new_target = s1;

                   if (new_target != tr_it.get_target_state())
                     {
                       HfstTransition<C> tr
                         (new_target,
                          tr_it.get_input_symbol(),
                          tr_it.get_output_symbol(),
                          tr_it.get_weight());

                       it->operator[](i) = tr;
                     }

                 } // all transitions gone through

             } // ----- all states gone through -----

           // Swap final states, if needed
           typename FinalWeightMap::iterator s1_it = final_weight_map.find(s1);
           typename FinalWeightMap::iterator s2_it = final_weight_map.find(s2);
           typename FinalWeightMap::iterator end_it = final_weight_map.end();

           if (s1_it != end_it && s2_it != end_it) {
             typename C::WeightType s1_weight = s1_it->second;
             final_weight_map[s1] = s2_it->second;
             final_weight_map[s2] = s1_weight;
           }
           if (s1_it != end_it) {
             typename C::WeightType w = s1_it->second;
             final_weight_map.erase(s1);
             final_weight_map[s2] = w;
           }
           if (s2_it != end_it) {
             typename C::WeightType w = s2_it->second;
             final_weight_map.erase(s2);
             final_weight_map[s1] = w;
           }

           return;

         }

         static void write_weight(FILE * file, float weight)
         {
           //if (weight == 0) // avoid unnecessary 0.000000's
           //  fprintf(file, "%i", 0); 
           //else
           fprintf(file, "%f", weight);
         }

         static void write_weight(std::ostream & os, float weight)
         {
           //if (weight == 0) // avoid unnecessary 0.000000's
           //  os << 0; 
           //else
           os << weight;
         }

         /* Replace all strings \a str1 in \a symbol with \a str2. */
         static void replace_all(std::string & symbol, 
                                 const std::string &str1,
                                 const std::string &str2)
         {
           size_t pos = symbol.find(str1);
           while (pos != std::string::npos) // while there are str1:s to replace
             {
               symbol.erase(pos, str1.size()); // erase str1
               symbol.insert(pos, str2);       // insert str2 instead
               pos = symbol.find               // find next str1
                 (str1, pos+str2.size());      
             }
           return;
         }

         static void xfstize(std::string & symbol)
         {
           std::string escaped_symbol;
           for (size_t pos = 0; pos < symbol.size(); pos++)
             {
               if (symbol[pos] == '%')
                 escaped_symbol.append("\"%\"");
               else if (symbol[pos] == '"')
                 escaped_symbol.append("%\"");
               else if (symbol[pos] == '?')
                 escaped_symbol.append("\"?\"");
               else
                 escaped_symbol.append(1, symbol[pos]);
             }
           symbol = escaped_symbol;
         }

         static void xfstize_symbol(std::string & symbol)
         {
           xfstize(symbol);
           replace_all(symbol, "@_EPSILON_SYMBOL_@", "0");
           replace_all(symbol, "@_UNKNOWN_SYMBOL_@", "?");
           replace_all(symbol, "@_IDENTITY_SYMBOL_@", "?");
           replace_all(symbol, "\t", "@_TAB_@");
         }

         void print_xfst_state(std::ostream & os, HfstState state)
         {
           if (state == INITIAL_STATE) { os << "S"; }
           if (is_final_state(state)) { os << "f"; }
           os << "s" << state;
         }

         void print_xfst_state(FILE * file, HfstState state)
         {
           if (state == INITIAL_STATE) { fprintf(file, "S"); }
           if (is_final_state(state)) { fprintf(file, "f"); }
           fprintf(file, "s%i", state);
         }

         void print_xfst_arc(std::ostream & os, C data)
         {
           // replace all spaces, epsilons and tabs
           if (data.get_input_symbol() !=
               data.get_output_symbol()) 
             {
               os << "<";
             } 
           std::string s = data.get_input_symbol();
           xfstize_symbol(s);
           os << s;
           if (data.get_input_symbol() !=
               data.get_output_symbol() ||
               data.get_output_symbol() == "@_UNKNOWN_SYMBOL_@")
             {
               s = data.get_output_symbol();
               xfstize_symbol(s);
               os << ":" << s; 
             }
           if (data.get_input_symbol() !=
               data.get_output_symbol())
             {
               os << ">";
             }
         }

         void print_xfst_arc(FILE * file, C data)
         {
           if (data.get_input_symbol() !=
               data.get_output_symbol())
             {
               fprintf(file, "<");
             }
           // replace all spaces, epsilons and tabs
           std::string s = data.get_input_symbol();
           xfstize_symbol(s);
           fprintf(file, "%s", s.c_str());

           if (data.get_input_symbol() !=
               data.get_output_symbol() ||
               data.get_output_symbol() == "@_UNKNOWN_SYMBOL_@")
             {
               s = data.get_output_symbol();
               xfstize_symbol(s);
               fprintf(file, ":%s", s.c_str());
             }
           if (data.get_input_symbol() !=
               data.get_output_symbol())
             {
               fprintf(file, ">");
             }
         }

       public:

         /** @brief Write the graph in xfst text format to ostream \a os.
             \a write_weights defines whether weights are printed (todo). */
         void write_in_xfst_format(std::ostream &os, bool write_weights=true) 
         {
           (void)write_weights; // todo
           unsigned int source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {
               print_xfst_state(os, source_state);
               os << ":\t";

               if (it->begin() == it->end())
                 {
                   os << "(no arcs)";
                 }
               else
                 {
                   for (typename HfstTransitions::iterator tr_it
                          = it->begin();
                        tr_it != it->end(); tr_it++)
                     {
                       if (tr_it != it->begin())
                         {
                           os << ", ";
                         }
                       C data = tr_it->get_transition_data();
                       print_xfst_arc(os, data);

                       os << " -> ";
                       print_xfst_state(os, tr_it->get_target_state());
                     }
                 }
               os << "." << std::endl;
               source_state++;
             }          
         }

         // note: unknown and identity are both '?'
         static std::string prologize_symbol(const std::string & symbol)
         {
           if (symbol == "0")
             return "%0";
           if (symbol == "?")
             return "%?";
           if (symbol == "@_EPSILON_SYMBOL_@")
             return "0";
           if (symbol == "@_UNKNOWN_SYMBOL_@")
             return "?";
           if(symbol == "@_IDENTITY_SYMBOL_@")
             return "?";
           // prepend a backslash to a double quote
           std::string retval(symbol);
           replace_all(retval, "\"", "\\\"");
           return retval;
         }

         // caveat: '?' is always unknown
         static std::string deprologize_symbol(const std::string & symbol)
         {
           if (symbol == "%0")
             return "0";
           if (symbol == "%?")
             return "?";
           if (symbol == "0")
             return "@_EPSILON_SYMBOL_@";
           if (symbol == "?")
             return "@_UNKNOWN_SYMBOL_@";
           // remove the escaping backslash in front of a double quote
           std::string retval(symbol);
           replace_all(retval, "\\\"", "\"");
           return retval;
         }

         static void print_prolog_arc_symbols(FILE * file, C data)
         {
           std::string symbol = prologize_symbol(data.get_input_symbol());
           fprintf(file, "\"%s\"", symbol.c_str());

           if (data.get_input_symbol() !=
               data.get_output_symbol() || 
               data.get_input_symbol() == "@_UNKNOWN_SYMBOL_@")
             {
               symbol = prologize_symbol(data.get_output_symbol());
               fprintf(file, ":\"%s\"", symbol.c_str());
             }
         }
         
         static void print_prolog_arc_symbols(std::ostream & os, C data)
         {
           std::string symbol = prologize_symbol(data.get_input_symbol());
           os << "\"" << symbol << "\"";

           if (data.get_input_symbol() !=
               data.get_output_symbol() || 
               data.get_input_symbol() == "@_UNKNOWN_SYMBOL_@")
             {
               symbol = prologize_symbol(data.get_output_symbol());
               os << ":\"" << symbol << "\"";
             }
         }

         /** @brief Write the graph in prolog format to FILE \a file.
             \a write_weights defines whether weights are printed (todo). */
         void write_in_prolog_format(FILE * file, const std::string & name, 
                                     bool write_weights=true) 
         {
           unsigned int source_state=0;
           const char * identifier = name.c_str();
           // Print the name.
           if (name.find(",") != std::string::npos)
             {
               std::string msg("no commas allowed in the name of prolog networks");
               HFST_THROW_MESSAGE(HfstException, msg);
             }
           fprintf(file, "network(%s).\n", identifier);

           // Print symbols that are in the alphabet but not used in arcs.
           HfstTransitionGraphAlphabet symbols_used_ = symbols_used();
           initialize_alphabet(symbols_used_); // exclude special symbols
           for (typename HfstTransitionGraphAlphabet::const_iterator it 
                  = alphabet.begin(); it != alphabet.end(); it++)
             {
               if (symbols_used_.find(*it) == symbols_used_.end())
                 {
                   fprintf(file, "symbol(%s, \"%s\").\n", identifier, it->c_str());
                 }
             }

           // Print arcs.
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   fprintf(file, "arc(%s, %i, %i, ",
                           identifier, source_state, tr_it->get_target_state());
                   C data = tr_it->get_transition_data();
                   print_prolog_arc_symbols(file, data);
                   if (write_weights) {
                     fprintf(file, ", ");
                     write_weight(file, data.get_weight());
                   }
                   fprintf(file, ").\n");
                 }
               source_state++;
             }

           // Print final states.
           for (typename FinalWeightMap::const_iterator it 
                  = this->final_weight_map.begin();
                it != this->final_weight_map.end(); it++)
             {
               fprintf(file, "final(%s, %i", identifier, it->first);
               if (write_weights) 
                 {
                   fprintf(file, ", ");
                   write_weight(file, it->second);
                 }
               fprintf(file, ").\n");
             }
         }

         /** @brief Write the graph in prolog format to ostream \a os.
             \a write_weights defines whether weights are printed (todo). */
         void write_in_prolog_format(std::ostream & os, const std::string & name, 
                                     bool write_weights=true) 
         {
           unsigned int source_state=0;

           // Print the name.
           if (name.find(",") != std::string::npos)
             {
               std::string msg("no commas allowed in the name of prolog networks");
               HFST_THROW_MESSAGE(HfstException, msg);
             }
           os << "network(" << name << ")." << std::endl;

           // Print symbols that are in the alphabet but not used in arcs.
           HfstTransitionGraphAlphabet symbols_used_ = symbols_used();
           initialize_alphabet(symbols_used_); // exclude special symbols
           for (typename HfstTransitionGraphAlphabet::const_iterator it 
                  = alphabet.begin(); it != alphabet.end(); it++)
             {
               if (symbols_used_.find(*it) == symbols_used_.end())
                 {
                   os << "symbol(" << name << ", \"" << *it << "\")" << std::endl;
                 }
             }

           // Print arcs.
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   os << "arc(" << name << ", " << source_state << ", " << tr_it->get_target_state() << ", ";
                   C data = tr_it->get_transition_data();
                   print_prolog_arc_symbols(os, data);
                   if (write_weights) {
                     os << ", ";
                     write_weight(os, data.get_weight()); 
                   }
                   os << ")." << std::endl;
                 }
               source_state++;
             }

           // Print final states.
           for (typename FinalWeightMap::const_iterator it 
                  = this->final_weight_map.begin();
                it != this->final_weight_map.end(); it++)
             {
               os << "final(" << name << ", " << it->first;
               if (write_weights) {
                 os << ", ";
                 write_weight(os, it->second);
               }
               os <<  ")." << std::endl;
             }
         }
         
         // If \a str is of format ".+", change it to .+ and return true.
         // Else, return false.
         static bool strip_quotes_from_both_sides(std::string & str)
         {
           if (str.size() < 3)
             return false;
           if (str[0] != '"' || str[str.length()-1] != '"')
             return false;
           str.erase(0, 1);
           str.erase(str.length()-1, 1);
           return true;
         }

         // If \a str is of format .+)\.", change it to .+ and return true.
         // Else, return false.
         static bool strip_ending_parenthesis_and_comma(std::string & str)
         {
           if (str.size() < 3)
             return false;
           if (str[str.length()-2] != ')' || str[str.length()-1] != '.')
             return false;
           str.erase(str.length()-2);
           return true;
         }

         static bool parse_prolog_network_line(const std::string & line, std::string & name)
         {
           // 'network(NAME).'
           char namearr[100];
           int n = sscanf(line.c_str(), "network(%s", namearr);
           if (n != 1)
             return false;

           std::string namestr(namearr);
           // strip the ending ")." from namestr
           if (!strip_ending_parenthesis_and_comma(namestr))
             return false;

           name = namestr;
           return true;
         }

         // Get positions of \a c in \a str. If \a esc is precedes
         // \a c, \a c is not included.
         static std::vector<unsigned int> get_positions_of_unescaped_char
           (const std::string & str, char c, char esc)
         {
           std::vector<unsigned int> retval;
           for (size_t i=0; i < str.length(); i++)
             {
               if (str[i] == c)
                 {
                   if (i == 0)
                     retval.push_back(i);
                   else if (str[i-1] == esc)
                     ; // skip escaped chars
                   else
                     retval.push_back(i);
                 }
             }
           return retval;
         }

         // Extract input and output symbols, if possible, from prolog arc 
         // \a str and store them to \a isymbol and \a osymbol. 
         // Return whether symbols were succesfully extracted.
         // \a str must be of format "foo":"bar" or "foo"
         static bool get_prolog_arc_symbols
           (const std::string & str, std::string & isymbol, std::string & osymbol)
         {
           // find positions of non-escaped double quotes (todo: double double-quote?)
           std::vector<unsigned int> quote_positions
             = get_positions_of_unescaped_char(str, '"', '\\');

           // "foo"
           if (quote_positions.size() == 2)
             {
               if (quote_positions[0] != 0 ||
                   quote_positions[1] != str.length()-1)
                 return false; // extra characters outside quotes
             }
           // "foo":"bar"
           else if (quote_positions.size() == 4)
             {
               if (quote_positions[0] != 0 ||
                   quote_positions[3] != str.length()-1) 
                 {
                   return false;  // extra characters outside quotes
                 }
               if (quote_positions[2] - quote_positions[1] != 2)
                 {
                   return false;  // missing colon between inner quotes
                 }
               if (str[quote_positions[1] + 1] != ':')
                 {
                   return false;  // else than colon between inner quotes
                 }
             }
           // not valid prolog arc
           else
             {
               return false;
             }
           
           // "foo"
           if (quote_positions.size() == 2)
             {
               // "foo" -> foo
               std::string symbol(str, quote_positions[0]+1, quote_positions[1]-quote_positions[0]-1);
               isymbol = deprologize_symbol(symbol);
               if (isymbol == "@_UNKNOWN_SYMBOL_@") // single unknown -> identity
                 isymbol = "@_IDENTITY_SYMBOL_@";
               osymbol = isymbol;
             }
           // "foo":"bar"
           else
             {
               // "foo" -> foo, "bar" -> bar
               std::string insymbol(str, quote_positions[0]+1, quote_positions[1]-quote_positions[0]-1);
               std::string outsymbol(str, quote_positions[2]+1, quote_positions[3]-quote_positions[2]-1);
               isymbol = deprologize_symbol(insymbol);
               osymbol = deprologize_symbol(outsymbol);
             }

           return true;
         }

         static bool extract_weight(std::string & symbol, float & weight)
         {
           size_t last_double_quote = symbol.find_last_of('"');
           size_t last_space = symbol.find_last_of(' ');

           // at least two double quotes should be found
           if (last_double_quote == std::string::npos)
             { return false; }

           if (last_space == std::string::npos) {
             ; // no weight 
           }
           else if (last_double_quote > last_space) {
             ; // no weight, last space is part of a symbol
           }
           else if (last_double_quote + 2 == last_space && last_space < symbol.size()-1) // + 2 because of the comma
             {
               std::istringstream buffer(symbol.substr(last_space+1));
               buffer >> weight;
               if (buffer.fail()) // a float could not be read
                 { return false; }
               symbol.resize(last_space-1); // get rid of the comma and weight
             }
           else {
             return false; // not valid symbol and weight
           }
           return true;
         }

         static bool parse_prolog_arc_line(const std::string & line, HfstTransitionGraph & graph)
         {
           // symbolstr can also contain the weight
           char namestr[100]; char sourcestr[100];
           char targetstr[100]; char symbolstr[100];

           int n = sscanf(line.c_str(), "arc(%[^,], %[^,], %[^,], %[^\t\n]", 
                          namestr, sourcestr, targetstr, symbolstr);

           std::string symbol(symbolstr);

           // strip the ending ")." from symbolstr
           if (!strip_ending_parenthesis_and_comma(symbol)) 
             { return false; }

           if (n != 4)
             { return false; }
           if (std::string(namestr) != graph.name)
             { return false; }

           unsigned int source = atoi(sourcestr);
           unsigned int target = atoi(targetstr);

           // handle the weight that might be included in symbol string
           float weight = 0;
           if (! extract_weight(symbol, weight))
             { return false; }

           std::string isymbol = "";
           std::string osymbol = "";

           if (!get_prolog_arc_symbols(symbol, isymbol, osymbol))
             return false;

           graph.add_transition(source, HfstTransition<C>(target, isymbol, osymbol, weight));
           return true;
         }

         static bool parse_prolog_final_line(const std::string & line, HfstTransitionGraph & graph)
         {
           // 'final(NAME, number).' or 'final(NAME, number, weight).'
           char namestr[100];
           char finalstr[100];
           char weightstr[100];
           float weight = 0;

           unsigned int number_of_commas = 0;
           size_t pos = line.find(',');
           while (pos != std::string::npos)
             {
               number_of_commas++;
               pos = line.find(',', pos+1);
             }

           if (number_of_commas == 1)
             {
               int n = sscanf(line.c_str(), "final(%[^,], %[^)]).", namestr, finalstr);
               if (n != 2)
                 { return false; }
             }
           else if (number_of_commas == 2)
             {
               int n = sscanf(line.c_str(), "final(%[^,], %[^,], %[^)]).", namestr, finalstr, weightstr);
               if (n != 3)
                 { return false; }
               std::istringstream buffer(weightstr);
               buffer >> weight;
               if (buffer.fail()) // a float could not be read
                 { return false; }
             }
           else
             {
               return false;
             }

           if (std::string(namestr) != graph.name)
             return false;

           graph.set_final_weight(atoi(finalstr), weight);
           return true;
         }

         static bool parse_prolog_symbol_line(const std::string & line, HfstTransitionGraph & graph)
         {
           // 'symbol(NAME, "foo").'
           char namearr[100];
           char symbolarr[100];
           int n = sscanf(line.c_str(), "symbol(%[^,], %s", namearr, symbolarr);
           
           if (n != 2)
             return false;

           std::string namestr(namearr);
           std::string symbolstr(symbolarr);

           if (namestr != graph.name)
             return false;

           if (! strip_ending_parenthesis_and_comma(symbolstr))
             return false;

           if (! strip_quotes_from_both_sides(symbolstr))
             return false;
           
           graph.add_symbol_to_alphabet(symbolstr);
           return true;
         }

         // Erase newlines from the end of \a str and return \a str.
         static std::string strip_newlines(std::string & str)
         {
           for (signed int i=(signed int)str.length()-1; i >= 0; --i)
             {
               if (str[i] == '\n' || str[i] == '\r')
                 str.erase(i, 1);
               else
                 break;
             }
           return str;
         }

         // Try to get a line from \a is (if \a file == NULL)
         // or from \a file. If succesfull, strip the line from newlines,
         // increment \a linecount by one and return the line.
         // Else, throw an EndOfStreamException.
         static std::string get_stripped_line
           (std::istream & is, FILE * file, unsigned int & linecount)
         {
           char line [255];

           if (file == NULL) { /* we use streams */
             if (not is.getline(line,255).eof())
               HFST_THROW(EndOfStreamException);
           }
           else { /* we use FILEs */            
             if (NULL == fgets(line, 255, file))
               HFST_THROW(EndOfStreamException);
           }
           linecount++;

           std::string linestr(line);
           return strip_newlines(linestr);       
         }

         /* Create an HfstTransitionGraph as defined in prolog format 
            in istream \a is or FILE \a file.

            The functions is called by functions 
            read_in_prolog_format(istream&) and
            read_in_prolog_format(FILE*). 
            If \a file is NULL, it is ignored and \a is is used.
            If \a file is not NULL, it is used and \a is is ignored. */
         static HfstTransitionGraph read_in_prolog_format
           (std::istream &is, FILE *file, unsigned int & linecount) 
         {

           HfstTransitionGraph retval;
           std::string linestr;

           while(true)
             {
               try 
                 {
                   linestr = get_stripped_line(is, file, linecount);
                 }             
               catch (const EndOfStreamException & e) 
                 {
                   HFST_THROW(NotValidPrologFormatException); 
                 }

               if (linestr.length() != 0 && linestr[0] == '#')
                 {
                   continue; // comment line
                 }
               else
                 {
                   break; // first non-comment line
                 }
             }


           if (! parse_prolog_network_line(linestr, retval.name))
             {
               std::string message("first line not valid prolog: ");
               message.append(linestr);
               HFST_THROW_MESSAGE(NotValidPrologFormatException, message);
             }

           while(true)
             {
               try 
                 {
                   linestr = get_stripped_line(is, file, linecount);
                   if (linestr == "") // prolog separator 
                     {
                       return retval;
                     }
                 }             
               catch (const EndOfStreamException & e) 
                 {
                   return retval;
                 }
               
               if (! (parse_prolog_arc_line(linestr, retval) ||
                      parse_prolog_final_line(linestr, retval) ||
                      parse_prolog_symbol_line(linestr, retval)) )
                 {
                   std::string message("line not valid prolog: ");
                   message.append(linestr);
                   HFST_THROW_MESSAGE(NotValidPrologFormatException, message);
                 }
             }
           HFST_THROW(NotValidPrologFormatException); // this should not happen
         }

         static HfstTransitionGraph read_in_prolog_format
           (std::istream &is,
            unsigned int & linecount) 
         {
           return read_in_prolog_format
             (is, NULL /* a dummy variable */,
              linecount);
         }

         static HfstTransitionGraph read_in_prolog_format
           (FILE *file, 
            unsigned int & linecount) 
         {
           return read_in_prolog_format
             (std::cin /* a dummy variable */,
              file, linecount);
         }       

         static HfstTransitionGraph read_in_prolog_format
           (HfstFile &file, 
            unsigned int & linecount)
         {
           return read_in_att_format(std::cin /* a dummy variable */, file.get_file(),
                                     linecount);
         }



         /** @brief Write the graph in xfst text format to FILE \a file.
             \a write_weights defines whether weights are printed (todo). */
         void write_in_xfst_format(FILE * file, bool write_weights=true) 
         {
           (void)write_weights;
           unsigned int source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {
               print_xfst_state(file, source_state);
               fprintf(file, ":\t");

               if (it->begin() == it->end())
                 {
                   fprintf(file, "(no arcs)");
                 }
               else
                 {
                   for (typename HfstTransitions::iterator tr_it
                          = it->begin();
                        tr_it != it->end(); tr_it++)
                     {
                       if (tr_it != it->begin())
                         {
                           fprintf(file, ", ");
                         }
                       C data = tr_it->get_transition_data();

                       print_xfst_arc(file, data);

                       fprintf(file, " -> ");
                       print_xfst_state(file, tr_it->get_target_state());
                     }
                 }
               fprintf(file, ".\n");
               source_state++;
             }          
         }

         
         

         /** @brief Write the graph in AT&T format to ostream \a os.
             \a write_weights defines whether weights are printed. */
         void write_in_att_format(std::ostream &os, bool write_weights=true) 
         {
           unsigned int source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   std::string isymbol = data.get_input_symbol();
                   replace_all(isymbol, " ", "@_SPACE_@");
                   replace_all(isymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(isymbol, "\t", "@_TAB_@");

                   std::string osymbol = data.get_output_symbol();
                   replace_all(osymbol, " ", "@_SPACE_@");
                   replace_all(osymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(osymbol, "\t", "@_TAB_@");

                   os <<  source_state << "\t" 
                      <<  tr_it->get_target_state() << "\t"
                      <<  isymbol << "\t"
                      <<  osymbol;

                   if (write_weights) {
                     os <<  "\t";
                     write_weight(os, data.get_weight());
                   }
                   os << "\n";
                 }
               if (is_final_state(source_state))
                 {
                   os <<  source_state;
                   if (write_weights) {
                     os << "\t";
                     write_weight(os, get_final_weight(source_state));
                   }
                   os << "\n";
                 }
           source_state++;
             }          
         }

         /** @brief Write the graph in AT&T format to FILE \a file.
             \a write_weights defines whether weights are printed. */
         void write_in_att_format(FILE *file, bool write_weights=true) 
         {
           unsigned int source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   std::string isymbol = data.get_input_symbol();
                   replace_all(isymbol, " ", "@_SPACE_@");
                   replace_all(isymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(isymbol, "\t", "@_TAB_@");

                   std::string osymbol = data.get_output_symbol();
                   replace_all(osymbol, " ", "@_SPACE_@");
                   replace_all(osymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(osymbol, "\t", "@_TAB_@");

                   fprintf(file, "%i\t%i\t%s\t%s",
                           source_state,
                           tr_it->get_target_state(),
                           isymbol.c_str(),
                           osymbol.c_str());

                   if (write_weights) {
                     fprintf(file, "\t");
                     write_weight(file, data.get_weight());
                   } 
                   fprintf(file, "\n");
                 }
               if (is_final_state(source_state))
                 {
                   fprintf(file, "%i", source_state);
                   if (write_weights) {
                     fprintf(file, "\t"); 
                     write_weight(file, get_final_weight(source_state));
                   }
                   fprintf(file, "\n");
                 }
           source_state++;
             }          
         }

         void write_in_att_format(char * ptr, bool write_weights=true) 
         {
       unsigned int source_state=0;
       size_t cwt = 0; // characters written in total
       size_t cw = 0; // characters written in latest call to sprintf
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   std::string isymbol = data.get_input_symbol();
                   replace_all(isymbol, " ", "@_SPACE_@");
                   replace_all(isymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(isymbol, "\t", "@_TAB_@");

                   std::string osymbol = data.get_output_symbol();
                   replace_all(osymbol, " ", "@_SPACE_@");
                   replace_all(osymbol, "@_EPSILON_SYMBOL_@", "@0@");
                   replace_all(osymbol, "\t", "@_TAB_@");

                   cw = sprintf(ptr + cwt, "%i\t%i\t%s\t%s",
                                source_state,
                                tr_it->get_target_state(),
                                isymbol.c_str(),
                                osymbol.c_str());
                   
                   cwt = cwt + cw;

                   if (write_weights)
                     cw = sprintf(ptr + cwt, "\t%f",
                             data.get_weight());
                   cwt = cwt + cw;
                   cw = sprintf(ptr + cwt, "\n");
                   cwt = cwt + cw;
                 }
               if (is_final_state(source_state))
                 {
                   cw = sprintf(ptr + cwt, "%i", source_state);
                   cwt = cwt + cw;
                   if (write_weights)
                     cw = sprintf(ptr + cwt, "\t%f", 
                             get_final_weight(source_state));
                   cwt = cwt + cw;
                   cw = sprintf(ptr + cwt, "\n");
                   cwt = cwt + cw;
                 }
           source_state++;
             }          
         }


         /** @brief Write the graph in AT&T format to FILE \a file using numbers
             instead of symbol names.
             \a write_weights defines whether weights are printed. */
         void write_in_att_format_number(FILE *file, bool write_weights=true) 
         {
           unsigned int source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   fprintf(file, "%i\t%i\t%i\t%i",
                           source_state,
                           tr_it->get_target_state(),
                           tr_it->get_input_number(),
                           tr_it->get_output_number());

                   if (write_weights)
                     fprintf(file, "\t%f",
                             data.get_weight()); 
                   fprintf(file, "\n");

                   if (is_final_state(source_state))
                     {
                       fprintf(file, "%i", source_state);
                       if (write_weights)
                         fprintf(file, "\t%f", 
                                 get_final_weight(source_state));
                       fprintf(file, "\n");
                     }
                 }
               source_state++;
             }
         }


         /* Create an HfstTransitionGraph as defined in AT&T format 
            in istream \a is or FILE \a file. \a epsilon_symbol defines
            how epsilon is represented. 

            The functions is called by functions 
            read_in_att_format(istream&, std::string) and
            read_in_att_format(FILE*, std::string). 
            If \a file is NULL, it is ignored and \a is is used.
            If \a file is not NULL, it is used and \a is is ignored. */
         static HfstTransitionGraph read_in_att_format
           (std::istream &is,
            FILE *file,
            std::string epsilon_symbol,
            unsigned int & linecount) {

           if (file == NULL) {
             if (is.eof()) {
               HFST_THROW(EndOfStreamException);
             }
           }
           else {
             if (feof(file)) {
               HFST_THROW(EndOfStreamException);
             }
           }

           HfstTransitionGraph retval;
           char line [255];
           while(true) {

             if (file == NULL) { /* we use streams */
               if (not is.getline(line,255).eof())
                 break;
             }
             else { /* we use FILEs */            
               if (NULL == fgets(line, 255, file))
                 break;
             }

             linecount++;

             // an empty line signifying an empty transducer,
             // a special case that is accepted if it is the only
             // transducer in the stream
             if ( // empty line with or without a newline  
                 (line[0] == '\0') ||
                 (line[0] == '\n' && line[1] == '\0') ||
                 // windows newline
                 (line[0] == '\r' && line[1] == '\n' && line[2] == '\0')
                  ) {

               // make sure that the end-of-file is reached
               if (file == NULL)
                 is.get();
               else
                 fgetc(file);
               break;
             }

             if (*line == '-') // transducer separator line is "--"
               return retval;

             // scan one line that can have a maximum of five fields
             char a1 [100]; char a2 [100]; char a3 [100]; 
             char a4 [100]; char a5 [100];
             // how many fields could be parsed
             int n = sscanf(line, "%s%s%s%s%s", a1, a2, a3, a4, a5);

             // set value of weight
             float weight = 0;
             if (n == 2) // a final state line with weight
               weight = atof(a2);
             if (n == 5) // a transition line with weight
               weight = atof(a5);

             if (n == 1 || n == 2)  // a final state line
               retval.set_final_weight( atoi(a1), weight );

             else if (n == 4 || n == 5) { // a transition line
               std::string input_symbol=std::string(a3);
               std::string output_symbol=std::string(a4);

               // replace "@_SPACE_@"s with " " and "@0@"s with 
               // "@_EPSILON_SYMBOL_@" 
               replace_all(input_symbol, "@_SPACE_@", " ");
               replace_all(input_symbol, "@0@", "@_EPSILON_SYMBOL_@");
               replace_all(input_symbol, "@_TAB_@", "\t");
               replace_all(input_symbol, "@_COLON_@", ":");

               replace_all(output_symbol, "@_SPACE_@", " ");
               replace_all(output_symbol, "@0@", "@_EPSILON_SYMBOL_@");
               replace_all(output_symbol, "@_TAB_@", "\t");
               replace_all(output_symbol, "@_COLON_@", ":");

               if (epsilon_symbol.compare(input_symbol) == 0)
                 input_symbol="@_EPSILON_SYMBOL_@";
               if (epsilon_symbol.compare(output_symbol) == 0)
                 output_symbol="@_EPSILON_SYMBOL_@";

               HfstTransition <C> tr( atoi(a2), input_symbol, 
                                       output_symbol, weight );
               retval.add_transition( atoi(a1), tr );
             }

             else  {  // line could not be parsed
               std::string message(line);
               HFST_THROW_MESSAGE
                 (NotValidAttFormatException,
                  message);
             }    
           }
           return retval;
         }


         /** @brief Create an HfstTransitionGraph as defined in AT&T 
             transducer format in istream \a is. \a epsilon_symbol 
             defines how epsilon is represented. 
             @pre \a is not at end, otherwise an exception is thrown. 
             @note Multiple AT&T transducer definitions are separated with 
             the line "--". */
         static HfstTransitionGraph read_in_att_format
           (std::istream &is,
            std::string epsilon_symbol,
            unsigned int & linecount) 
         {
           return read_in_att_format
             (is, NULL /* a dummy variable */,
              epsilon_symbol, linecount);
         }

         /** @brief Create an HfstTransitionGraph as defined 
             in AT&T transducer format in FILE \a file. 
             \a epsilon_symbol defines how epsilon is represented. 
             @pre \a is not at end, otherwise an exception is thrown. 
             @note Multiple AT&T transducer definitions are separated with 
             the line "--". */
         static HfstTransitionGraph read_in_att_format
           (FILE *file, 
            std::string epsilon_symbol,
            unsigned int & linecount) 
         {
           return read_in_att_format
             (std::cin /* a dummy variable */,
              file, epsilon_symbol, linecount);
         }       

         static HfstTransitionGraph read_in_att_format
           (HfstFile &file, 
            std::string epsilon_symbol,
            unsigned int & linecount)
         {
           return read_in_att_format(std::cin /* a dummy variable */, file.get_file(),
                                     epsilon_symbol, linecount);
         }


     // ----------------------------------------------
     // -----       Substitution functions       -----
     // ----------------------------------------------

       protected:

     /* A function that performs in-place-substitution in the graph. */

         void substitute_(HfstSymbol old_symbol,
                          HfstSymbol new_symbol,
                          bool input_side=true, 
                          bool output_side=true)
         {
           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // Go through all transitions
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   // The substituting input and output symbols for the 
                   // current transition.
                   HfstSymbol substituting_input_symbol
                     = tr_it.get_input_symbol();
                   HfstSymbol substituting_output_symbol
                     = tr_it.get_output_symbol();

                   // Whether a substitution will be performed.
                   bool substitution_made=false;

                   if (input_side &&
                       tr_it.get_input_symbol() == old_symbol) {
                     substituting_input_symbol = new_symbol;
                     substitution_made=true;
                   }
                   if (output_side &&
                       tr_it.get_output_symbol() == old_symbol) {
                     substituting_output_symbol = new_symbol;
                     substitution_made=true;
                   }

                   // If a substitution is to be performed,
                   if (substitution_made) {

                     add_symbol_to_alphabet(new_symbol);

                     // change the current transition accordingly.
                     HfstTransition<C> tr
                       (tr_it.get_target_state(),
                        substituting_input_symbol,
                        substituting_output_symbol,
                        tr_it.get_weight());

                     it->operator[](i) = tr;
                   }

                 } // all transitions gone through

             } // ----- all states gone through -----

           return;
         }

         /* A function that performs in-place substitutions in the graph
            as defined in \a substitutions.

            substitutions[from_number] = to_number, 
            if substitutions[from_number] = no_substitution, no substitution is made */
         void substitute_(const HfstNumberVector &substitutions,
                          unsigned int no_substitution)
         {
           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // Go through all transitions
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   HfstNumber old_inumber = tr_it.get_input_number();
                   HfstNumber old_onumber = tr_it.get_output_number();

                   HfstNumber new_inumber = substitutions.at(old_inumber);
                   HfstNumber new_onumber = substitutions.at(old_onumber);

                     // If a substitution is to be performed,
                   if (new_inumber != no_substitution ||
                       new_onumber != no_substitution) 
                     {
                       if (new_inumber != no_substitution)
                         add_symbol_to_alphabet(C::get_symbol(new_inumber));
                       else
                         new_inumber = old_inumber;

                       if (new_onumber != no_substitution)
                         add_symbol_to_alphabet(C::get_symbol(new_onumber));
                       else
                         new_onumber = old_onumber;

                       // change the current transition accordingly.
                       HfstTransition<C> tr
                         (tr_it.get_target_state(),
                          new_inumber,
                          new_onumber,
                          tr_it.get_weight(), false);

                     it->operator[](i) = tr;
                   }

                 } // all transitions gone through

             } // ----- all states gone through -----

           return;
         }
         
         /* A function that performs in-place substitutions in the graph
            as defined in \a substitutions. */
         void substitute_(const HfstNumberPairSubstitutions &substitutions)
         {
           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // Go through all transitions
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   HfstNumberPair old_number_pair
                     ( tr_it.get_input_number(),
                       tr_it.get_output_number() );

                   HfstNumberPairSubstitutions::const_iterator subst_it
                     = substitutions.find(old_number_pair);

                     // If a substitution is to be performed,
                   if (subst_it != substitutions.end()) {

                     HfstNumber new_input_number = subst_it->second.first;
                     HfstNumber new_output_number = subst_it->second.second;

                     add_symbol_to_alphabet(HfstTropicalTransducerTransitionData::
                                            get_symbol(new_input_number));
                     add_symbol_to_alphabet(HfstTropicalTransducerTransitionData::
                                            get_symbol(new_output_number));

                     // change the current transition accordingly.
                     HfstTransition<C> tr
                       (tr_it.get_target_state(),
                        new_input_number,
                        new_output_number,
                        tr_it.get_weight(), false);

                     it->operator[](i) = tr;
                   }

                 } // all transitions gone through

             } // ----- all states gone through -----

           return;
         }

       public:

         /* A function that performs in-place removal of all transitions
            equivalent to \a sp in the graph. */
         void remove_transitions(const HfstSymbolPair &sp)
         {
           unsigned int in_match = C::get_number(sp.first);
           unsigned int out_match = C::get_number(sp.second);

           bool in_match_used = false;
           bool out_match_used = false;

           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // Go through all transitions of the current state
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   // If a match was found, remove the transition:
                   unsigned int in_tr = tr_it.get_input_number();
                   unsigned int out_tr = tr_it.get_output_number();
                   if (in_tr == in_match && out_tr == out_match) {
                     it->erase(it->begin()+i); }
                   else 
                     {
                       if (in_tr == in_match || out_tr == in_match) {
                         in_match_used=true; }
                       if (in_tr == out_match || out_tr == out_match) {
                         out_match_used=true; }
                     }
                 }
             }

           // Handle the alphabet
           if (!in_match_used) {
             alphabet.erase(sp.first); }
           if (!out_match_used) {
             alphabet.erase(sp.second); }
         }

       protected:

         /* A function that performs in-place-substitution in the graph. */
         void substitute_(const HfstSymbolPair &old_sp, 
                          const HfstSymbolPairSet &new_sps)
         {
           if (new_sps.empty())
             {
               return remove_transitions(old_sp);
             }

           unsigned int old_input_number = C::get_number(old_sp.first);
           unsigned int old_output_number = C::get_number(old_sp.second);

           // Whether any substitution was performed
           bool substitution_performed=false;

           // ----- Go through all states -----
           for (iterator it = begin(); it != end(); it++)
             {
               // The transitions to be added to the current state
               HfstTransitions new_transitions;

               // Go through all transitions of the current state
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   // If a match was found, substitute:
                   if (tr_it.get_input_number() == old_input_number &&
                       tr_it.get_output_number() == old_output_number)
                     {
                       substitution_performed=true;

                       // change the current transition so that it is equivalent
                       // to the first substituting transition in new_sps
                       typename HfstSymbolPairSet::const_iterator IT 
                         = new_sps.begin();

                       HfstTransition<C> tr
                         (tr_it.get_target_state(),
                          C::get_number(IT->first),
                          C::get_number(IT->second),
                          tr_it.get_weight(),
                          true);
                       
                       it->operator[](i) = tr;

                       // and schedule the rest of the substituting transitions
                       // in new_sps to be added to the current state.
                       while (IT != new_sps.end())
                         {
                           HfstTransition<C> TR
                             (tr_it.get_target_state(),
                              C::get_number(IT->first),
                              C::get_number(IT->second),
                              tr_it.get_weight(),
                              true);

                           new_transitions.push_back(TR);
                           IT++;
                         }

                     } // (substitution and scheduling done)       

                 } // (all transitions of a state gone through)
               
               // Add the new transitions to the current state
               for (typename HfstTransitions
                      ::const_iterator NIT = new_transitions.begin();
                    NIT != new_transitions.end(); NIT++)
                 {
                   it->push_back(*NIT);
                 }

             } // ( ----- all states in the graph gone through ----- )

           // If at least one substitution was performed, add all the
           // symbols in the substituting transitions to the alphabet of
           // the graph.
           if (substitution_performed) {
             add_symbols_to_alphabet(new_sps);
           }

           // Remove symbols that were removed because of substitutions
           // (or didn't occur in the graph in the first place)
           std::set<unsigned int> syms;
           /*for (typename HfstSymbolPairSet::const_iterator it = new_sps.begin();
                it != new_sps.end(); it++) {
             syms.insert(C::get_number(it->first));
             syms.insert(C::get_number(it->second)); ?????????
             }*/
           syms.insert(old_input_number);
           syms.insert(old_output_number);
           prune_alphabet_after_substitution(syms);
           
           return;
         }
             

     /* A function that performs in-place-substitution in the graph. */
     void substitute_(bool (*func)
              (const HfstSymbolPair &sp, HfstSymbolPairSet &sps))
     {
           // ----- Go through all states. -----
           for (iterator it = begin(); it != end(); it++)
             {
           // The transitions to be added to the current state.
           HfstTransitions new_transitions;

           // Go through all transitions.
               for (unsigned int i=0; i < it->size(); i++)
                 {
           HfstTransition<C> &tr_it = it->operator[](i);

           HfstSymbolPair transition_symbol_pair
             (tr_it.get_input_symbol(),
              tr_it.get_output_symbol());
           HfstSymbolPairSet substituting_transitions;

           // If a substitution is to be performed,
           bool perform_substitution=false;
           try {
             perform_substitution = 
               (*func)(transition_symbol_pair, substituting_transitions);
           }
           catch (const HfstException & e)
             {
               throw e;
             }
           if (perform_substitution)
             {          
               // change the transition to the first element
               // in new_sps
               typename HfstSymbolPairSet::const_iterator IT 
                 = substituting_transitions.begin();

               if (not C::is_valid_symbol(IT->first) ||
               not C::is_valid_symbol(IT->second) )
             HFST_THROW_MESSAGE
               (EmptyStringException,
                "HfstTransitionGraph::substitute");

               HfstTransition<C> tr
             (tr_it.get_target_state(),
              IT->first,
              IT->second,
              tr_it.get_weight());

               it->operator[](i) = tr;

               add_symbol_to_alphabet(IT->first);
               add_symbol_to_alphabet(IT->second);

               // and schedule the rest of the elements in new_sps
               // to be added to this state.
               while (IT != substituting_transitions.end())
             {

               if (not C::is_valid_symbol(IT->first) ||
                   not C::is_valid_symbol(IT->second) )
                 HFST_THROW_MESSAGE
                   (EmptyStringException,
                    "HfstTransitionGraph::substitute");

               HfstTransition<C> TR
                 (tr_it.get_target_state(),
                  IT->first,
                  IT->second,
                  tr_it.get_weight());

               new_transitions.push_back(TR);

               add_symbol_to_alphabet(IT->first);
               add_symbol_to_alphabet(IT->second);

               IT++;
             }

             } // Substitution and scheduling performed.   

         } // All transitions gone through.

           // Add the new transitions.
           for (typename HfstTransitions
              ::const_iterator NIT = new_transitions.begin();
            NIT != new_transitions.end(); NIT++)
         {
           it->push_back(*NIT);
         }

         } // ----- All states gone through. -----

       return;
     }

       public:

         /* ----------------------------------------
               The public substitution functions.
            ---------------------------------------- */

         /** @brief Substitute \a old_symbol with \a new_symbol in 
             all transitions. \a input_side and \a output_side define
             whether the substitution is made on input and output sides. */
         HfstTransitionGraph &
           substitute(const HfstSymbol &old_symbol, 
                      const HfstSymbol  &new_symbol,
                      bool input_side=true, 
                      bool output_side=true) {

       if (not C::is_valid_symbol(old_symbol) || 
           not C::is_valid_symbol(new_symbol) ) {
         HFST_THROW_MESSAGE
           (EmptyStringException,
            "HfstTransitionGraph::substitute"); }

           // If a symbol is substituted with itself, do nothing.
           if (old_symbol == new_symbol)
             return *this;
           // If the old symbol is not known to the graph, do nothing.
           if (alphabet.find(old_symbol) == alphabet.end())
             return *this;

           // Remove the symbol to be substituted from the alphabet
           // if the substitution is made on both sides.
           if (input_side && output_side) {
             /* Special symbols are always included in the alphabet */
             if (not is_epsilon(old_symbol) && 
                 not is_unknown(old_symbol) &&
                 not is_identity(old_symbol)) {
               alphabet.erase(old_symbol); }
           }
           // Insert the substituting symbol to the alphabet.
           alphabet.insert(new_symbol);

       substitute_(old_symbol, new_symbol, input_side, output_side);

           return *this;
         }

         HfstTransitionGraph &substitute_symbols
           (const HfstSymbolSubstitutions &substitutions)
           { return this->substitute(substitutions); }

         /** @brief Substitute all transitions as defined in \a substitutions */
         HfstTransitionGraph &substitute
           (const HfstSymbolSubstitutions &substitutions)
           {
             // add symbols to the global HfstTransition alphabet
             for (HfstSymbolSubstitutions::const_iterator it
                    = substitutions.begin();
                  it != substitutions.end(); it++)
               {
                 (void)get_symbol_number(it->first);
                 (void)get_symbol_number(it->second);
               }

             // how symbol numbers are substituted:
             // substitutions_[from_symbol] = to_symbol
             std::vector<unsigned int> substitutions_;
             // marker that means that no substitution is made
             unsigned int no_substitution = C::get_max_number()+substitutions.size()+1;
             substitutions_.resize
               (C::get_max_number()+1, no_substitution);
             for (HfstSymbolSubstitutions::const_iterator it
                    = substitutions.begin();
                  it != substitutions.end(); it++)
               {
                 HfstNumber from_symbol = get_symbol_number(it->first);
                 HfstNumber to_symbol = get_symbol_number(it->second);

                 substitutions_.at(from_symbol) = to_symbol;
               }

             substitute_(substitutions_, no_substitution);

             return *this;
           }

         HfstTransitionGraph &substitute_symbol_pairs
           (const HfstSymbolPairSubstitutions &substitutions)
           { return this->substitute(substitutions); }

         /** @brief Substitute all transitions as defined in \a substitutions.

             For each transition x:y, \a substitutions is searched and if 
             a mapping x:y -> X:Y is found, the transition x:y is replaced
             with X:Y. If no mapping is found, the transition remains the same.
          */
         HfstTransitionGraph &substitute
           (const HfstSymbolPairSubstitutions &substitutions)
           {
             // Convert from symbols to numbers
             HfstNumberPairSubstitutions substitutions_;
             for (HfstSymbolPairSubstitutions::const_iterator it 
                    = substitutions.begin();
                  it != substitutions.end(); it++)
               {
                 HfstNumberPair from_transition
                   (get_symbol_number(it->first.first), 
                    get_symbol_number(it->first.second));
                 HfstNumberPair to_transition
                   (get_symbol_number(it->second.first), 
                    get_symbol_number(it->second.second));
                 substitutions_[from_transition] = to_transition;
               }

             substitute_(substitutions_);

             return *this;
           }

         /** @brief Substitute all transitions \a sp with a set of transitions
             \a sps. */
         HfstTransitionGraph &substitute
           (const HfstSymbolPair &sp, const HfstSymbolPairSet &sps) 
       {
         if (not C::is_valid_symbol(sp.first) || 
         not C::is_valid_symbol(sp.second) ) {
           HFST_THROW_MESSAGE
         (EmptyStringException,
          "HfstTransitionGraph::substitute"); }

         for (typename HfstSymbolPairSet::const_iterator it = sps.begin();
          it != sps.end(); it++)
           {
         if (not C::is_valid_symbol(it->first) || 
             not C::is_valid_symbol(it->second) ) {
           HFST_THROW_MESSAGE
             (EmptyStringException,
              "HfstTransitionGraph::substitute"); }
           }

         substitute_(sp, sps);

           return *this;
         }

         /** @brief Substitute all transitions \a old_pair with 
             \a new_pair. */
         HfstTransitionGraph &substitute
           (const HfstSymbolPair &old_pair, 
            const HfstSymbolPair &new_pair) 
         {
       if (not C::is_valid_symbol(old_pair.first) || 
           not C::is_valid_symbol(new_pair.first) ||
           not C::is_valid_symbol(old_pair.second) || 
           not C::is_valid_symbol(new_pair.second) ) {
         HFST_THROW_MESSAGE
           (EmptyStringException,
            "HfstTransitionGraph::substitute"); }

       StringPairSet new_pair_set;
       new_pair_set.insert(new_pair);
       substitute_(old_pair, new_pair_set);

           return *this;
         } 

         /** @brief Substitute all transitions with a set of transitions as
             defined by function \a func. 

             \a func takes as its argument a transition \a sp and inserts
             into the set of transitions \a sps the transitions with which
             the original transition \a sp must be replaced. \a func returns
             a value indicating whether any substitution must be made, i.e.
             whether any transition was inserted into \a sps. */
         HfstTransitionGraph &
           substitute(bool (*func)
                      (const HfstSymbolPair &sp, HfstSymbolPairSet &sps) ) 
         { 
       substitute_(func);
           return *this;
         }




         /* ----------------------------------------------------           
               Substitute string pair with a transition graph
            ---------------------------------------------------- */

       protected:
         /* Used in function 
            substitute(const StringPair&, HfstTransitionGraph&) */
         struct substitution_data 
         {
           HfstState origin_state;
           HfstState target_state;
           typename C::WeightType weight;
           HfstTransitionGraph * substituting_graph;

           substitution_data(HfstState origin, 
                             HfstState target,
                             typename C::WeightType weight, 
                             HfstTransitionGraph * substituting)
           {
             origin_state=origin;
             target_state=target;
             this->weight=weight;
             substituting_graph=substituting;
           }
         };
         
         /* Used in function substitute(const StringPair&, 
                                        HfstTransitionGraph&)
            Add a copy of substituting graph with epsilon transitions between 
            states and with weight as defined in \a sub. */
         void add_substitution(const substitution_data &sub) {
           // Epsilon transition to initial state of \a graph
           HfstState s = add_state();
           HfstTransition <C> epsilon_transition
             (s, C::get_epsilon(), C::get_epsilon(), 
              sub.weight);
           add_transition(sub.origin_state, epsilon_transition);
           
           /* Offset between state numbers */
           unsigned int offset = s;
           
           // Copy \a graph
           const HfstTransitionGraph * graph = sub.substituting_graph;
           HfstState source_state=0;
           for (const_iterator it = graph->begin(); 
                it != graph->end(); it++)
             {
               for (typename HfstTransitions::const_iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   HfstTransition <C> transition
                     (tr_it->get_target_state() + offset, 
                      data.get_input_symbol(),
                      data.get_output_symbol(),
                      data.get_weight());

                   add_transition(source_state + offset, transition);
                 }
           source_state++;
             }

           // Epsilon transitions from final states of \a graph
           for (typename FinalWeightMap::const_iterator it 
                  = graph->final_weight_map.begin();
                it != graph->final_weight_map.end(); it++)
             {
               HfstTransition <C> epsilon_transition
                 (sub.target_state, C::get_epsilon(), C::get_epsilon(),
                  it->second);
               add_transition(it->first + offset, epsilon_transition);
             }
         }


       public:

         /** @brief Substitute all transitions \a old_symbol : \a new_symbol
             with a copy of \a graph.

             Copies of \a graph are attached to this graph with
             epsilon transitions. 

             The weights of the transitions to be substituted are copied
             to epsilon transitions leaving from the source state of
             the transitions to be substituted to the initial state
             of a copy of \a graph.

             The final weights in \a 
             graph are copied to epsilon transitions leading from
             the final states (after substitution non-final states)
             of \a graph to target states of transitions
             \a old_symbol : \a new_symbol (that are substituted)
             in this graph.            
         */
         HfstTransitionGraph &
           substitute(const HfstSymbolPair &sp, 
              const HfstTransitionGraph &graph) {

           if ( not ( C::is_valid_symbol(sp.first) &&              
                      C::is_valid_symbol(sp.second) ) ) {
             HFST_THROW_MESSAGE
               (EmptyStringException, 
                "HfstTransitionGraph::substitute(const HfstSymbolPair&, "
                "const HfstTransitionGraph&)");
           }


           // If neither symbol to be substituted is known to the graph,
           // do nothing.
           if (alphabet.find(sp.first) == alphabet.end() && 
               alphabet.find(sp.second) == alphabet.end())
             return *this;

           // Where the substituting copies of substituting graphs
           // are inserted (source state, target state, weight)
           std::vector<substitution_data> substitutions;

           // Go through all states
           HfstState source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {

               // The transitions that are substituted, i.e. removed
               std::vector<typename HfstTransitions::iterator> 
                 old_transitions;

               // Go through all transitions
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   // Whether there is anything to substitute 
                   // in this transition
                   if (data.get_input_symbol() == sp.first &&
                       data.get_output_symbol() == sp.second) 
                     {
                       // schedule a substitution
                       substitutions.push_back(substitution_data
                                               (source_state, 
                                                tr_it->get_target_state(), 
                                                data.get_weight(),
                                                const_cast<HfstTransitionGraph *>(&graph)));
                       // schedule the old transition to be deleted
                       old_transitions.push_back(tr_it);
                     }
                   // (one transition gone through)
                 } 
               // (all transitions in a state gone through)

               // Remove the substituted transitions
               for (typename std::vector<typename 
                      HfstTransitions::iterator>::iterator IT =
                      old_transitions.begin(); 
                    IT != old_transitions.end(); IT++) {
                 it->erase(*IT);
               }

               source_state++;
             }
           // (all states gone trough)

           // Add the substitutions
           for (typename std::vector<substitution_data>::iterator IT 
                  = substitutions.begin();
                IT != substitutions.end(); IT++)
             {
               add_substitution(*IT);
             }
           return *this;
         }

         // ####
         // another version of substitute for internal use..
         // ####
         typedef std::map<HfstSymbol, HfstTransitionGraph> SubstMap;
         
         HfstTransitionGraph &
           substitute(SubstMap & substitution_map,
                      bool harmonize) {
           
           bool symbol_found = false;
           for (typename SubstMap::const_iterator it = substitution_map.begin();
                it != substitution_map.end(); it++)
             {
               if ( not ( C::is_valid_symbol(it->first) ))
                 {
                   HFST_THROW_MESSAGE(EmptyStringException, 
                    "HfstTransitionGraph::substitute "
                    "(const std::map<HfstSymbol, HfstTransitionGraph> &)");
                 }
               if (!symbol_found && alphabet.find(it->first) != alphabet.end()) 
                 {
                   symbol_found = true;
                 }
             }
           
           // If none of the symbols to be substituted is known to the graph,
           // do nothing.
           if (!symbol_found)
             {
               return *this;
             }
           
           std::set<String> substitutions_performed_for_symbols;

           // Where the substituting copies of graphs
           // are inserted (source state, target state, weight)
           std::vector<substitution_data> substitutions;
           
           // Go through all states
           HfstState source_state=0;
           for (iterator it = begin(); it != end(); it++)
             {

               // The transitions that are substituted, i.e. removed
               std::stack<typename HfstTransitions::iterator> 
                 old_transitions;

               // Go through all transitions
               for (typename HfstTransitions::iterator tr_it
                      = it->begin();
                    tr_it != it->end(); tr_it++)
                 {
                   C data = tr_it->get_transition_data();

                   // Whether there is anything to substitute 
                   // in this transition
                   String istr = data.get_input_symbol();
                   String ostr = data.get_output_symbol();
                   typename SubstMap::iterator map_it_input = substitution_map.find(istr);
                   typename SubstMap::iterator map_it_output = substitution_map.find(ostr);

                   if (map_it_input == substitution_map.end() &&
                       map_it_output == substitution_map.end())
                     {
                       ;
                     }
                   else if (istr != ostr)
                     {
                       std::string msg("symbol to be substituted must not occur only on one side of transition");
                       HFST_THROW_MESSAGE(HfstException, msg);
                     }
                   else
                     {
                       // schedule a substitution
                       substitution_data sd
                         (source_state,
                          tr_it->get_target_state(), 
                          data.get_weight(),
                          &(map_it_input->second));
                       substitutions.push_back(sd);
                       // schedule the old transition to be deleted
                       old_transitions.push(tr_it);
                       // ...
                       substitutions_performed_for_symbols.insert(istr);
                     }
                   // (one transition gone through)
                 } 
               // (all transitions in a state gone through)

               // Remove the substituted transitions
               while (!old_transitions.empty()) 
                 {
                   it->erase(old_transitions.top());
                   old_transitions.pop();
                 }
               
               source_state++;
             }
           // (all states gone trough)

           // Remove all symbols that were substituted
           for (StringSet::const_iterator sym_it = substitutions_performed_for_symbols.begin();
                sym_it != substitutions_performed_for_symbols.end(); sym_it++)
             {
               if (*sym_it != "@_EPSILON_SYMBOL_@" && *sym_it != "@_UNKNOWN_SYMBOL_@" && *sym_it != "@_IDENTITY_SYMBOL_@")
                 this->remove_symbol_from_alphabet(*sym_it);
             }

           // Harmonize the resulting and the substituting graphs, if needed
           if (harmonize)
             {
               for (StringSet::iterator sym_it = substitutions_performed_for_symbols.begin();
                    sym_it != substitutions_performed_for_symbols.end(); sym_it++)
                 {
                   this->harmonize(substitution_map.at(*sym_it));
                 }
             }

           // Add the substitutions
           for (typename std::vector<substitution_data>::iterator IT 
                  = substitutions.begin();
                IT != substitutions.end(); IT++)
             {
               add_substitution(*IT);
             }

           return *this;
         }

         /* ----------------------------
               Insert freely functions
            ---------------------------- */


         /** @brief Insert freely any number of \a symbol_pair in 
             the graph with weight \a weight. */
         HfstTransitionGraph &insert_freely
           (const HfstSymbolPair &symbol_pair, typename C::WeightType weight) 
           {    
         if ( not ( C::is_valid_symbol(symbol_pair.first) &&           
                C::is_valid_symbol(symbol_pair.second) ) ) {
           HFST_THROW_MESSAGE
         (EmptyStringException, 
          "HfstTransitionGraph::insert_freely"
          "(const HfstSymbolPair&, W)");
         }

             alphabet.insert(symbol_pair.first);
             alphabet.insert(symbol_pair.second);

         HfstState source_state=0;
             for (iterator it = begin(); it != end(); it++) {
               HfstTransition <C> tr( source_state, symbol_pair.first, 
                                      symbol_pair.second, weight );              
               it->push_back(tr);
           source_state++;
             }

             return *this;
           }

         /** @brief Insert freely any number of any symbol in \a symbol_pairs in 
             the graph with weight \a weight. */
         HfstTransitionGraph &insert_freely
           (const HfstSymbolPairSet &symbol_pairs, 
            typename C::WeightType weight) 
           {
             for (typename HfstSymbolPairSet::const_iterator symbols_it 
                    = symbol_pairs.begin();
                  symbols_it != symbol_pairs.end(); symbols_it++)
               {
                 if ( not ( C::is_valid_symbol(symbols_it->first) &&           
                            C::is_valid_symbol(symbols_it->second) ) ) {
                   HFST_THROW_MESSAGE
                     (EmptyStringException, 
                      "HfstTransitionGraph::insert_freely"
                      "(const HfstSymbolPairSet&, W)");
                 }

                 alphabet.insert(symbols_it->first);
                 alphabet.insert(symbols_it->second);
               }

             HfstState source_state=0;
             for (iterator it = begin(); it != end(); it++) 
               {
                 for (typename HfstSymbolPairSet::const_iterator symbols_it 
                        = symbol_pairs.begin();
                      symbols_it != symbol_pairs.end(); symbols_it++)
                   {
                     HfstTransition <C> tr( source_state, symbols_it->first, 
                                            symbols_it->second, weight );
                     it->push_back(tr);
                   }
                 source_state++;
               }

             return *this;
           }

         /** @brief Insert freely any number of \a graph in this
             graph. */
         HfstTransitionGraph &insert_freely
           (const HfstTransitionGraph &graph)
           {
         HfstSymbol marker_this = C::get_marker(alphabet);
         HfstSymbol marker_graph = C::get_marker(alphabet);
         HfstSymbol marker = marker_this;
         if (marker_graph > marker)
           marker = marker_graph;

             HfstSymbolPair marker_pair(marker, marker);
             insert_freely(marker_pair, 0);
             substitute(marker_pair, graph);
             alphabet.erase(marker); // TODO: fix

             return *this;
           }


         /* -------------------------------
                Harmonization function
            ------------------------------- */

         /** @brief Harmonize this HfstTransitionGraph and \a another.

             In harmonization the unknown and identity symbols in 
             transitions of both graphs are expanded according to
             the symbols that are previously unknown to the graph. 

             For example the graphs
 \verbatim 
    [a:b ?:?]
    [c:d ? ?:c]
 \endverbatim
             are expanded to
 \verbatim
    [ a:b [?:? | ?:c | ?:d | c:d | d:c | c:? | d:?] ] 
    [ c:d [? | a | b] [?:c| a:c | b:?] ]
 \endverbatim
             when harmonized.
              The symbol "?" means \@_UNKNOWN_SYMBOL_\@ in either or 
             both sides of a transition 
             (transitions of type [?:x], [x:?] and [?:?]).
             The transition [?] means [\@_IDENTITY_SYMBOL_\@].

             @note This function is always called for arguments of functions
             that take two or more graphs as their arguments, unless otherwise
             said.
         */
         HfstTransitionGraph &harmonize(HfstTransitionGraph &another) 
       {
         HarmonizeUnknownAndIdentitySymbols foo(*this, another);
         return *this;
       }


         /* -------------------------------
                 Disjunction functions
            ------------------------------- */

       protected:
         /* Disjunct the transition of path \a spv pointed by \a it
            to state \a s. If the transition does not exist in the graph,
            it is created as well as its target state.

            @return The final state of path \a spv, when \a it is at end. */
         HfstState disjunct(const StringPairVector &spv, 
                            StringPairVector::const_iterator &it,
                            HfstState s)
         {
           // Path inserted, return the final state on this path
           if (it == spv.end()) {
             return s;
           }

           HfstTransitions tr = state_vector[s];
           bool transition_found=false;
           /* The target state of the transition followed or added */
           HfstState next_state; 

           // Find the transition
           // (Searching is slow?)
           for (typename HfstTransitions::iterator tr_it = tr.begin();
                tr_it != tr.end(); tr_it++)
             {
               C data = tr_it->get_transition_data();
               if (data.get_input_symbol().compare(it->first) == 0 &&
                   data.get_output_symbol().compare(it->second) == 0)
                 {
                   transition_found=true;
                   next_state = tr_it->get_target_state();
                   break;
                 }
             }

           // If not found, create the transition
           if (not transition_found)
             {
               next_state = add_state();
               HfstTransition <C> transition(next_state, it->first,
                                              it->second, 0);
               add_transition(s, transition);
             }

           // Advance to the next transition on path
           it++;
           return disjunct(spv, it, next_state);
         }

       public:

         /** @brief Disjunct this graph with a one-path graph 
             defined by string pair vector \a spv with weight \a weight. 

             @pre This graph must be a trie where all weights are in
             final states, i.e. all transitions have a zero weight. 

             There is no way to test whether a graph is a trie, so the use
             of this function is probably limited to fast construction 
             of a lexicon. Here is an example: 

             \verbatim
             HfstBasicTransducer lexicon;
             HfstTokenizer TOK;
             lexicon.disjunct(TOK.tokenize("dog"), 0.3);
             lexicon.disjunct(TOK.tokenize("cat"), 0.5);
             lexicon.disjunct(TOK.tokenize("elephant"), 1.6);
             \endverbatim

         */
         HfstTransitionGraph &disjunct
           (const StringPairVector &spv, typename C::WeightType weight) 
         {
           StringPairVector::const_iterator it = spv.begin();
           HfstState final_state = disjunct(spv, it, INITIAL_STATE);

           // Set the weight of final state
           if (is_final_state(final_state)) 
             {
               float old_weight = get_final_weight(final_state);
               if (old_weight < weight) 
                 return *this; /* The same path with smaller weight remains */
             }
           set_final_weight(final_state, weight);
           return *this;
         }

         bool is_special_symbol(const std::string & symbol)
           {
             if (symbol.size() < 2)
               return false;
             if (symbol[0] == '@' && symbol[1] == '_')
               return true;
             return false;
           }

         HfstTransitionGraph &complete()
           {
             HfstState failure_state = add_state();
             HfstState current_state = 0;

             for (iterator it = begin(); it != end(); it++)
               {
                 std::set<HfstSymbol> symbols_present;

                 for (typename HfstTransitions::iterator tr_it
                        = it->begin();
                      tr_it != it->end(); tr_it++)
                   {
                     C data = tr_it->get_transition_data();

                     if (data.get_input_symbol() != data.get_output_symbol())
                       {
                         HFST_THROW(TransducersAreNotAutomataException);
                       }
                     symbols_present.insert(data.get_input_symbol());
                   }
                 for (std::set<std::string>::const_iterator alpha_it = alphabet.begin();
                      alpha_it != alphabet.end(); alpha_it++)
                   {
                     if (symbols_present.find(*alpha_it) == 
                         symbols_present.end() && 
                         ! is_special_symbol(*alpha_it))
                       {
                         add_transition
                           (current_state,
                            HfstBasicTransition(failure_state, *alpha_it, *alpha_it, 0));
                       }
                   }
                 current_state++;
               }
           }

         StringSet get_flags() const
           {
             StringSet flags;
             for (StringSet::const_iterator it = alphabet.begin();
                  it != alphabet.end(); it++) 
               {
                 if (FdOperation::is_diacritic(*it)) {
                   flags.insert(*it);
                 }
               }
             return flags;
           }

         // Whether symbol \a symbol must be purged from transitions and alphabet
         // of a transducer after \a flag has been eliminated from the transducer.
         // If \a flag is the empty string, all flags have been eliminated.
         bool purge_symbol(const std::string & symbol, const std::string & flag)
         {         
           if (! FdOperation::is_diacritic(symbol))
             return false;
           if (flag == "")
             return true;
           else if (FdOperation::get_feature(symbol) == flag)
             return true;
           return false;
         }

         // Replace arcs in \a transducer that use flag \a flag with epsilon arcs
         // and remove \a flag from alphabet of \a transducer. If \a flag is the empty                                                  
         // string, replace/remove all flags.
         void flag_purge(const std::string & flag)
         {
           // (1) Go through all states and transitions
           for (iterator it = begin(); it != end(); it++)
             {
               for (unsigned int i=0; i < it->size(); i++)
                 {
                   HfstTransition<C> &tr_it = it->operator[](i);

                   if ( purge_symbol(tr_it.get_input_symbol(), flag) ||
                        purge_symbol(tr_it.get_output_symbol(), flag) )
                     {
                       // change the current transition
                       HfstTransition<C> tr
                         (tr_it.get_target_state(), "@_EPSILON_SYMBOL_@",
                          "@_EPSILON_SYMBOL_@", tr_it.get_weight());
                       it->operator[](i) = tr;
                     }
                 }
             }
           // (2) Go through the alphabet
           StringSet extra_symbols;
           for (StringSet::const_iterator it = alphabet.begin();
                it != alphabet.end(); it++) 
             {
               if (purge_symbol(*it, flag))
                 extra_symbols.insert(*it);
             }
           // remove symbols
           remove_symbols_from_alphabet(extra_symbols);
         }

         /* A topological sort. */
         struct TopologicalSort
         {
           std::vector<int> distance_of_state;
           std::vector<std::set<HfstState> > states_at_distance;

           /* Initialize the TopologicalSort by reserving space for a transducer 
              with biggest state number \a biggest_state_number, */
           void set_biggest_state_number(unsigned int biggest_state_number)
           {
             distance_of_state = std::vector<int>(biggest_state_number+1, -1);
           }

           /* Set the maximum distance of \a state to \a distance, */
           void set_state_at_distance(HfstState state, unsigned int distance,
                                      bool overwrite)
           {
             // see that 'state' does not exceed the maximum state number given in initialization
             if (state > (distance_of_state.size() - 1))
               {
                 std::cerr << "ERROR in TopologicalSort::set_state_at_distance: first argument ("
                           << state << ") is out of range (should be < " << distance_of_state.size() 
                           << ")" << std::endl;
               }
             // if there is nothing on index 'state',
             // push back empty sets of states up to index 'state', including
             while (distance + 1 > (unsigned int)states_at_distance.size()) 
               {
                 std::set<HfstState> empty_set;
                 states_at_distance.push_back(empty_set);
               }
             // if there was previous distance defined for 'state', erase it, if needed
             int previous_distance = distance_of_state.at(state);
             if (previous_distance != -1 && previous_distance != distance && overwrite)
               {
                 states_at_distance.at(previous_distance).erase(state);
               }
             // set state and distance
             states_at_distance.at(distance).insert(state);
             distance_of_state.at(state) = distance;
           }

           /* The states that have a maximum distance of \a distance. */
           const std::set<HfstState> & get_states_at_distance(unsigned int distance)
           {
             // if there is nothing on index 'state',
             // push back empty sets of states up to index 'state', including
             while (distance > (states_at_distance.size() - 1)) 
               {
                 std::set<HfstState> empty_set;
                 states_at_distance.push_back(empty_set);
               }
             return states_at_distance.at(distance);
           }
         };

         enum SortDistance { MaximumDistance, MinimumDistance };

         /* 
            Get a topological (maximum distance) sort of this graph.
            @return A vector of sets of states. At each vector index ind, the
            result contains the set of all states whose (maximum) distance from
            the start state is ind.
         */
         std::vector<std::set<HfstState> > topsort(SortDistance dist) const
           {
             typedef std::set<HfstState>::const_iterator StateIt;
             unsigned int current_distance = 0; // topological distance
             TopologicalSort TopSort;
             TopSort.set_biggest_state_number(state_vector.size()-1);
             TopSort.set_state_at_distance(0,current_distance,(dist == MaximumDistance));
             bool new_states_found = false; // end condition for do-while loop

            do
              {
                new_states_found = false;
                // states that are accessible from the current set of states
                std::set<HfstState> new_states;

                // go through all states at current distance
                const std::set<HfstState> & states = 
                  TopSort.get_states_at_distance(current_distance);
                for (StateIt state_it = states.begin();
                     state_it != states.end(); state_it++)
                  {
                    // go through all transitions of each state
                    const HfstTransitions & transitions 
                      = this->state_vector.at(*state_it);
                    for (typename HfstTransitions::const_iterator transition_it 
                           = transitions.begin();
                         transition_it != transitions.end(); transition_it++)
                      {
                        new_states_found = true;
                        new_states.insert(transition_it->get_target_state());
                      } 
                    // all transitions gone through
                  }
                // all states gone through
                
                // set each accessible state at distance one higher than the
                // current distance
                for (StateIt it = new_states.begin(); 
                     it != new_states.end(); it++)
                  {
                    TopSort.set_state_at_distance(*it, current_distance + 1, (dist == MaximumDistance)); 
                  }
                current_distance++;
              }
            while (new_states_found);

            return TopSort.states_at_distance;
          }

        /** The length of longest string accepted by this graph. 
            If no string is accepted, return -1. */
         int longest_path_size()
        {
          // get topological maximum distance sort
          std::vector<std::set<HfstState> > states_sorted = this->topsort(MaximumDistance);
          // go through all sets of states in descending order
          for (int distance = states_sorted.size() - 1; distance >= 0; distance--)
            {
              const std::set<HfstState> & states 
                = states_sorted.at((unsigned int)distance);
              // go through all states in a set
              for (std::set<HfstState>::const_iterator it = states.begin();
                   it != states.end(); it++)
                {
                  // if a final state is encountered, return the distance
                  // of that state
                  if (is_final_state(*it))
                    {
                      return distance;
                    }
                }
            }
          // if no final states were encountered, return a negative value
          return -1;
        }

         /** The lengths of strings accepted by this graph, in descending order. 
             If not string is accepted, return an empty vector. */
         std::vector<unsigned int> path_sizes()
           {
             std::vector<unsigned int> result;
             // get topological maximum distance sort
             std::vector<std::set<HfstState> > states_sorted = this->topsort(MinimumDistance);
             // go through all sets of states in descending order
             for (int distance = states_sorted.size() - 1; distance >= 0; distance--)
               {
                 const std::set<HfstState> & states 
                   = states_sorted.at((unsigned int)distance);
                 // go through all states in a set
                 for (std::set<HfstState>::const_iterator it = states.begin();
                      it != states.end(); it++)
                   {
                     // if a final state is encountered, add its distance
                     // to result
                     if (is_final_state(*it))
                       {
                         result.push_back((unsigned int)distance);
                         break; // go to next set of states
                       }
                   }
               }
             return result;
           }

         bool is_infinitely_ambiguous
           (HfstState state, 
            std::set<HfstState> &epsilon_path_states,
            std::vector<unsigned int> &states_handled)
         {
           if (states_handled[state] != 0)
             return false;

           // Go through all transitions in this state                                 
           const HfstBasicTransducer::HfstTransitions &transitions 
             = this->operator[](state);
           for (HfstBasicTransducer::HfstTransitions::const_iterator it
                  = transitions.begin();
                it != transitions.end(); it++)
             {
               // (Diacritics are also treated as epsilons, although it might cause false                                                   
               //  positive results, because loops with diacritics can be invalidated by                                                    
               //  other diacritics.)                                                  
               if ( is_epsilon(it->get_input_symbol()) ||
                    FdOperation::is_diacritic(it->get_input_symbol()) )
                 {
                   epsilon_path_states.insert(state);
                   if (epsilon_path_states.find(it->get_target_state())
                       != epsilon_path_states.end())
                     {
                       return true;
                     }
                   if (is_infinitely_ambiguous
                       (it->get_target_state(), epsilon_path_states, states_handled))
                     {
                       return true;
                     }
                   epsilon_path_states.erase(state);
                 }               
             }
           // mark state as handled
           states_handled[state] = 1;
           return false;
         }
         
         bool is_infinitely_ambiguous()
         {
           std::set<HfstState> epsilon_path_states;
           HfstState max_state = this->get_max_state();
           std::vector<unsigned int> states_handled(max_state+1, 0);

           for (unsigned int state = INITIAL_STATE; state < (max_state+1); state++)
             {
               if (is_infinitely_ambiguous(state, epsilon_path_states, states_handled))
                 return true;
             }
           return false;
         }

         bool is_lookup_infinitely_ambiguous
           (const HfstOneLevelPath& s,
            unsigned int& index, HfstState state,
            std::set<HfstState> &epsilon_path_states)
         {
           // Whether the end of the lookup path s has been reached                    
           bool only_epsilons=false;
           if ((unsigned int)s.second.size() == index)
             {
               only_epsilons=true;
             }
           
           // Go through all transitions in this state                                 
           const HfstBasicTransducer::HfstTransitions &transitions 
             = this->operator[](state);
           for (HfstBasicTransducer::HfstTransitions::const_iterator it
                  = transitions.begin();
                it != transitions.end(); it++)
             {
               // CASE 1: Input epsilons do not consume a symbol in the lookup path s,               
               //         so they can be added freely.                                 
               // (Diacritics are also treated as epsilons, although it might cause false                                                   
               //  positive results, because loops with diacritics can be invalidated by                                                    
               //  other diacritics.)                                                  
               if ( is_epsilon(it->get_input_symbol()) ||
                    FdOperation::is_diacritic(it->get_input_symbol()) )
                 {
                   epsilon_path_states.insert(state);
                   if (epsilon_path_states.find(it->get_target_state())
                       != epsilon_path_states.end())
                     {
                       return true;
                     }
                   if (is_lookup_infinitely_ambiguous
                       (s, index, it->get_target_state(), epsilon_path_states))
                     {
                       return true;
                     }
                   epsilon_path_states.erase(state);
                 }
               
               /* CASE 2: Other input symbols consume a symbol in the lookup path s,   
                  so they can be added only if the end of the lookup path s has not    
                  been reached. */
               else if (not only_epsilons)
                 {
                   if ( it->get_input_symbol().compare(s.second.at(index)) == 0 )
                     {
                       index++; // consume an input symbol in the lookup path s            
                       std::set<HfstState> empty_set;
                       if (is_lookup_infinitely_ambiguous
                           (s, index, it->get_target_state(), empty_set))
                         {
                           return true;
                         }
                       index--; // add the input symbol back to the lookup path s.         
                     }
                 }
             }
           return false;
         }



         bool is_lookup_infinitely_ambiguous(const HfstOneLevelPath & s)
         {
           std::set<HfstState> epsilon_path_states;
           epsilon_path_states.insert(0);
           unsigned int index=0;

           return is_lookup_infinitely_ambiguous(s, index, INITIAL_STATE,
                                                 epsilon_path_states);
         }

         bool is_lookup_infinitely_ambiguous(const StringVector & s)
         {
           std::set<HfstState> epsilon_path_states;
           epsilon_path_states.insert(0);
           unsigned int index=0;
           HfstOneLevelPath path(0, s);

           return is_lookup_infinitely_ambiguous(path, index, INITIAL_STATE,
                                                 epsilon_path_states);
         }





         static void push_back_to_two_level_path
           (HfstTwoLevelPath &path,
            const StringPair &sp,
            const float &weight)
         {
           path.second.push_back(sp);
           path.first = path.first + weight;
         }
         
         static void pop_back_from_two_level_path
           (HfstTwoLevelPath &path,
            const float &weight)
         {
           path.second.pop_back();
           path.first = path.first - weight;
         }
         
         static void add_to_results
           (HfstTwoLevelPaths &results,
            HfstTwoLevelPath &path_so_far,
            const float &final_weight,
            float * max_weight)
         {
           path_so_far.first = path_so_far.first + final_weight;
           
           if (max_weight == NULL) // no weight limitation given
             {
               results.insert(path_so_far);
             }
           else if (!(path_so_far.first > *max_weight)) // weight limitation not exceeded
             {
               results.insert(path_so_far);
             }
           else // weight limitation exceeded
             {
               ;
             }
           path_so_far.first = path_so_far.first - final_weight;
         }

         static bool is_possible_transition
           (const HfstBasicTransition &transition,
            const StringVector &lookup_path,
            const unsigned int &lookup_index,
            const StringSet &alphabet,
            bool &input_symbol_consumed)
         {
           std::string isymbol = transition.get_input_symbol();
           
           // If we are not at the end of lookup_path,
           if (not (lookup_index == (unsigned int)lookup_path.size()))
             {
               // we can go further if the current symbol in lookup_path
               // matches to the input symbol of the transition, i.e
               // either the input symbol is the same as the current symbol
               if ( isymbol.compare(lookup_path.at(lookup_index)) == 0 ||
                    // or the input symbol is the identity symbol and
                    // the current symbol is not found in the alphabet
                    // of the transducer.
                    ( is_identity(isymbol) &&
                      (alphabet.find(lookup_path.at(lookup_index)) 
                       == alphabet.end()) ) 
                    )
                 {
                   input_symbol_consumed=true;
                   return true;
                 }
             }
           // Whether there are more symbols in lookup_path or not,
           // we can always go further if the input symbol of the transition
           // is an epsilon or a flag diacritic.
           if ( is_epsilon(isymbol) || 
                FdOperation::is_diacritic(isymbol) )
             {
               input_symbol_consumed=false;
               return true;
             }
           
           // No matches.
           return false;
         }
         
         void lookup_fd
           (const StringVector &lookup_path,
            HfstTwoLevelPaths &results,
            HfstState state,
            unsigned int lookup_index, // an iterator instead?
            HfstTwoLevelPath &path_so_far,
            StringSet &alphabet,
            HfstEpsilonHandler Eh,
            size_t infinite_cutoff,
            float * max_weight = NULL)
         {
           // Check whether the number of input epsilon cycles is exceeded
           if (not Eh.can_continue(state)) {
             return;
           }
           // Check whether the maximum weight is exceeded
           if (max_weight != NULL && path_so_far.first > *max_weight) {
             return;
           }
           
           // If we are at the end of lookup_path,
           if (lookup_index == lookup_path.size())
             {
               // and if the current state is final, 
               if (this->is_final_state(state)) 
                 {
                   // path_so_far is a valid result if max_weight is not exceeded
                   add_to_results
                     (results, path_so_far, this->get_final_weight(state), max_weight);
                 }
             }
           
           // Whether there are more symbols in lookup_path or not,
           // go through all transitions in the current state.
           const HfstBasicTransducer::HfstTransitions &transitions 
             = this->operator[](state);
           for (HfstBasicTransducer::HfstTransitions::const_iterator it 
                  = transitions.begin();
                it != transitions.end(); it++)
             {
               bool input_symbol_consumed=false;
               if ( is_possible_transition
                    (*it, lookup_path, lookup_index, alphabet, 
                     input_symbol_consumed) )
                 {
                   // update path_so_far and lookup_index
                   
                   if (not (is_identity(it->get_input_symbol()))) {
                     push_back_to_two_level_path
                       (path_so_far, 
                        StringPair(it->get_input_symbol(), 
                                   it->get_output_symbol()),
                        it->get_weight());
                   }
                   else { // identity symbol is replaced with the lookup symbol
                     push_back_to_two_level_path
                       (path_so_far, 
                        StringPair(lookup_path.at(lookup_index), 
                                   lookup_path.at(lookup_index)),
                        it->get_weight());
                   }
                   
                   HfstEpsilonHandler * Ehp = NULL;
                   if (input_symbol_consumed) {
                     lookup_index++;
                     Ehp = new HfstEpsilonHandler(infinite_cutoff);
                   }
                   else {
                     Eh.push_back(state);
                     Ehp = &Eh;
                   }
                   
                   // call lookup for the target state of the transition
                   lookup_fd(lookup_path, results, it->get_target_state(),
                             lookup_index, path_so_far, alphabet, *Ehp, infinite_cutoff, max_weight);
                   
                   // return to the original values of path_so_far 
                   // and lookup_index
                   if (input_symbol_consumed) {
                     lookup_index--;
                     delete Ehp;
                   }
                   else {
                     // Eh.pop_back();  not needed because the destructor
                     // of Eh is automatically called next
                   }
                   
                   pop_back_from_two_level_path(path_so_far, it->get_weight());
                 }
             }
           
         }
         
         void lookup_fd
           (const StringVector &lookup_path,
            HfstTwoLevelPaths &results,
            size_t infinite_cutoff,
            float * max_weight = NULL)
         {
           HfstState state = 0;
           unsigned int lookup_index = 0;
           HfstTwoLevelPath path_so_far;
           StringSet alphabet = this->get_alphabet();
           HfstEpsilonHandler Eh(infinite_cutoff);
           lookup_fd(lookup_path, results, state, lookup_index, path_so_far, 
                     alphabet, Eh, infinite_cutoff, max_weight);
         }


/*      /\** @brief Determine whether this graph has input-epsilon cycles. */
/*       *\/ */
/*      bool has_input_epsilon_cycles(void) */
/*      { */
/*                  typedef std::map<HfstState,  */
/*           std::set<HfstTransition<C> > > */
/*           HfstStates; */
/*         HfstStates state_map; */

/*          std::set<HfstState> total_seen; */
/*          for (state_vector::iterator it = state_vector.begin(); */
/*               it != state_vector.end(); ++it) { */
/*              if (total_seen.count(*it) != 0) { */
/*                  continue; */
/*              } */
        
/*          } */
/*      } */
        

    // --- Friends ---

        friend class ConversionFunctions;
        friend class hfst::HarmonizeUnknownAndIdentitySymbols;
      };
   
    /** @brief An HfstTransitionGraph with transitions of type 
    HfstTropicalTransducerTransitionData and weight type float.
    
    This is probably the most useful kind of HfstTransitionGraph. */
    typedef HfstTransitionGraph <HfstTropicalTransducerTransitionData> 
      HfstBasicTransducer;
    
    /** @brief A specialization for faster conversion. */
    typedef HfstTransitionGraph <HfstFastTransitionData> 
      HfstFastTransducer;

 
       }
   
}

#endif // #ifndef _HFST_TRANSITION_GRAPH_H_
