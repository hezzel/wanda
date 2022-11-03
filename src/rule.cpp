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

#include "rule.h"

/* === Rule inherit === */

PTerm Rule :: apply_top(PTerm term) {
  return term;
}

bool Rule :: applicable_top(PTerm term) {
  return false;
}

PTerm Rule :: apply(PTerm term, string position) {
  if (term == NULL) return NULL;

  if (position == "") return apply_top(term);

  PTerm sub = term->subterm(position);
  if (sub == NULL) return term;
  sub = apply_top(sub);
  term->replace_subterm(sub, position);
  return term;
}

bool Rule :: applicable(PTerm term, string position) {
  if (term == NULL) return false;

  PTerm sub = term->subterm(position);
  if (sub == NULL) return false;
  return applicable_top(sub);
}

PTerm Rule :: normalise(PTerm term) {
  // normalise children
  if (term->query_abstraction()) {
    PTerm sub = normalise(term->subterm("1"));
    term->replace_subterm(sub, "1");
  }
  if (term->query_application()) {
    PTerm left = normalise(term->subterm("1"));
    PTerm right = normalise(term->subterm("2"));
    term->replace_subterm(left, "1");
    term->replace_subterm(right, "2");
  }

  // apply top rule and continue normalising if possible
  if (applicable_top(term)) {
    return normalise(apply_top(term));
  }
  else {
    return term;
  }
}

bool Rule :: normal(PTerm term) {
  PTerm sub1, sub2;

  if (term == NULL) return true;
  if (applicable_top(term)) return false;
  
  sub1 = term->subterm("1");
  sub2 = term->subterm("2");
  return normal(sub1) && normal(sub2);
}

string Rule :: to_string() {
  return name;
}

