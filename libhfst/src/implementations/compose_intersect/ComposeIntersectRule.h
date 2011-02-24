#ifndef COMPOSE_INTERSECT_RULE_H
#define COMPOSE_INTERSECT_RULE_H

#include "ComposeIntersectFst.h"

namespace hfst
{
  namespace implementations
  {
    class ComposeIntersectRule : public ComposeIntersectFst
    {
    public:
      ComposeIntersectRule(const HfstBasicTransducer &);
      ComposeIntersectRule(void);
    };
  }
}

#endif
