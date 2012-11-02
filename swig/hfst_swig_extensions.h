#include <string>
#include <vector>
#include <utility>
#include "HfstDataTypes.h"
#include "HfstTransducer.h"
#include "HfstFlagDiacritics.h"

// NO EFFECT in Swig...
// Make sure that all standard streams are in binary mode.
// Line feed and carriage return characters are not handled correctly
// in text mode.
//#include <fcntl.h>
//int _CRT_fmode = _O_BINARY;


typedef std::vector<std::pair <float, std::vector<std::string> > >
OneLevelPathVector;

typedef std::vector<std::pair <std::string, float> > StringFloatVector;

namespace hfst {

  ImplementationType sfst_type() { return SFST_TYPE; }
  ImplementationType tropical_openfst_type() { return TROPICAL_OPENFST_TYPE; }
  ImplementationType foma_type() { return FOMA_TYPE; }
  ImplementationType hfst_ol_type() { return HFST_OL_TYPE; }
  ImplementationType hfst_olw_type() { return HFST_OLW_TYPE; }


  HfstTwoLevelPaths extract_paths(const HfstTransducer &t, int max_num=-1, int cycles=-1)
  {
    HfstTwoLevelPaths result;
    t.extract_paths(result, max_num, cycles);
    return result;
  }

  HfstTwoLevelPaths extract_paths_fd(const HfstTransducer &t, int max_num=-1, int cycles=-1, bool filter_fd=false)
  {
    HfstTwoLevelPaths result;
    t.extract_paths_fd(result, max_num, cycles, filter_fd);
    return result;
  }


OneLevelPathVector vectorize(HfstOneLevelPaths * holps)
{
    std::vector<std::pair <float, std::vector<std::string> > > retval;
    for (HfstOneLevelPaths::const_iterator it = holps->begin();
     it != holps->end(); ++it) {
    retval.push_back(*it);
    }
    return retval;
}

OneLevelPathVector purge_flags(OneLevelPathVector olpv)
{
    OneLevelPathVector retval;
    for (OneLevelPathVector::const_iterator it = olpv.begin(); it != olpv.end();
     ++it) {
    std::vector<std::string> temp;
    for (std::vector<std::string>::const_iterator str_it =
         it->second.begin(); str_it != it->second.end(); ++str_it) {
        if (!FdOperation::is_diacritic(*str_it)) {
            temp.push_back(*str_it);
        }
    }
    retval.push_back(std::pair<float, std::vector<std::string> >(it->first, temp));
    }
    return retval;
}

StringFloatVector detokenize_vector(OneLevelPathVector olpv)
{
    StringFloatVector retval;
    for (OneLevelPathVector::const_iterator it = olpv.begin(); it != olpv.end();
     ++it) {
    std::string temp;
    for (std::vector<std::string>::const_iterator str_it =
         it->second.begin(); str_it != it->second.end(); ++str_it) {
        temp.append(*str_it);
    }
    retval.push_back(std::pair<std::string, float>(temp, it->first));
    }
    return retval;
}

StringFloatVector detokenize_paths(HfstOneLevelPaths * holps)
{
    return detokenize_vector(vectorize(holps));
}

  }
