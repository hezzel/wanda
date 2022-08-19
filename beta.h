/**************************************************************************
   Copyright 2012 Cynthia Kop

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#ifndef BETA_H
#define BETA_H

#include "term.h"
#include "rule.h"

/**
 * This class implements the beta rule, which is commonly used in
 * higher order rewrite systems (although depending on the actual
 * system used it might be a separate rule or an implicit step
 * following each rule step).
 */

class Beta : public Rule {
  protected:
    PTerm apply_top(PTerm term);
    bool applicable_top(PTerm term);

  public:
    Beta() : Rule("beta") {}
};

#endif
