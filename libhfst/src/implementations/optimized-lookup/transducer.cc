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

#include "./transducer.h"

#include <cstdio> // testing

#ifndef MAIN_TEST

namespace hfst_ol {

bool should_ascii_tokenize(unsigned char c)
{
    return c <= 127;
//    return ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z'));
}

TransducerAlphabet::TransducerAlphabet(std::istream& is,
                                       SymbolNumber symbol_count,
                                       bool preserve_diacritic_strings)
{
    unknown_symbol = NO_SYMBOL_NUMBER;
    identity_symbol = NO_SYMBOL_NUMBER;
    default_symbol = NO_SYMBOL_NUMBER;
    for(SymbolNumber i=0; i<symbol_count; i++)
    {
        std::string str;
        std::getline(is, str, '\0');
        if(hfst::FdOperation::is_diacritic(str)) {
            fd_table.define_diacritic(i, str);
            if (!preserve_diacritic_strings) {
                str = "";
            }
        } else if (hfst::is_unknown(str)) {
            unknown_symbol = i;
        } else if (hfst::is_default(str)) {
            default_symbol = i;
        } else if (hfst::is_identity(str)) {
            identity_symbol = i;
        }
        if(!is) {
            HFST_THROW(TransducerHasWrongTypeException);
        }
        symbol_table.push_back(str.c_str());
    }
    orig_symbol_count = symbol_table.size();
}

void TransducerAlphabet::add_symbol(char * symbol)
{
    symbol_table.push_back(symbol);
}

void TransducerAlphabet::add_symbol(const std::string & symbol)
{
    symbol_table.push_back(symbol);
}

TransducerAlphabet::TransducerAlphabet(const SymbolTable& st):
    symbol_table(st)
{
    unknown_symbol = NO_SYMBOL_NUMBER;
    default_symbol = NO_SYMBOL_NUMBER;
    identity_symbol = NO_SYMBOL_NUMBER;
    for(SymbolNumber i=0; i<symbol_table.size(); i++)
    {
        if(hfst::FdOperation::is_diacritic(symbol_table[i])) {
            fd_table.define_diacritic(i, symbol_table[i]);
        } else if (hfst::is_unknown(symbol_table[i])) {
            unknown_symbol = i;
        } else if (hfst::is_default(symbol_table[i])) {
            default_symbol = i;
        } else if (hfst::is_identity(symbol_table[i])) {
            identity_symbol = i;
        }
    }
    orig_symbol_count = symbol_table.size();
}

SymbolNumber TransducerAlphabet::symbol_from_string(
    const std::string symbol_string) const
{
    for(SymbolNumber i = 0; i < symbol_table.size(); ++i) {
        if (symbol_table[i] == symbol_string) {
            return i;
        }
    }
    return NO_SYMBOL_NUMBER;
}

StringSymbolMap TransducerAlphabet::build_string_symbol_map(void) const
{
    StringSymbolMap ss_map;
    for(SymbolNumber i = 0; i < symbol_table.size(); ++i) {
        ss_map[symbol_table[i]] = i;
    }
    return ss_map;
}

bool TransducerAlphabet::is_like_epsilon(SymbolNumber symbol) const
{
    if (fd_table.is_diacritic(symbol)) {
        return true;
    }
    if (symbol >= symbol_table.size()) {
        return false;
    }
    std::string s = symbol_table[symbol];
    // Check for Insert symbols like @I.something@ here
    if (s.size() >= 5 && s[0] == '@' && s[1] == 'I' &&
        s[2] == '.' && s[s.size() - 1] == '@') {
        return true;
    }
    return false;
}

void TransducerAlphabet::display() const
{
    std::cout << "Transducer alphabet:" << std::endl;
    for(size_t i=0;i<symbol_table.size();i++)
        std::cout << " Symbol " << i << ": " << symbol_table[i] << std::endl;
}

void TransitionIndex::display() const
{
    std::cout << "input_symbol: " << input_symbol
              << ", target: " << first_transition_index
              << (final()?" (final)":"") << std::endl;
}
void Transition::display() const
{
    std::cout << "input_symbol: " << input_symbol << ", output_symbol: "
              << output_symbol << ", target: " << target_index
              << (final()?" (final)":"") << std::endl;
}
void TransitionW::display() const
{
    std::cout << "input_symbol: " << input_symbol << ", output_symbol: "
              << output_symbol << ", target: " << target_index
              << ", weight: " << transition_weight << (final()?" (final)":"")
              << std::endl;
}

bool TransitionIndex::matches(SymbolNumber s) const
{
    return input_symbol != NO_SYMBOL_NUMBER && input_symbol == s;
}
bool Transition::matches(SymbolNumber s) const
{
    return input_symbol != NO_SYMBOL_NUMBER && input_symbol == s;
}

bool TransitionIndex::final() const
{
    return input_symbol == NO_SYMBOL_NUMBER
        && first_transition_index != NO_TABLE_INDEX;
}
bool Transition::final() const
{
    return input_symbol == NO_SYMBOL_NUMBER
        && output_symbol == NO_SYMBOL_NUMBER && target_index == 1;
}

Weight TransitionWIndex::final_weight(void) const
{
    union to_weight
    {
        TransitionTableIndex i;
        Weight w;
    } weight;
    weight.i = first_transition_index;
    return weight.w;
}

void OlLetterTrie::add_string(const char * p, SymbolNumber symbol_key)
{
    if (*(p+1) == 0)
    {
        symbols[(unsigned char)(*p)] = symbol_key;
        return;
    }
    if (letters[(unsigned char)(*p)] == NULL)
    {
        letters[(unsigned char)(*p)] = new OlLetterTrie();
    }
    letters[(unsigned char)(*p)]->add_string(p+1,symbol_key);
}

bool OlLetterTrie::has_key_starting_with(const char c) const
{
    return letters[(unsigned char) c] != NULL;
}

SymbolNumber OlLetterTrie::find_key(char ** p)
{
    const char * old_p = *p;
    ++(*p);
    if (letters[(unsigned char)(*old_p)] == NULL)
    {
        return symbols[(unsigned char)(*old_p)];
    }
    SymbolNumber s = letters[(unsigned char)(*old_p)]->find_key(p);
    if (s == NO_SYMBOL_NUMBER)
    {
        --(*p);
        return symbols[(unsigned char)(*old_p)];
    }
    return s;
}

void Encoder::read_input_symbols(const SymbolTable & kt)
{
    for (SymbolNumber k = 0; k < number_of_input_symbols; ++k) {
        read_input_symbol(kt[k].c_str(), k);
    }
}

void Encoder::read_input_symbol(const char * s, const int s_num)
{
    if ((strlen(s) == 1) && should_ascii_tokenize((unsigned char)(*s))
        && !letters.has_key_starting_with(*s)) {
        ascii_symbols[(unsigned char)(*s)] = s_num;
    }
    // If there's an ascii tokenized symbol shadowing this, remove it
    if (strlen(s) > 1 && 
        should_ascii_tokenize((unsigned char)(*s)) && 
        ascii_symbols[(unsigned char)(*s)] != NO_SYMBOL_NUMBER) {
      ascii_symbols[(unsigned char)(*s)] = NO_SYMBOL_NUMBER;
    }
    letters.add_string(s, s_num);
}

SymbolNumber Encoder::find_key(char ** p)
{
    if (!should_ascii_tokenize((unsigned char) **p) ||
        ascii_symbols[(unsigned char)(**p)] == NO_SYMBOL_NUMBER)
    {
        return letters.find_key(p);
    }
    SymbolNumber s = ascii_symbols[(unsigned char)(**p)];
    ++(*p);
    return s;
}

bool Transducer::initialize_input(const char * input)
{
    char * input_str = const_cast<char *>(input);
    char ** input_str_ptr = &input_str;
    unsigned int i = 0;
    SymbolNumber k = NO_SYMBOL_NUMBER;
    while(**input_str_ptr != 0) {
        char * original_input_loc = *input_str_ptr;
        k = encoder->find_key(input_str_ptr);
        if (k == NO_SYMBOL_NUMBER) {
            // Add what we assume to be an unknown utf-8 symbol to the alphabet
            *input_str_ptr = original_input_loc;
            int bytes_to_tokenize = nByte_utf8(**input_str_ptr);
            if (bytes_to_tokenize == 0) {
                return false; // tokenization failed
            }
            char * new_symbol = new char[bytes_to_tokenize + 1];
            memcpy(new_symbol, *input_str_ptr, bytes_to_tokenize);
            new_symbol[bytes_to_tokenize] = '\0';
            (*input_str_ptr) += bytes_to_tokenize;
            alphabet->add_symbol(new_symbol);
            k = alphabet->get_symbol_table().size() - 1;
            encoder->read_input_symbol(new_symbol, k);
            delete [] new_symbol;
        }
        input_tape.write(i, k);
        ++i;
    }
    input_tape.write(i, NO_SYMBOL_NUMBER);
    return true;
}

HfstOneLevelPaths * Transducer::lookup_fd(const StringVector & s, ssize_t limit)
{
    std::string input_str;
    for (StringVector::const_iterator it = s.begin(); it != s.end(); ++it) {
        input_str.append(*it);
    }
    return lookup_fd(input_str, limit);
}

HfstOneLevelPaths * Transducer::lookup_fd(const std::string & s, ssize_t limit)
{
    return lookup_fd(s.c_str(), limit);
}

bool Transducer::is_lookup_infinitely_ambiguous(const std::string & s)
{
    if (!initialize_input(s.c_str())) {
        return false;
    }
    traversal_states.clear();
    try {
        find_loop(0, 0);
    } catch (bool e) {
        current_weight = 0.0;
        flag_state = alphabet->get_fd_table();
        return e;
    }
    return false;
}

bool Transducer::is_lookup_infinitely_ambiguous(const StringVector & s)
{
    std::string input_str;
    for (StringVector::const_iterator it = s.begin(); it != s.end(); ++it) {
        input_str.append(*it);
    }
    return is_lookup_infinitely_ambiguous(input_str);
}


HfstOneLevelPaths * Transducer::lookup_fd(const char * s, ssize_t limit)
{
    max_lookups = limit;
    HfstOneLevelPaths * results = new HfstOneLevelPaths;
    lookup_paths = results;
    if (!initialize_input(s)) {
        lookup_paths = NULL;
        return results;
    }
    traversal_states.clear();
    //current_weight += s.second;
    get_analyses(0, 0, 0);
    //current_weight -= s.second;
    lookup_paths = NULL;
    return results;
}

void Transducer::try_epsilon_transitions(unsigned int input_pos,
                                         unsigned int output_pos,
                                         TransitionTableIndex i)
{
    while (true)
    {
        SymbolNumber input = tables->get_transition_input(i);
        SymbolNumber output = tables->get_transition_output(i);
        TransitionTableIndex target = tables->get_transition_target(i);
        Weight weight = tables->get_weight(i);
        if (input == 0) // epsilon
        {
            output_tape.write(output_pos, output);
            current_weight += weight;
            get_analyses(input_pos, output_pos + 1, target);
            found_transition = true;
            current_weight -= weight;
            ++i;
        } else if (alphabet->is_flag_diacritic(input)) {
            FlagDiacriticState flags = flag_state.get_values();
            if (flag_state.apply_operation(
                    *(alphabet->get_operation(input)))) {
                // flag diacritic allowed

                TraversalState flag_reachable(target, flags);
                if (traversal_states.count(flag_reachable) == 1) {
                    // We've been here before at this input, back out
                    flag_state.assign_values(flags);
                    ++i;
                    continue;
                }

                traversal_states.insert(flag_reachable);
                output_tape.write(output_pos, output);
                current_weight += weight;
                get_analyses(input_pos, output_pos + 1, target);
                found_transition = true;
                current_weight -= weight;
                traversal_states.erase(flag_reachable);
            }
            flag_state.assign_values(flags);
            ++i;
        } else { // it's not epsilon and it's not a flag, so nothing to do
            return;
        }
    }
}

void Transducer::try_epsilon_indices(unsigned int input_pos,
                                     unsigned int output_pos,
                                     TransitionTableIndex i)
{
    if (tables->get_index_input(i) == 0)
    {
        try_epsilon_transitions(input_pos,
                                output_pos,
                                tables->get_index_target(i) - 
                                TRANSITION_TARGET_TABLE_START);
        found_transition = true;
    }
}

void Transducer::find_transitions(SymbolNumber input,
                                  unsigned int input_pos,
                                  unsigned int output_pos,
                                  TransitionTableIndex i)
{

    while (tables->get_transition_input(i) != NO_SYMBOL_NUMBER)
    {
        if (tables->get_transition_input(i) == input)
        {
            // We're not going to find an epsilon / flag loop
            traversal_states.clear();
            SymbolNumber output = tables->get_transition_output(i);
            if (output == alphabet->get_default_symbol()
                || output == alphabet->get_identity_symbol()
                || output == alphabet->get_unknown_symbol()) {
                // we got here via default, identity or unknown, so look
                // back in the input tape to find the symbol we want to write
                output = input_tape[input_pos - 1];
            }
            output_tape.write(output_pos, output);
            current_weight += tables->get_weight(i);
            get_analyses(input_pos,
                         output_pos + 1,
                         tables->get_transition_target(i));
            current_weight -= tables->get_weight(i);
            found_transition = true;
        }
        else
        {
            return;
        }
        ++i;
    }
}

void Transducer::find_index(SymbolNumber input,
                            unsigned int input_pos,
                            unsigned int output_pos,
                            TransitionTableIndex i)
{
    if (tables->get_index_input(i+input) == input)
    {
        find_transitions(input,
                         input_pos,
                         output_pos,
                         tables->get_index_target(i+input) - 
                         TRANSITION_TARGET_TABLE_START);
        found_transition = true;
    }
}




void Transducer::get_analyses(unsigned int input_pos,
                              unsigned int output_pos,
                              TransitionTableIndex i)
{
    found_transition = false;
    
    if (recursion_depth_left == 0) {
//        std::cerr << __FILE__ <<
//            ": maximum recursion depth exceeded, discarding results\n";
        return;
    }
    if (max_lookups >= 0 && lookup_paths->size() >= max_lookups) {
        // Back out because we have enough results already
        return;
    }
    --recursion_depth_left;
    if (indexes_transition_table(i))
    {
        i -= TRANSITION_TARGET_TABLE_START;
        // First we check for finality and collect the result
        if (input_tape[input_pos] == NO_SYMBOL_NUMBER) {
            if (max_lookups < 0 || lookup_paths->size() < max_lookups) {
                output_tape.write(output_pos, NO_SYMBOL_NUMBER);
                if (tables->get_transition_finality(i)) {
                    current_weight += tables->get_weight(i);
                    note_analysis();
                    current_weight -= tables->get_weight(i);
                }
            }
        }

        // Then we check epsilons
        try_epsilon_transitions(input_pos,
                                output_pos,
                                i+1);

        if (input_tape[input_pos] == NO_SYMBOL_NUMBER) {
            // No more input
            ++recursion_depth_left;
            return;
        }
        
        SymbolNumber input = input_tape[input_pos];
        ++input_pos;

        if (input < alphabet->get_orig_symbol_count()) {
            // Input is in the alphabet
            find_transitions(input,
                             input_pos,
                             output_pos,
                             i+1);
        } else {
            if (alphabet->get_identity_symbol() != NO_SYMBOL_NUMBER) {
                find_transitions(alphabet->get_identity_symbol(),
                                 input_pos, output_pos, i+1);
            }
            if (alphabet->get_unknown_symbol() != NO_SYMBOL_NUMBER) {
                find_transitions(alphabet->get_unknown_symbol(),
                                 input_pos, output_pos, i+1);
            }
        }
        if (alphabet->get_default_symbol() != NO_SYMBOL_NUMBER &&
            !found_transition) {
            find_transitions(alphabet->get_default_symbol(),
                             input_pos, output_pos, i+1);
        }
    }
    else
    {
        if (input_tape[input_pos] == NO_SYMBOL_NUMBER) {
            if (max_lookups < 0 || lookup_paths->size() < max_lookups) {
                output_tape.write(output_pos, NO_SYMBOL_NUMBER);
                if (tables->get_index_finality(i)) {
                    current_weight += tables->get_final_weight(i);
                    note_analysis();
                    current_weight -= tables->get_final_weight(i);
                }
            }
        }
        
        try_epsilon_indices(input_pos,
                            output_pos,
                            i+1);
        
        if (input_tape[input_pos] == NO_SYMBOL_NUMBER) {
            ++recursion_depth_left;
            return;
        }
      
        SymbolNumber input = input_tape[input_pos];
        ++input_pos;

        if (input < alphabet->get_orig_symbol_count()) {
            // Input is in the alphabet
            find_index(input, input_pos, output_pos, i+1);
        } else {
            if (alphabet->get_identity_symbol() != NO_SYMBOL_NUMBER) {
                find_index(alphabet->get_identity_symbol(),
                           input_pos, output_pos, i+1);
            }
            if (alphabet->get_unknown_symbol() != NO_SYMBOL_NUMBER) {
                find_index(alphabet->get_unknown_symbol(),
                           input_pos, output_pos, i+1);
            }
        }
        // If we have a default symbol defined and we didn't find an index,
        // check for that
        if (alphabet->get_default_symbol() != NO_SYMBOL_NUMBER && !found_transition) {
            find_index(alphabet->get_default_symbol(),
                       input_pos, output_pos, i+1);
        }
    }
    output_tape.write(output_pos, NO_SYMBOL_NUMBER);
    ++recursion_depth_left;
}

void Transducer::note_analysis(void)
{
    HfstOneLevelPath result;
    for (SymbolNumberVector::const_iterator it = output_tape.begin();
         *it != NO_SYMBOL_NUMBER; ++it) {
        result.second.push_back(alphabet->string_from_symbol(*it));
    }
    result.first = current_weight;
    lookup_paths->insert(result);
}



Transducer::Transducer():
    header(NULL), alphabet(NULL), tables(NULL),
    current_weight(0.0), lookup_paths(NULL), encoder(NULL),
    input_tape(), output_tape(),
    flag_state(), found_transition(false), max_lookups(-1),
    recursion_depth_left(MAX_RECURSION_DEPTH){}

Transducer::Transducer(std::istream& is):
    header(new TransducerHeader(is)),
    alphabet(new TransducerAlphabet(is, header->symbol_count())),
    tables(NULL), current_weight(0.0), lookup_paths(NULL),
    encoder(new Encoder(alphabet->get_symbol_table(),
                        header->input_symbol_count())),
    input_tape(), output_tape(),
    flag_state(alphabet->get_fd_table()), found_transition(false), max_lookups(-1),
    recursion_depth_left(MAX_RECURSION_DEPTH)
{
    load_tables(is);
}


Transducer::Transducer(bool weighted):
    header(new TransducerHeader(weighted)),
    alphabet(new TransducerAlphabet()),
    current_weight(0.0),
    lookup_paths(NULL),
    encoder(new Encoder(alphabet->get_symbol_table(),
                        header->input_symbol_count())),
    input_tape(), output_tape(),
    flag_state(alphabet->get_fd_table()), found_transition(false),
    max_lookups(-1), recursion_depth_left(MAX_RECURSION_DEPTH)
{
    if(weighted)
        tables = new TransducerTables<TransitionWIndex,TransitionW>();
    else
        tables = new TransducerTables<TransitionIndex,Transition>();
}

Transducer::Transducer(const TransducerHeader& header,
                       const TransducerAlphabet& alphabet,
                       const TransducerTable<TransitionIndex>& index_table,
                       const TransducerTable<Transition>& transition_table):
    header(new TransducerHeader(header)),
    alphabet(new TransducerAlphabet(alphabet)),
    tables(new TransducerTables<TransitionIndex,Transition>(
               index_table, transition_table)),
    current_weight(0.0),
    lookup_paths(NULL),
    encoder(new Encoder(alphabet.get_symbol_table(),
                        header.input_symbol_count())),
    input_tape(), output_tape(),
    flag_state(alphabet.get_fd_table()), found_transition(false), max_lookups(-1),
    recursion_depth_left(MAX_RECURSION_DEPTH)
{}

Transducer::Transducer(const TransducerHeader& header,
                       const TransducerAlphabet& alphabet,
                       const TransducerTable<TransitionWIndex>& index_table,
                       const TransducerTable<TransitionW>& transition_table):
    header(new TransducerHeader(header)),
    alphabet(new TransducerAlphabet(alphabet)),
    tables(new TransducerTables<TransitionWIndex,TransitionW>(
               index_table, transition_table)),
    current_weight(0.0),
    lookup_paths(NULL),
    encoder(new Encoder(alphabet.get_symbol_table(),
                        header.input_symbol_count())),
    input_tape(), output_tape(),
    flag_state(alphabet.get_fd_table()), found_transition(false), max_lookups(-1),
    recursion_depth_left(MAX_RECURSION_DEPTH)
{}

Transducer::~Transducer()
{
    delete header;
    delete alphabet;
    delete tables;
    delete encoder;
}

TransducerTable<TransitionWIndex> & Transducer::copy_windex_table()
{
    if (!header->probe_flag(Weighted)) {
        HFST_THROW(TransducerHasWrongTypeException);
    }
    TransducerTable<TransitionWIndex> * another =
        new TransducerTable<TransitionWIndex>;
    for (unsigned int i = 0; i < header->index_table_size(); ++i) {
        another->append(TransitionWIndex(tables->get_index_input(i),
                                         tables->get_index_target(i)));
    }
    return *another;
}
TransducerTable<TransitionW> & Transducer::copy_transitionw_table()
{
    if (!header->probe_flag(Weighted)) {
        HFST_THROW(TransducerHasWrongTypeException);
    }
    TransducerTable<TransitionW> * another = new TransducerTable<TransitionW>;
    for (unsigned int i = 0; i < header->target_table_size(); ++i) {
        another->append(TransitionW(tables->get_transition_input(i),
                                    tables->get_transition_output(i),
                                    tables->get_transition_target(i),
                                    tables->get_weight(i)));
    }
    return *another;
}
TransducerTable<TransitionIndex> & Transducer::copy_index_table()
{
    if (header->probe_flag(Weighted)) {
        HFST_THROW(TransducerHasWrongTypeException);
    }
    TransducerTable<TransitionIndex> * another =
        new TransducerTable<TransitionIndex>;
    for (unsigned int i = 0; i < header->index_table_size(); ++i) {
        another->append(tables->get_index(i));
    }
    return *another;
}
TransducerTable<Transition> & Transducer::copy_transition_table()
{
    if (header->probe_flag(Weighted)) {
        HFST_THROW(TransducerHasWrongTypeException);
    }
    TransducerTable<Transition> * another = new TransducerTable<Transition>();
    for (unsigned int i = 0; i < header->target_table_size(); ++i) {
        another->append(tables->get_transition(i));
    }
    return *another;
}


void Transducer::load_tables(std::istream& is)
{
    if(header->probe_flag(Weighted))
        tables = new TransducerTables<TransitionWIndex,TransitionW>(
            is, header->index_table_size(),header->target_table_size());
    else
        tables = new TransducerTables<TransitionIndex,Transition>(
            is, header->index_table_size(),header->target_table_size());
    if(!is) {
        HFST_THROW(TransducerHasWrongTypeException);
    }
}

void Transducer::write(std::ostream& os) const
{
    header->write(os);
    alphabet->write(os);
    for(size_t i=0;i<header->index_table_size();i++)
        tables->get_index(i).write(os, header->probe_flag(Weighted));
    for(size_t i=0;i<header->target_table_size();i++)
        tables->get_transition(i).write(os, header->probe_flag(Weighted));
}

Transducer * Transducer::copy(Transducer * t, bool weighted)
{
    Transducer * another;
    if (weighted) {
        another = new Transducer(
            t->get_header(), t->get_alphabet(),
            t->copy_windex_table(), t->copy_transitionw_table());
    } else {
        another = new Transducer(
            t->get_header(), t->get_alphabet(),
            t->copy_index_table(), t->copy_transition_table());
    }
    return another;
}

void Transducer::display() const
{
    std::cout << "-----Displaying optimized-lookup transducer------"
              << std::endl;
    header->display();
    alphabet->display();
    tables->display();
    std::cout << "-------------------------------------------------"
              << std::endl;
}

TransitionTableIndexSet Transducer::get_transitions_from_state(
    TransitionTableIndex state_index) const
{
    TransitionTableIndexSet transitions;
  
    if(indexes_transition_index_table(state_index)) {
        // for each input symbol that has a transition from this state
        for(SymbolNumber symbol=0; symbol < header->symbol_count(); symbol++) {
            // There may be flags at index 0 even if there aren't
            // any epsilons, so those have to be checked for
            if (alphabet->is_like_epsilon(symbol)) {
                TransitionTableIndex transition_i =
                    get_index(state_index+1).get_target();
                if (!get_index(state_index+1).matches(0)) {
                    continue;
                }
                while(true) {
                    SymbolNumber input = get_transition(transition_i).
                        get_input_symbol();
                    if (get_transition(transition_i).matches(symbol)) {
                        transitions.insert(transition_i);
                        // There could still be epsilons here, or other flags
                    } else if (input != 0 && !alphabet->is_like_epsilon(input)) {
                        break;
                    }
                    ++transition_i;
                }
            } else { // not a flag
                const TransitionIndex& test_transition_index =
                    get_index(state_index+1+symbol);
                if(test_transition_index.matches(symbol)) {
                    // there are one or more transitions with this input symbol,
                    // starting at test_transition_index.get_target()
                    TransitionTableIndex transition_i =
                        test_transition_index.get_target();
                    while(true)
                    {
                        if(get_transition(transition_i).matches(
                               test_transition_index.get_input_symbol())) {
                            transitions.insert(transition_i);
                        } else {
                            break;
                        }
                        ++transition_i;
                    }
                }
            }
        }
    } else { // indexes transition table
        const Transition& transition = get_transition(state_index);
        if(transition.get_input_symbol() != NO_SYMBOL_NUMBER ||
           transition.get_output_symbol() != NO_SYMBOL_NUMBER)
        {
            // Oops
            throw;
        }
    
        TransitionTableIndex transition_i = state_index+1;
        while(true)
        {
            if(get_transition(transition_i)
               .get_input_symbol() != NO_SYMBOL_NUMBER)
                transitions.insert(transition_i);
            else
                break;
            transition_i++;
        }
    }
    return transitions;
}

TransitionTableIndex Transducer::next(const TransitionTableIndex i,
                                      const SymbolNumber symbol) const
{
    if (i >= TRANSITION_TARGET_TABLE_START) {
        return i - TRANSITION_TARGET_TABLE_START + 1;
    } else {
        return get_index(i+1+symbol).get_target() - TRANSITION_TARGET_TABLE_START;
    }
}

bool Transducer::has_transitions(const TransitionTableIndex i,
                                 const SymbolNumber symbol) const
{
    if (i >= TRANSITION_TARGET_TABLE_START) {
        return (get_transition(i - TRANSITION_TARGET_TABLE_START).get_input_symbol() == symbol);
    } else {
        return (get_index(i+symbol).get_input_symbol() == symbol);
    }
}

bool Transducer::has_epsilons_or_flags(const TransitionTableIndex i)
{
    if (i >= TRANSITION_TARGET_TABLE_START) {
        return(get_transition(i - TRANSITION_TARGET_TABLE_START)
               .get_input_symbol() == 0 ||
               is_flag(get_transition(i - TRANSITION_TARGET_TABLE_START)
                       .get_input_symbol()));
    } else {
        return (get_index(i).get_input_symbol() == 0);
    }
}

STransition Transducer::take_epsilons(const TransitionTableIndex i) const
{
    if (get_transition(i).get_input_symbol() != 0) {
        return STransition(0, NO_SYMBOL_NUMBER);
    }
    return STransition(get_transition(i).get_target(),
                       get_transition(i).get_output_symbol(),
                       get_transition(i).get_weight());
}

STransition Transducer::take_epsilons_and_flags(const TransitionTableIndex i)
{
    if (get_transition(i).get_input_symbol() != 0&&
        !is_flag(get_transition(i).get_input_symbol())) {
        return STransition(0, NO_SYMBOL_NUMBER);
    }
    return STransition(get_transition(i).get_target(),
                       get_transition(i).get_output_symbol(),
                       get_transition(i).get_weight());
}

STransition Transducer::take_non_epsilons(const TransitionTableIndex i,
                                          const SymbolNumber symbol) const
{
    if (get_transition(i).get_input_symbol() != symbol) {
        return STransition(0, NO_SYMBOL_NUMBER);
    }
    return STransition(get_transition(i).get_target(),
                       get_transition(i).get_output_symbol(),
                       get_transition(i).get_weight());
}

Weight Transducer::final_weight(const TransitionTableIndex i) const
{
    if (i >= TRANSITION_TARGET_TABLE_START) {
        return get_transition(i - TRANSITION_TARGET_TABLE_START).get_weight();
    } else {
        return get_index(i).final_weight();
    }
}


}

#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
