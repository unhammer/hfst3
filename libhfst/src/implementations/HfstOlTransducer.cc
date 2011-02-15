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

#include <cstring>
#include "HfstOlTransducer.h"

#ifndef DEBUG_MAIN
namespace hfst { namespace implementations
{
  HfstOlInputStream::HfstOlInputStream(bool weighted):
    i_stream(),input_stream(std::cin), weighted(weighted)
  {}
  HfstOlInputStream::HfstOlInputStream
  (const std::string &filename, bool weighted):
    filename(std::string(filename)),i_stream(filename.c_str()),
    input_stream(i_stream),weighted(weighted)
  {}
  
  /* Skip the identifier string "HFST_OL_TYPE" or "HFST_OLW_TYPE" */
  void HfstOlInputStream::skip_identifier_version_3_0(void)
  { input_stream.ignore((weighted?14:13)); }

  void HfstOlInputStream::skip_hfst_header(void)
  {
    input_stream.ignore(6);
    skip_identifier_version_3_0();
  }
  
  void HfstOlInputStream::open(void) {}
  void HfstOlInputStream::close(void)
  {
    if (filename != string())
      { i_stream.close(); }
  }
  bool HfstOlInputStream::is_open(void) const
  { 
    if (filename != string())
      { return i_stream.is_open(); }
    return true;
  }
  bool HfstOlInputStream::is_eof(void) const
  {
    return input_stream.peek() == EOF;
  }
  bool HfstOlInputStream::is_bad(void) const
  {
    if (filename == string())
      { return std::cin.bad(); }
    else
      { return input_stream.bad(); }    
  }
  bool HfstOlInputStream::is_good(void) const
  {
    if(is_eof())
      return false;
    
    if (filename == string())
      { return std::cin.good(); }
    else
      { return input_stream.good(); }
  }
  
  bool HfstOlInputStream::is_fst(void) const
  {
    return (is_fst(input_stream)!=0);
  }
  
  int HfstOlInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    
    char buffer[24];
    size_t num_read = fread(buffer, 24, 1, f);
    unsigned int weighted = *((int*)(buffer+20));
    int res;
    if(num_read != 24)
      { res = 0; }
    else if(weighted == 0)
      { res = 1; }
    else if(weighted == 1)
      { res = 2; }
    else
      { res = 0; }
    
    for(int i=num_read-1;i>=0;i--)
      { ungetc(buffer[i], f); }
    if(num_read != 24)
      { clearerr(f); }
    
    return res;
  }
  
  int HfstOlInputStream::is_fst(istream &s)
  {
    if(!s.good())
      { return 0; }
    
    char buffer[24];
    s.read(buffer, 24);
    size_t num_read = s.gcount();
    unsigned int weighted = *((int*)(buffer+20));
    int res;
    if(num_read != 24)
      { res = 0; }
    else if(weighted == 0)
      { res = 1; }
    else if(weighted == 1)
      { res = 2; }
    else
      { res = 0; }
    
    for(int i=num_read-1;i>=0;i--)
      { s.putback(buffer[i]); }
    if(num_read != 24)
      { s.clear(); }
    
    return res;
  }

  char HfstOlInputStream::stream_get() {
    return (char) input_stream.get(); }

  short HfstOlInputStream::stream_get_short() {
    short i;
    input_stream.read((char*)&i,sizeof(i));
    return i;
  }

  void HfstOlInputStream::stream_unget(char c) {
    input_stream.putback(c); }

void HfstOlInputStream::ignore(unsigned int n)
{
    input_stream.ignore(n);
}

  bool HfstOlInputStream::operator() (void) const
  { return is_good(); }

  hfst_ol::Transducer * HfstOlInputStream::read_transducer(bool has_header)
  {
    if (is_eof())
      { throw StreamIsClosedException(); }
    try 
    {
      if (has_header)
        skip_hfst_header();

      hfst_ol::Transducer* t = new hfst_ol::Transducer(input_stream);
      //t->display();
      return t;
    }
    catch (TransducerHasWrongTypeException e)
    { throw e; }
  }
  
  
  HfstOlOutputStream::HfstOlOutputStream(bool weighted):
    filename(std::string()), output_stream(std::cout), weighted(weighted)
  {}

  HfstOlOutputStream::HfstOlOutputStream(const std::string &str, bool weighted):
    filename(std::string(str)),o_stream(str.c_str(),std::ios::out),
    output_stream(o_stream), weighted(weighted)
  {
    if (!output_stream)
      fprintf(stderr, "HfstOlOutputStream: ERROR: failbit set (3).\n");
  }

  void HfstOlOutputStream::write_transducer(hfst_ol::Transducer * transducer) 
  {
    if (!output_stream)
      fprintf(stderr, "HfstOlOutputStream: ERROR: failbit set (1).\n");
    transducer->write(output_stream);
  }
  
  void HfstOlOutputStream::open(void) {}
  void HfstOlOutputStream::close(void) 
  {
    if (filename != string())
      { o_stream.close(); }
  }

  void HfstOlOutputStream::write(const char &c)
  {
    output_stream.put(char(c));
  }

 
  hfst_ol::Transducer * HfstOlTransducer::create_empty_transducer(bool weighted)
  { return new hfst_ol::Transducer(weighted); }
  
  bool HfstOlTransducer::is_cyclic(hfst_ol::Transducer* t)
  { return t->get_header().probe_flag(hfst_ol::Cyclic); }
  
  static bool extract_strings
  (hfst_ol::Transducer* t, hfst_ol::TransitionTableIndex s,
   std::map<hfst_ol::TransitionTableIndex,unsigned short> all_visitations, 
   std::map<hfst_ol::TransitionTableIndex, unsigned short> path_visitations,
   std::vector<char>& lbuffer, int lpos, std::vector<char>& ubuffer, int upos,
   float weight_sum,
   ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<hfst_ol::SymbolNumber> >* fd_state_stack, 
   bool filter_fd)
  {
    if(cycles >= 0 && path_visitations[s] > cycles)
      return true;
    all_visitations[s]++;
    path_visitations[s]++;
    
    if(lpos > 0 && upos > 0)
    {
      lbuffer[lpos]=0;
      ubuffer[upos]=0;
        
      // check for finality
      bool final=false;
      float final_weight=0.0f;
      if(hfst_ol::indexes_transition_index_table(s))
      {
        if(t->get_index(s).final())
        {
          final = true;
          final_weight = t->get_header().probe_flag(hfst_ol::Weighted) ?
              dynamic_cast<const hfst_ol::TransitionWIndex&>
	    (t->get_index(s)).final_weight() : 0.0f;
        }
      }
      else
      {
        if(t->get_transition(s).final())
        {
          final = true;
          final_weight = t->get_header().probe_flag(hfst_ol::Weighted) ?
              dynamic_cast<const hfst_ol::TransitionW&>
	    (t->get_transition(s)).get_weight() : 0.0f;
        }
      }
      
      
      hfst::WeightedPath<float> path(&lbuffer[0],&ubuffer[0],
				     weight_sum+final_weight);
      hfst::ExtractStringsCb::RetVal ret = callback(path, final);
      if(!ret.continueSearch || !ret.continuePath)
      {
        path_visitations[s]--;
        return ret.continueSearch;
      }
    }
    
    // sort arcs by number of visitations
    hfst_ol::TransitionTableIndexSet transitions 
      = t->get_transitions_from_state(s);
    std::vector<hfst_ol::TransitionTableIndex> sorted_transitions;
    for(hfst_ol::TransitionTableIndexSet::const_iterator it
	  =transitions.begin();it!=transitions.end();it++)
    {
      const hfst_ol::Transition& transition = t->get_transition(*it);
      size_t i;
      for( i=0; i<sorted_transitions.size(); i++ )
        if(all_visitations[transition.get_target()] < 
	   all_visitations[t->get_transition
			   (sorted_transitions[i]).get_target()])
          break;
      sorted_transitions.push_back(hfst_ol::NO_TABLE_INDEX);
      for( size_t k=sorted_transitions.size()-1; k>i; k-- )
        sorted_transitions[k] = sorted_transitions[k-1];
      sorted_transitions[i] = *it;
    }
    
    bool res = true;
    for(size_t i=0; i<sorted_transitions.size() && res == true; i++)
    {
      const hfst_ol::Transition& transition 
	= t->get_transition(sorted_transitions[i]);
      hfst_ol::SymbolNumber input = transition.get_input_symbol();
      hfst_ol::SymbolNumber output = transition.get_output_symbol();
      
      bool added_fd_state = false;
      
      if (fd_state_stack) {
        if(fd_state_stack->back().get_table().get_operation(input) != NULL) {
          fd_state_stack->push_back(fd_state_stack->back());
          if(fd_state_stack->back().apply_operation(input))
            added_fd_state = true;
          else {
            fd_state_stack->pop_back();
            continue; // don't follow the transition
          }
        }
      }
      
      int lp=lpos;
      int up=upos;
      
      if(input != 0 && (!filter_fd || 
			fd_state_stack->back().get_table().
			get_operation(input)==NULL))
      {
        std::string str = t->get_alphabet().get_symbol_table()[input];
        if(lpos+str.length() >= lbuffer.size())
          lbuffer.resize(lbuffer.size()*2, 0);
        strcpy(&lbuffer[lpos], str.c_str());
        lp += str.length();
      }
      if(output != 0 && (!filter_fd || 
			 fd_state_stack->back().get_table()
			 .get_operation(output)==NULL))
      {
        std::string str = t->get_alphabet().get_symbol_table()[output];
        if(upos+str.length() > ubuffer.size())
          ubuffer.resize(ubuffer.size()*2, 0);
        strcpy(&ubuffer[upos], str.c_str());
        up += str.length();
      }
      
      res = extract_strings
	(t, transition.get_target(), all_visitations, path_visitations,
	 lbuffer,lp, ubuffer,up, 
	 weight_sum + (t->get_header().probe_flag(hfst_ol::Weighted) ? 
		       dynamic_cast<const hfst_ol::TransitionW&>(transition)
		       .get_weight() : 0.0f),
	 callback, cycles, fd_state_stack, filter_fd);
      
      if(added_fd_state)
        fd_state_stack->pop_back();
    }
    
    path_visitations[s]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
  void HfstOlTransducer::extract_strings
  (hfst_ol::Transducer * t, hfst::ExtractStringsCb& callback,
   int cycles, const FdTable<hfst_ol::SymbolNumber>* fd, bool filter_fd)
  {
    std::vector<char> lbuffer(BUFFER_START_SIZE, 0);
    std::vector<char> ubuffer(BUFFER_START_SIZE, 0);
    std::map<hfst_ol::TransitionTableIndex, unsigned short> all_visitations;
    std::map<hfst_ol::TransitionTableIndex, unsigned short> path_visitations;
    std::vector<hfst::FdState<hfst_ol::SymbolNumber> >* fd_state_stack 
      = (fd==NULL) ? NULL : 
      new std::vector<hfst::FdState<hfst_ol::SymbolNumber> >
      (1, hfst::FdState<hfst_ol::SymbolNumber>(*fd));
    
    hfst::implementations::extract_strings
      (t,0,all_visitations,path_visitations,lbuffer,0,ubuffer,0,0.0f,
       callback,cycles,fd_state_stack,filter_fd);
  }
  
  const FdTable<hfst_ol::SymbolNumber>* HfstOlTransducer::
  get_flag_diacritics(hfst_ol::Transducer* t)
  {
    return &(t->get_alphabet().get_fd_table());
  }

} }

#else
#include <cstdlib>
#include <cassert>
#include <iostream>
using namespace hfst::implementations;

int main(int argc, char * argv[]) 
{
    std::cout << "Unit tests for " __FILE__ ":";
    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif

