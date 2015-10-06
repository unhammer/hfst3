%module libhfst
%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"
%include "std_set.i"
%include "std_map.i"
%include "exception.i"

%feature("autodoc", "3");

%{
#define HFSTIMPORT
#include "HfstDataTypes.h"
#include "HfstTransducer.h"
#include "HfstOutputStream.h"
#include "HfstInputStream.h"
#include "HfstExceptionDefs.h"
#include "HfstTokenizer.h"
#include "HfstFlagDiacritics.h"
//#include "HfstRules.h" ???
#include "parsers/XreCompiler.h"
#include "parsers/LexcCompiler.h"
#include "parsers/PmatchCompiler.h"
#include "implementations/HfstTransitionGraph.h"

// todo instead: #include "hfst_extensions.h"

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
      bool is_eof(void);
  };

  HfstFile::HfstFile(): file(NULL){};
  HfstFile::~HfstFile() {};
  void HfstFile::set_file(FILE * f) { file = f; };
  FILE * HfstFile::get_file() { return file; };
  void HfstFile::close() { if (file != stdout && file != stderr && file != stdin) { fclose(file); } };
  void HfstFile::write(const char * str) { fprintf(file, "%s", str); };
  bool HfstFile::is_eof(void) { return (feof(file) != 0); }; 

  HfstFile hfst_open(const char * filename, const char * mode) {
    FILE * f = fopen(filename, mode);
    HfstFile file;
    file.set_file(f);
    return file;
  };

  HfstFile hfst_stdin() {
    HfstFile file;
    file.set_file(stdin);
    return file;
  };

  HfstFile hfst_stdout() {
    HfstFile file;
    file.set_file(stdout);
    return file;
  };

typedef std::vector<float> FloatVector;

hfst::ImplementationType type = hfst::TROPICAL_OPENFST_TYPE;

hfst::HfstTokenizer deftok; // default tokenizer

void set_default_fst_type(hfst::ImplementationType t)
{
        type = t;
}

hfst::ImplementationType get_default_fst_type()
{
        return type;
}

std::string fst_type_to_string(hfst::ImplementationType t)
{
        std::string retval = hfst::implementation_type_to_string(t);
        return retval;
}

bool is_diacritic(const std::string & symbol)
{
        return hfst::FdOperation::is_diacritic(symbol);
}
        
hfst::HfstTransducer fst(const std::string & symbol)
{
        return hfst::HfstTransducer(symbol, type);
}

hfst::HfstTransducer fst(const std::string & isymbol, const std::string & osymbol)
{
        return hfst::HfstTransducer(isymbol, osymbol, type);
}

hfst::HfstTransducer * regex(const std::string & regex_string)
{
        hfst::xre::XreCompiler comp(type);
        return comp.compile(regex_string);
}

hfst::HfstTransducer word(const std::string & w, float weight=0)
{
        HfstTransducer retval = hfst::HfstTransducer(w, deftok, type);
        retval.set_final_weights(weight);
        return retval;
}

hfst::HfstTransducer word_pair(const std::string & wi, const std::string & wo, float weight=0)
{
        HfstTransducer retval = hfst::HfstTransducer(wi, wo, deftok, type);
        retval.set_final_weights(weight);
        return retval;
}

hfst::HfstTransducer word_list_hfst(const StringVector & wl, const FloatVector & weights)
{
        hfst::implementations::HfstBasicTransducer retval;
        unsigned int i=0;
        for (std::vector<std::string>::const_iterator it = wl.begin();
                it != wl.end(); it++)
                {
                        retval.disjunct(deftok.tokenize(*it), weights[i]);
                        i++;
                }
        return hfst::HfstTransducer(retval, type);
}

hfst::HfstTransducer word_pair_list_hfst(const StringPairVector & wpl, const FloatVector & weights)
{
        hfst::implementations::HfstBasicTransducer retval;
        unsigned int i=0;
        for (std::vector<std::pair<std::string, std::string> >::const_iterator it = wpl.begin();
                it != wpl.end(); it++)
                {
                        retval.disjunct(deftok.tokenize(it->first, it->second), weights[i]);
                        i++;
                }
        return hfst::HfstTransducer(retval, type);
}

hfst::HfstOutputStream * create_hfst_output_stream(const std::string & filename, hfst::ImplementationType type, bool hfst_format)
{
        if (filename == "") { return new hfst::HfstOutputStream(type, hfst_format); }
        else { return new hfst::HfstOutputStream(filename, type, hfst_format); }
}

hfst::HfstTransducer * compile_lexc_file(const std::string & filename) /* throw ... */
{
    hfst::lexc::LexcCompiler comp(type /*, withFlags?*/);
    comp.setVerbosity(0);
    comp.parse(filename.c_str());
    return comp.compileLexical();
}

hfst::HfstTransducer * read_att(hfst::HfstFile & f, std::string epsilon="@_EPSILON_SYMBOL_@")
{
    return new HfstTransducer(f.get_file(), type, epsilon);
}

hfst::HfstTransducer * read_prolog(hfst::HfstFile & f)
{
    unsigned int linecount = 0;
    hfst::implementations::HfstBasicTransducer fsm = hfst::implementations::HfstBasicTransducer::read_in_prolog_format(f.get_file(), linecount);
    return new hfst::HfstTransducer(fsm, type);
}

std::string one_level_paths_to_string(const hfst::HfstOneLevelPaths & paths)
{
    std::ostringstream oss;
    for(hfst::HfstOneLevelPaths::const_iterator it = paths.begin(); it != paths.end(); it++)
    {   
      for (hfst::StringVector::const_iterator svit = it->second.begin(); svit != it->second.end(); svit++)
      {
        oss << *svit;
      }
      oss << "\t" << it->first << std::endl;
    }
    return oss.str();
}

std::string two_level_paths_to_string(const hfst::HfstTwoLevelPaths & paths)
{
    std::ostringstream oss;
    for(hfst::HfstTwoLevelPaths::const_iterator it = paths.begin(); it != paths.end(); it++)
    {   
      std::string input("");
      std::string output("");
      for (hfst::StringPairVector::const_iterator svit = it->second.begin(); svit != it->second.end(); svit++)
      {
        input += svit->first;
        output += svit->second;
      }
      oss << input << ":" << output << "\t" << it->first << std::endl;
    }
    return oss.str();
}


}

%}

#ifdef _MSC_VER
%include <windows.h>
#endif

%include "typemaps.i"

namespace std {
%template(StringVector) vector<string>;
%template(StringPair) pair<string, string>;
%template(StringPairVector) vector<pair<string, string > >;
%template(FloatVector) vector<float>;
%template(StringSet) set<string>;
%template(StringPairSet) set<pair<string, string> >;
%template(HfstTransducerVector) vector<hfst::HfstTransducer>;
%template(HfstSymbolSubstitutions) map<string, string>;
%template(HfstSymbolPairSubstitutions) map<pair<string, string>, pair<string, string> >;
%template(FooBarBaz) vector<hfst::implementations::HfstBasicTransition>;
%template(BarBazFoo) vector<unsigned int>;
%template(HfstBasicStates) vector<vector<hfst::implementations::HfstBasicTransition> >;
%template(HfstOneLevelPath) pair<float, vector<string> >;
%template(HfstOneLevelPaths) set<pair<float, vector<string> > >;
%template(HfstTwoLevelPath) pair<float, vector<pair<string, string > > >;
%template(HfstTwoLevelPaths) set<pair<float, vector<pair<string, string > > > >;
}

//%ignore hfst::HfstTransducer::lookup_fd(const std::string & s) const;

class HfstException
{
public:
 HfstException();
 HfstException(const std::string&, const std::string&, size_t);
 ~HfstException();
};

class HfstTransducerTypeMismatchException : public HfstException { public: HfstTransducerTypeMismatchException(const std::string&, const std::string&, size_t); ~HfstTransducerTypeMismatchException(); };
class ImplementationTypeNotAvailableException : public HfstException { public: ImplementationTypeNotAvailableException(const std::string&, const std::string&, size_t); ~ImplementationTypeNotAvailableException(); };
class FunctionNotImplementedException : public HfstException { public: FunctionNotImplementedException(const std::string&, const std::string&, size_t); ~FunctionNotImplementedException(); };
class StreamNotReadableException : public HfstException { public: StreamNotReadableException(const std::string&, const std::string&, size_t); ~StreamNotReadableException(); };
class StreamCannotBeWrittenException : public HfstException { public: StreamCannotBeWrittenException(const std::string&, const std::string&, size_t); ~StreamCannotBeWrittenException(); };
class StreamIsClosedException : public HfstException { public: StreamIsClosedException(const std::string&, const std::string&, size_t); ~StreamIsClosedException(); };
class EndOfStreamException : public HfstException { public: EndOfStreamException(const std::string&, const std::string&, size_t); ~EndOfStreamException(); };
class TransducerIsCyclicException : public HfstException { public: TransducerIsCyclicException(const std::string&, const std::string&, size_t); ~TransducerIsCyclicException(); };
class NotTransducerStreamException : public HfstException { public: NotTransducerStreamException(const std::string&, const std::string&, size_t); ~NotTransducerStreamException(); };
class NotValidAttFormatException : public HfstException { public: NotValidAttFormatException(const std::string&, const std::string&, size_t); ~NotValidAttFormatException(); };
class NotValidPrologFormatException : public HfstException { public: NotValidPrologFormatException(const std::string&, const std::string&, size_t); ~NotValidPrologFormatException(); };
class NotValidLexcFormatException : public HfstException { public: NotValidLexcFormatException(const std::string&, const std::string&, size_t); ~NotValidLexcFormatException(); };
class StateIsNotFinalException : public HfstException { public: StateIsNotFinalException(const std::string&, const std::string&, size_t); ~StateIsNotFinalException(); };
class ContextTransducersAreNotAutomataException : public HfstException { public: ContextTransducersAreNotAutomataException(const std::string&, const std::string&, size_t); ~ContextTransducersAreNotAutomataException(); };
class TransducersAreNotAutomataException : public HfstException { public: TransducersAreNotAutomataException(const std::string&, const std::string&, size_t); ~TransducersAreNotAutomataException(); };
class StateIndexOutOfBoundsException : public HfstException { public: StateIndexOutOfBoundsException(const std::string&, const std::string&, size_t); ~StateIndexOutOfBoundsException(); };
class TransducerHeaderException : public HfstException { public: TransducerHeaderException(const std::string&, const std::string&, size_t); ~TransducerHeaderException(); };
class MissingOpenFstInputSymbolTableException : public HfstException { public: MissingOpenFstInputSymbolTableException(const std::string&, const std::string&, size_t); ~MissingOpenFstInputSymbolTableException(); };
class TransducerTypeMismatchException : public HfstException { public: TransducerTypeMismatchException(const std::string&, const std::string&, size_t); ~TransducerTypeMismatchException(); };
class EmptySetOfContextsException : public HfstException { public: EmptySetOfContextsException(const std::string&, const std::string&, size_t); ~EmptySetOfContextsException(); };
class SpecifiedTypeRequiredException : public HfstException { public: SpecifiedTypeRequiredException(const std::string&, const std::string&, size_t); ~SpecifiedTypeRequiredException(); };
class HfstFatalException : public HfstException { public: HfstFatalException(const std::string&, const std::string&, size_t); ~HfstFatalException(); };
class TransducerHasWrongTypeException : public HfstException { public: TransducerHasWrongTypeException(const std::string&, const std::string&, size_t); ~TransducerHasWrongTypeException(); };
class IncorrectUtf8CodingException : public HfstException { public: IncorrectUtf8CodingException(const std::string&, const std::string&, size_t); ~IncorrectUtf8CodingException(); };
class EmptyStringException : public HfstException { public: EmptyStringException(const std::string&, const std::string&, size_t); ~EmptyStringException(); };
class SymbolNotFoundException : public HfstException { public: SymbolNotFoundException(const std::string&, const std::string&, size_t); ~SymbolNotFoundException(); };
class MetadataException : public HfstException { public: MetadataException(const std::string&, const std::string&, size_t); ~MetadataException(); };
class FlagDiacriticsAreNotIdentitiesException : public HfstException { public: FlagDiacriticsAreNotIdentitiesException(const std::string&, const std::string&, size_t); ~FlagDiacriticsAreNotIdentitiesException(); };

namespace hfst
{

class HfstFile {
  public:
    HfstFile();
    ~HfstFile();
    void write(const char * str);
    void close();
    bool is_eof(void);
};

HfstFile hfst_stdout();
HfstFile hfst_stdin();
HfstFile hfst_open(const char * filename, const char * mode);

typedef std::vector<std::string> StringVector;
typedef std::pair<std::string, std::string> StringPair;
typedef std::vector<std::pair<std::string, std::string> > StringPairVector;
typedef std::vector<float> FloatVector;
typedef std::set<std::string> StringSet;
typedef std::set<std::pair<std::string, std::string> > StringPairSet;
typedef std::pair<float, std::vector<std::string> > HfstOneLevelPath;
typedef std::set<std::pair<float, std::vector<std::string> > > HfstOneLevelPaths;
typedef std::pair<float, std::vector<std::pair<std::string, std::string > > > HfstTwoLevelPath;
typedef std::set<std::pair<float, std::vector<std::pair<std::string, std::string> > > > HfstTwoLevelPaths;
typedef std::map<std::string, std::string> HfstSymbolSubstitutions;
typedef std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > HfstSymbolPairSubstitutions;

enum ImplementationType
{
    SFST_TYPE,
    TROPICAL_OPENFST_TYPE,
    LOG_OPENFST_TYPE,
    FOMA_TYPE,
    XFSM_TYPE,
    HFST_OL_TYPE,
    HFST_OLW_TYPE,
    HFST2_TYPE,
    UNSPECIFIED_TYPE,
    ERROR_TYPE
};

enum PushType { TO_INITIAL_STATE, TO_FINAL_STATE };

/*
%typemap(out) HfstOneLevelPaths* {
        $result = PyList_New((*$1).size());
        unsigned int i = 0;
        for (hfst::HfstOneLevelPaths::const_iterator it = (*$1).begin(); it != (*$1).end(); it++)
        {
                std::string result_string("");
                for (hfst::StringVector::const_iterator svit = it->second.begin(); svit != it->second.end(); svit++)
                {
                        result_string += *svit;
                }
                PyObject * res = PyTuple_New(2);
                PyTuple_SetItem(res, 0, PyString_FromString(result_string.c_str()));
                PyTuple_SetItem(res, 1, PyFloat_FromDouble(it->first));
                PyList_SetItem($result, i, res);
                i++;
        }
}
*/

/* instead, run:   
      sed -i 's/class HfstException(_object):/class HfstException(Exception):/' libhfst.py
   after build to make HfstException and its subclasses subclasses of Python's Exception.

%typemap(throws) HfstTransducerTypeMismatchException %{ PyErr_SetString(PyExc_RuntimeError, "HfstTransducerTypeMismatchException"); SWIG_fail; %}
... etc for all exception classes
*/

bool is_diacritic(const std::string & symbol);

%pythoncode{
  def is_string(s):
      if isinstance(s, str):
         return True
      else:
        return False   
  def is_string_pair(sp):
      if not isinstance(sp, tuple):
         return False
      if len(sp) != 2:
         return False
      if not is_string(sp[0]):
         return False
      if not is_string(sp[1]):
         return False
      return True
  def is_string_vector(sv):
      if not isinstance(sv, tuple):
         return False
      for s in sv:
          if not is_string(s):
             return False
      return True
  def is_string_pair_vector(spv):
      if not isinstance(spv, tuple):
         return False
      for sp in spv:
          if not is_string_pair(sp):
             return False
      return True

  def two_level_paths_to_dict(tlps):
      retval = {}
      for tlp in tlps:
          input = ""
          output = ""
          for sp in tlp[1]:
              input = input + sp[0]
              output = output + sp[1]
          if input in retval:
              retval[input] = (retval[input], (output, tlp[0]))
          else:
              retval[input] = (output, tlp[0])
      return retval
%}

class HfstTransducer 
{
public: 
HfstTransducer(ImplementationType);
//HfstTransducer(const std::string &, const std::string &, ImplementationType);
//HfstTransducer(hfst::HfstInputStream & istr) throw(EndOfStreamException);
~HfstTransducer();

/* Basic binary operations */
HfstTransducer & concatenate(const HfstTransducer&, bool harmonize=true);
HfstTransducer & disjunct(const HfstTransducer&, bool harmonize=true);
HfstTransducer & subtract(const HfstTransducer&, bool harmonize=true);
HfstTransducer & intersect(const HfstTransducer&, bool harmonize=true);
HfstTransducer & compose(const HfstTransducer&, bool harmonize=true);

/* More binary operations */
HfstTransducer & compose_intersect(const hfst::HfstTransducerVector &v, bool invert=false, bool harmonize=true);
HfstTransducer & priority_union(const HfstTransducer &another, bool harmonize=true, bool encode_epsilons=true);
HfstTransducer & lenient_composition(const HfstTransducer &another, bool harmonize=true);
HfstTransducer & cross_product(const HfstTransducer &another, bool harmonize=true);
HfstTransducer & shuffle(const HfstTransducer &another, bool harmonize=true);

/* Testing */
bool compare(const HfstTransducer&, bool harmonize=true) const;
unsigned int number_of_states() const;
unsigned int number_of_arcs() const;
StringSet get_alphabet() const;
bool is_cyclic() const;
bool is_automaton() const;
bool is_infinitely_ambiguous() const;
bool has_flag_diacritics() const;

/* Optimization */
HfstTransducer & remove_epsilons();
HfstTransducer & determinize();
HfstTransducer & minimize();
HfstTransducer & prune();
HfstTransducer & eliminate_flags();
HfstTransducer & eliminate_flag(const std::string&);
HfstTransducer & n_best(unsigned int n);
HfstTransducer & convert(ImplementationType impl);

/* Repeat */
HfstTransducer & repeat_star();
HfstTransducer & repeat_plus();
HfstTransducer & repeat_n(unsigned int);
HfstTransducer & repeat_n_to_k(unsigned int, unsigned int);
HfstTransducer & repeat_n_minus(unsigned int);
HfstTransducer & repeat_n_plus(unsigned int);

/* Other basic operations */
HfstTransducer & invert();
HfstTransducer & reverse();
HfstTransducer & input_project();
HfstTransducer & output_project();
HfstTransducer & optionalize();

/* Insert freely, substitute */

HfstTransducer & insert_freely(const StringPair &symbol_pair, bool harmonize=true);
HfstTransducer & insert_freely(const HfstTransducer &tr, bool harmonize=true);

//HfstTransducer & substitute(bool (*func)(const StringPair &sp, hfst::StringPairSet &sps));

HfstTransducer & substitute_symbol(const std::string &old_symbol, const std::string &new_symbol, bool input_side=true, bool output_side=true);
HfstTransducer & substitute_symbol_pair(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair);
HfstTransducer & substitute_symbol_pair_with_set(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set);
HfstTransducer & substitute_symbol_pair_with_transducer(const StringPair &symbol_pair, HfstTransducer &transducer, bool harmonize=true);
HfstTransducer & substitute_symbols(const hfst::HfstSymbolSubstitutions &substitutions); // alias for the previous function which is shadowed
HfstTransducer & substitute_symbol_pairs(const hfst::HfstSymbolPairSubstitutions &substitutions); // alias for the previous function which is shadowed


/* Weight handling */
HfstTransducer & set_final_weights(float weight, bool increment=false);
// Can 'transform_weights' be wrapped?  It maybe needs to be rewritten in python.
HfstTransducer & push_weights(hfst::PushType type);

// TODO:
void extract_shortest_paths(HfstTwoLevelPaths &results) const;
bool extract_longest_paths(HfstTwoLevelPaths &results, bool obey_flags=true) const;
int longest_path_size(bool obey_flags=true) const;


%extend {
    char *__str__() {
         static char tmp[1024];
         $self->write_in_att_format(tmp);
         return tmp;
    }
    HfstTransducer & write(hfst::HfstOutputStream & os) {
         (void) os.redirect(*$self);
         return *$self;
    }
    // 'union' is a reserved word in python, so it cannot be used as an alias for function 'disjunct' 
    HfstTransducer & minus(const HfstTransducer& t, bool harmonize=true) { return $self->subtract(t, harmonize); }
    HfstTransducer & conjunct(const HfstTransducer& t, bool harmonize=true) { return $self->intersect(t, harmonize); }

    void write_att(hfst::HfstFile & f, bool write_weights=true)
    {
      $self->write_in_att_format(f.get_file(), write_weights);
    }

    void write_prolog(hfst::HfstFile & f, const std::string & name, bool write_weights=true)
    {
      $self->write_in_prolog_format(f.get_file(), name, write_weights);
    }

    hfst::HfstTwoLevelPaths extract_paths_(int max_num=-1, int cycles=-1) const
    {
      hfst::HfstTwoLevelPaths results;
      $self->extract_paths(results, max_num, cycles);
      return results;
    }

    hfst::HfstTwoLevelPaths extract_paths_fd_(int max_num=-1, int cycles=-1, bool filter_fd=true) const
    {
      hfst::HfstTwoLevelPaths results;
      $self->extract_paths_fd(results, max_num, cycles, filter_fd);
      return results;
    }

    hfst::HfstTwoLevelPaths extract_random_paths_(int max_num) const
    {
      hfst::HfstTwoLevelPaths results;
      $self->extract_random_paths(results, max_num);
      return results;
    }

    hfst::HfstTwoLevelPaths extract_random_paths_fd_(int max_num, bool filter_fd) const
    {
      hfst::HfstTwoLevelPaths results;
      $self->extract_random_paths_fd(results, max_num, filter_fd);
      return results;
    }

// Wrappers for lookup functions

HfstOneLevelPaths lookup_fd_vector(const StringVector& s, int limit = -1 /*ignored?*/ ) const
{ return *($self->lookup_fd(s, limit)); }
HfstOneLevelPaths lookup_fd_string(const std::string& s, int limit /*=-1*/ ) const
{ return *($self->lookup_fd(s, limit)); }
HfstOneLevelPaths lookup_vector(const StringVector& s, int limit = -1) const
{ return *($self->lookup(s, limit)); }
HfstOneLevelPaths lookup_string(const std::string & s, int limit = -1) const
{ return *($self->lookup(s, limit)); }


%pythoncode{

  def lookup(self, input, **kvargs):
      
      obey_flags=True
      # filter_flags=True
      limit=-1
      simple_output=True

      for k,v in kvargs.items():
          if k == 'obey_flags':
             if v == 'True':
                pass
             elif v == 'False':
                obey_flags=False
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'True' and 'False'.")
          elif k == 'simple_output':
             if v == 'True':
                pass
             elif v == 'False':
                simple_output=False
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'True' and 'False'.")
          elif k == 'limit' :
             limit=v
          else:
             print('Warning: ignoring unknown argument %s.' % (k))

      retval=0

      if isinstance(input, tuple):
         if obey_flags:
            retval=self.lookup_fd_vector(input, limit)
         else:
            retval=self.lookup_vector(input, limit)
      elif isinstance(input, str):
         if obey_flags:
            retval=self.lookup_fd_string(input, limit)
         else:
            retval=self.lookup_string(input, limit)
      else:
         raise RuntimeError('Input argument must be string or tuple.')

      if simple_output:
         return one_level_paths_to_string(retval)
      else:
         return retval   

  def extract_paths(self, **kvargs):

      obey_flags=True
      filter_flags=True
      max_cycles=-1
      max_number=-1
      random=False
      output='dict' # 'dict' (default), 'text', 'raw'

      for k,v in kvargs.items():
          if k == 'obey_flags' :
             if v == 'True':
                pass
             elif v == 'False':
                obey_flags=False
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'True' and 'False'.")
          elif k == 'filter_flags' :
             if v == 'True':
                pass
             elif v == 'False':
                filter_flags=False
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'True' and 'False'.")
          elif k == 'max_cycles' :
             max_cycles=v
          elif k == 'max_number' :
             max_number=v
          elif k == 'random' :
             if v == 'False':
                pass
             elif v == 'True':
                random=True
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'True' and 'False'.")
          elif k == 'output':
             if v == 'text':
                pass
             elif v == 'raw':
                output='raw'
             elif v == 'dict':
                output='dict'
             else:
                print('Warning: ignoring argument %s as it has value %s.' % (k, v))
                print("Possible values are 'dict' (default), 'text', 'raw'.")
          else:
             print('Warning: ignoring unknown argument %s.' % (k))

      retval=0

      if obey_flags :
         if random :
            retval=self.extract_random_paths_fd_(max_number, filter_flags)
         else :
            retval=self.extract_paths_fd_(max_number, max_cycles)
      else :
         if random :
            retval=self.extract_random_paths_(max_number)  
         else :   
            retval=self.extract_paths_(max_number, max_cycles)

      if output == 'text':
         return two_level_paths_to_string(retval)
      elif output == 'dict':
         return two_level_paths_to_dict(retval)
      else:
         return retval

  def substitute(self, s, S=None, **kvargs):

      if S == None:
         if not isinstance(s, dict):
            raise RuntimeError('Sole input argument must be a dictionary.')

         subst_type=""

         for k, v in s.items():
             if is_string(k):
                if subst_type == "":
                   subst_type="string"
                elif subst_type == "string pair":
                   raise RuntimeError('')
                if not is_string(v):
                   raise RuntimeError('')
             elif is_string_pair(k):
                if subst_type == "":
                   subst_type="string pair"
                elif subst_type == "string":
                   raise RuntimeError('')
                if not is_string_pair(v):
                   raise RuntimeError('')
             else:
                raise RuntimeError('')

         if subst_type == "string":
            return self.substitute_symbols(s)
         else:
            return self.substitute_symbol_pairs(s)

      if is_string(s):
         if is_string(S):
            input=True
            output=True
            for k,v in kvargs.items():
                if k == 'input':
                   if v == False:
                      input=False
                elif k == 'output':
                   if v == False:
                      output=False
                else:
                   raise RuntimeError('Free argument not recognized.')
            return self.substitute_symbol(s, S, input, output)
         else:
            raise RuntimeError('...')
      elif is_string_pair(s):
         if is_string_pair(S):
            return self.substitute_symbol_pair(s, S)
         elif is_string_pair_vector(S):
            return self.substitute_symbol_pair_with_set(s, S)
         elif isinstance(S, HfstTransducer):
            return self.substitute_symbol_pair_with_transducer(s, S, True)
         else:
            raise RuntimeError('...')
      else:
         raise RuntimeError('...')
}

};

};

hfst::HfstOutputStream * create_hfst_output_stream(const std::string & filename, hfst::ImplementationType type, bool hfst_format);

class HfstOutputStream
{
public:
//HfstOutputStream(ImplementationType type, bool hfst_format=true);
//HfstOutputStream(const std::string &filename, ImplementationType type, bool hfst_format=true);
~HfstOutputStream(void);
HfstOutputStream &flush();
//HfstOutputStream &operator<< (HfstTransducer &transducer);
//HfstOutputStream& redirect (HfstTransducer &transducer);
void close(void);

%extend {

HfstOutputStream & write(hfst::HfstTransducer & transducer)
{
return $self->redirect(transducer);
}

HfstOutputStream() { return new hfst::HfstOutputStream(hfst::get_default_fst_type()); }

%pythoncode {

def __init__(self, **kvargs):
    filename = ""
    hfst_format = True
    type = _libhfst.get_default_fst_type()
    for k,v in kvargs.items():
        if k == 'filename':
           filename = v
        if k == 'hfst_format':
           hfst_format = v
        if k == 'type':
           type = v
    if filename == "":
       self.this = _libhfst.create_hfst_output_stream("", type, hfst_format)
    else:
       self.this = _libhfst.create_hfst_output_stream(filename, type, hfst_format)
}

}

};

class HfstInputStream
{
public:
    HfstInputStream(void) throw(StreamNotReadableException, NotTransducerStreamException, EndOfStreamException, TransducerHeaderException);
    HfstInputStream(const std::string &filename) throw(StreamNotReadableException, NotTransducerStreamException, EndOfStreamException, TransducerHeaderException);
    ~HfstInputStream(void);
    void close(void);
    bool is_eof(void);
    bool is_bad(void);
    bool is_good(void);
    ImplementationType get_type(void) const throw(TransducerTypeMismatchException);

%extend {
hfst::HfstTransducer * read() throw (EndOfStreamException)
{
  return new hfst::HfstTransducer(*($self));
}
}

};

  /* class MultiCharSymbolTrie;
  typedef std::vector<MultiCharSymbolTrie*> MultiCharSymbolTrieVector;
  typedef std::vector<bool> SymbolEndVector;

  class MultiCharSymbolTrie
  {
  public:
    MultiCharSymbolTrie(void);
    ~MultiCharSymbolTrie(void);
    void add(const char * p);
    const char * find(const char * p) const;  
  }; */
  
  class HfstTokenizer
  {    
  public:
     HfstTokenizer();
     void add_skip_symbol(const std::string &symbol);
     void add_multichar_symbol(const std::string& symbol);
     StringPairVector tokenize(const std::string &input_string) const;
     StringVector tokenize_one_level(const std::string &input_string) const;
     static StringPairVector tokenize_space_separated(const std::string & str);
     StringPairVector tokenize(const std::string &input_string,
                              const std::string &output_string) const;
     //StringPairVector tokenize(const std::string &input_string,
     //                         const std::string &output_string,
     //                         void (*warn_about_pair)(const std::pair<std::string, std::string> &symbol_pair)) const;
     //StringPairVector tokenize_and_align_flag_diacritics
     // (const std::string &input_string,
     //  const std::string &output_string,
     //  void (*warn_about_pair)(const std::pair<std::string, std::string> &symbol_pair)) const;
     static void check_utf8_correctness(const std::string &input_string);
  };

namespace implementations {

  class HfstBasicTransducer;
  class HfstBasicTransition;
  typedef unsigned int HfstState;

  typedef std::vector<std::vector<hfst::implementations::HfstBasicTransition> > HfstBasicStates;

class HfstBasicTransducer {

  public:

    typedef std::vector<HfstBasicTransition> HfstTransitions;

    HfstBasicTransducer(void);
    //HfstBasicTransducer(FILE *file);
    //HfstBasicTransducer &assign(const HfstBasicTransducer &graph);
    HfstBasicTransducer(const HfstBasicTransducer &graph);
    HfstBasicTransducer(const hfst::HfstTransducer &transducer);

    void add_symbol_to_alphabet(const std::string &symbol);
    void remove_symbol_from_alphabet(const std::string &symbol);
    void remove_symbols_from_alphabet(const StringSet &symbols);
    void add_symbols_to_alphabet(const StringSet &symbols);
    // shadowed by the previous function: void add_symbols_to_alphabet(const StringPairSet &symbols);
    std::set<std::string> symbols_used();
    void prune_alphabet(bool force=true);
    const std::set<std::string> &get_alphabet() const;

    HfstState add_state(void);
    HfstState add_state(HfstState s);
    HfstState get_max_state() const;
    std::vector<HfstState> states() const;
    void add_transition(HfstState s, const hfst::implementations::HfstBasicTransition & transition,
                         bool add_symbols_to_alphabet=true);
    void remove_transition(HfstState s, const hfst::implementations::HfstBasicTransition & transition,
                            bool remove_symbols_from_alphabet=false);
    bool is_final_state(HfstState s) const;
    float get_final_weight(HfstState s) const;
    void set_final_weight(HfstState s, const float & weight);
    HfstBasicTransducer &sort_arcs(void);
    const std::vector<HfstBasicTransition> & transitions(HfstState s) const;

    //void write_in_prolog_format(FILE * file, const std::string & name, 
    //                                 bool write_weights=true);
    //static HfstBasicTransducer read_in_prolog_format
    //       (FILE *file, 
    //        unsigned int & linecount) ;
    // void write_in_xfst_format(FILE * file, bool write_weights=true);
    // void write_in_att_format(FILE *file, bool write_weights=true); 

    void write_in_att_format(char * ptr, bool write_weights=true);
    // static HfstBasicTransducer read_in_att_format
    //       (FILE *file, 
    //        std::string epsilon_symbol,
    //        unsigned int & linecount);

    HfstBasicTransducer & substitute_symbol(const std::string &old_symbol, const std::string &new_symbol, bool input_side=true, bool output_side=true);
    HfstBasicTransducer & substitute_symbol_pair(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair);
    HfstBasicTransducer & substitute_symbol_pair_with_set(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set);
    HfstBasicTransducer & substitute_symbol_pair_with_transducer(const StringPair &symbol_pair, HfstBasicTransducer &transducer);
    HfstBasicTransducer & substitute_symbols(const hfst::HfstSymbolSubstitutions &substitutions); // alias for the previous function which is shadowed
    HfstBasicTransducer & substitute_symbol_pairs(const hfst::HfstSymbolPairSubstitutions &substitutions); // alias for the previous function which is shadowed
    
    // TODO lookup_fd

    hfst::implementations::HfstBasicStates states_and_transitions() const;



%extend {
  void write_in_prolog_format(hfst::HfstFile & f, const std::string & name, bool write_weights=true) {
    $self->write_in_prolog_format(f.get_file(), name, write_weights);
  }
  static HfstBasicTransducer read_in_prolog_format(hfst::HfstFile & f, unsigned int & linecount) {
    return hfst::implementations::HfstBasicTransducer::read_in_prolog_format(f.get_file(), linecount);
  }
  void write_in_xfst_format(hfst::HfstFile & f, bool write_weights=true) {
    $self->write_in_xfst_format(f.get_file(), write_weights);
  }
  void write_in_att_format(hfst::HfstFile & f, bool write_weights=true) {
    $self->write_in_att_format(f.get_file(), write_weights);
  }

  char * __str__()
  {
    static char str[1024];
    $self->write_in_att_format(str, true); // write_weights=true  
    return str;
  }

  static HfstBasicTransducer read_in_att_format(HfstFile & f, std::string epsilon_symbol, unsigned int & linecount) {
    return hfst::implementations::HfstBasicTransducer::read_in_att_format(f.get_file(), epsilon_symbol, linecount);
  }
  void add_transition(HfstState source, HfstState target, std::string input, std::string output, float weight=0) {
    hfst::implementations::HfstBasicTransition tr(target, input, output, weight);
    $self->add_transition(source, tr);
  }

%pythoncode{
  def __iter__(self):
      return self.states_and_transitions().__iter__()

  def __enumerate__(self):
      return enumerate(self.states_and_transitions())

  def substitute(self, s, S=None, **kvargs):

      if S == None:
         if not isinstance(s, dict):
            raise RuntimeError('First input argument must be a dictionary.')

         subst_type=""

         for k, v in s.items():
             if is_string(k):
                if subst_type == "":
                   subst_type="string"
                elif subst_type == "string pair":
                   raise RuntimeError('')
                if not is_string(v):
                   raise RuntimeError('')
             elif is_string_pair(k):
                if subst_type == "":
                   subst_type="string pair"
                elif subst_type == "string":
                   raise RuntimeError('')
                if not is_string_pair(v):
                   raise RuntimeError('')
             else:
                raise RuntimeError('')

         if subst_type == "string":
            return self.substitute_symbols(s)
         else:
            return self.substitute_symbol_pairs(s)

      if is_string(s):
         if is_string(S):
            input=True
            output=True
            for k,v in kvargs.items():
                if k == 'input':
                   if v == False:
                      input=False
                elif k == 'output':
                   if v == False:
                      output=False
                else:
                   raise RuntimeError('Free argument not recognized.')
            return self.substitute_symbol(s, S, input, output)
         else:
            raise RuntimeError('...')
      elif is_string_pair(s):
         if is_string_pair(S):
            return self.substitute_symbol_pair(s, S)
         elif is_string_pair_vector(S):
            return self.substitute_symbol_pair_with_set(s, S)
         elif isinstance(S, HfstBasicTransducer):
            return self.substitute_symbol_pair_with_transducer(s, S)
         else:
            raise RuntimeError('...')
      else:
         raise RuntimeError('...')

%}

}
        
};

class HfstBasicTransition {
  public:
    HfstBasicTransition();
    HfstBasicTransition(hfst::implementations::HfstState, std::string, std::string, float);
    ~HfstBasicTransition();
    HfstState get_target_state() const;
    std::string get_input_symbol() const;
    std::string get_output_symbol() const;
    float get_weight() const;
  
%extend{
    char *__str__() {
      static char str[1024];
      sprintf(str, "%u %s %s %f", $self->get_target_state(), $self->get_input_symbol().c_str(), $self->get_output_symbol().c_str(), $self->get_weight());
      return str;
    }
}

};

}


namespace pmatch {
  class PmatchCompiler
  {
    public:
      PmatchCompiler();
      PmatchCompiler(hfst::ImplementationType impl);
      void set_flatten(bool val) { flatten = val; }
      void set_verbose(bool val) { verbose = val; }
      void define(const std::string& name, const std::string& pmatch);
      std::map<std::string, HfstTransducer*> compile(const std::string& pmatch);
  };
}

namespace xre {
class XreCompiler
{
  public:
  XreCompiler();
  XreCompiler(hfst::ImplementationType impl);
  //XreCompiler(const struct XreConstructorArguments & args);
  void define(const std::string& name, const std::string& xre);
  void define_list(const std::string& name, const std::set<std::string>& symbol_list);
  bool define_function(const std::string& name, 
                       unsigned int arguments,
                       const std::string& xre);
  bool is_definition(const std::string& name);
  bool is_function_definition(const std::string& name);
  void define(const std::string& name, const HfstTransducer & transducer);
  void undefine(const std::string& name);
  HfstTransducer* compile(const std::string& xre);
  HfstTransducer* compile_first(const std::string& xre, unsigned int & chars_read);
  std::string get_error_message();
  bool contained_only_comments();
  bool get_positions_of_symbol_in_xre
    (const std::string & symbol, const std::string & xre, std::set<unsigned int> & positions);
  void set_expand_definitions(bool expand);
  void set_harmonization(bool harmonize);
  void set_flag_harmonization(bool harmonize_flags);
  void set_verbosity(bool verbose, FILE * file);
};
}


namespace lexc {
  class LexcCompiler
  {
    public:
      LexcCompiler();
      LexcCompiler(hfst::ImplementationType impl);
      LexcCompiler(hfst::ImplementationType impl, bool withFlags);
      LexcCompiler& parse(FILE* infile); // todo: HfstFile
      LexcCompiler& parse(const char* filename);
      LexcCompiler& setVerbosity(unsigned int verbose);
      bool isQuiet();
      LexcCompiler& setTreatWarningsAsErrors(bool value);
      bool areWarningsTreatedAsErrors();
      LexcCompiler& setAllowMultipleSublexiconDefinitions(bool value);
      LexcCompiler& setWithFlags(bool value);
      LexcCompiler& setMinimizeFlags(bool value);
      LexcCompiler& setRenameFlags(bool value);
      LexcCompiler& addAlphabet(const std::string& alphabet);
      LexcCompiler& addNoFlag(const std::string& lexname);
      LexcCompiler& setCurrentLexiconName(const std::string& lexicon_name);
      LexcCompiler& addStringEntry(const std::string& entry,
                                   const std::string& continuation,
                                   const double weight);
      LexcCompiler& addStringPairEntry(const std::string& upper,
                                       const std::string& lower,
                                       const std::string& continuation,
                                       const double weight);
      LexcCompiler& addXreEntry(const std::string& xre,
                                const std::string& continuation, 
                                const double weight);
      LexcCompiler& addXreDefinition(const std::string& name,
                                     const std::string& xre);
      LexcCompiler& setInitialLexiconName(const std::string& lexicon_name);
      hfst::HfstTransducer* compileLexical();
      // not implemented?: const std::map<std::string,hfst::HfstTransducer>& getStringTries() const;
      // not implemented?: const std::map<std::string,hfst::HfstTransducer>& getRegexpUnions() const;
      const LexcCompiler& printConnectedness(bool & warnings_printed) const;
  };
// ugh, the global
//extern LexcCompiler* lexc_;
}

hfst::HfstTransducer fst(const std::string & symbol);
hfst::HfstTransducer fst(const std::string & isymbol, const std::string & osymbol);
hfst::HfstTransducer * regex(const std::string & regex_string);
hfst::HfstTransducer * compile_lexc_file(const std::string & filename);
hfst::HfstTransducer word(const std::string & w, float weight=0);
//hfst::HfstTransducer word(const std::string & w, const hfst::HfstTokenizer & tok);
hfst::HfstTransducer word_pair(const std::string & wi, const std::string & wo, float weight=0);
//hfst::HfstTransducer word_pair(const std::string & wi, const std::string & wo, const hfst::HfstTokenizer & tok);
hfst::HfstTransducer word_list_hfst(const StringVector & wl, const FloatVector & weights);
hfst::HfstTransducer word_pair_list_hfst(const StringPairVector & wpl, const FloatVector & weights);

void set_default_fst_type(hfst::ImplementationType t);
hfst::ImplementationType get_default_fst_type();
std::string fst_type_to_string(hfst::ImplementationType t);

hfst::HfstTransducer * read_att(hfst::HfstFile & f, std::string epsilon="@_EPSILON_SYMBOL_@") throw(EndOfStreamException);
hfst::HfstTransducer * read_prolog(hfst::HfstFile & f) throw(EndOfStreamException);

std::string one_level_paths_to_string(const HfstOneLevelPaths &);
std::string two_level_paths_to_string(const HfstTwoLevelPaths &);

}

%pythoncode %{

EPSILON='@_EPSILON_SYMBOL_@'
UNKNOWN='@_UNKNOWN_SYMBOL_@'
IDENTITY='@_IDENTITY_SYMBOL_@'

def word_list(l):
        if not isinstance(l, list):
                raise RuntimeError('Input argument must be a list.')

        v = StringVector()
        f = FloatVector()
        for item in l:
                if isinstance(item, str):
                        v.push_back(item)
                        f.push_back(0)
                else:
                        v.push_back(item[0])
                        f.push_back(item[1])
        return _libhfst.word_list_hfst(v,f)
                
#def word_pair_list(l):
#        if not isinstance(l, list):
#                raise RuntimeError('Input argument must be a list.')
#
#        v = StringPairVector()
#        f = FloatVector()
#        for item in l:
#                v.push_back(StringPair(word[0],word[1]))
#                if (len(word) > 2):
#                        f.push_back(word[2])
#                else:
#                        f.push_back(0)  
#        return _libhfst.word_pair_list_hfst(v,f)

def dictionary(d):
    if not isinstance(d, dict):
       raise RuntimeError('Input argument must be a dictionary.')

    for k,v in d.items():
        if not isinstance(k, str):
            raise RuntimeError('Keys in the dictionary must be strings.')
        if isinstance(v, str): # "bar"
           pass
        elif isinstance(v, list) or isinstance(v, tuple): # ("bar", "baz")
           pass
        else:
           raise RuntimeError('Each value in the dictionary must be a string or a tuple/list of strings.')

        print("%s\t%s" % (k,v))

def foobar(f):
    print('FOOBAR: %s' % (f))

%}
