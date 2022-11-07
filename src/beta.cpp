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

#include "beta.h"
#include "substitution.h"

PTerm Beta :: apply_top(PTerm term) {
  if (!applicable_top(term)) return term;

  PTerm left = term->subterm("1");
  PTerm right = term->subterm("2");
  PVariable var =
    dynamic_cast<Abstraction*>(left)->query_abstraction_variable();

  Substitution sub;
  sub[var] = right->copy();
  PTerm result =
    left->replace_subterm(NULL, "1")->apply_substitution(sub);
  delete left;
  delete right;
  return result;
}

bool Beta :: applicable_top(PTerm term) {
  return term->query_application() &&
         term->subterm("1") != NULL &&
         term->subterm("1")->query_abstraction();
}

