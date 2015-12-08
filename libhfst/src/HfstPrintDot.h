#include "HfstTransducer.h"
#include <iostream>

namespace hfst 
{
  void print_dot(FILE* out, HfstTransducer& t);
  void print_dot(std::ostream & oss, HfstTransducer& t);
}
