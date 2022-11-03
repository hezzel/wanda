/**************************************************************************
   Copyright 2012, 2013 Cynthia Kop

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

#include "matchrule.h"
#include "substitution.h"
#include "typesubstitution.h"
#include "environment.h"

MatchRule :: MatchRule() : Rule("AFSM rule"), valid(false), l(NULL), r(NULL),
  invalid_reason("rule not set") {}

MatchRule :: MatchRule(PTerm _l, PTerm _r)
    : l(_l), r(_r), valid(true) {
  valid = check_valid();
}

MatchRule :: ~MatchRule() {
  if (l != NULL) delete l;
  if (r != NULL) delete r;
}

MatchRule *MatchRule :: copy() {
  return new MatchRule(l->copy(), r->copy());
}

bool MatchRule :: query_valid() {
  return valid;
}

bool MatchRule :: check_valid() {
  invalid_reason = "rule is valid";

  // we must have a left hand side and a right hand side
  if (l == NULL || r == NULL) {
    if (l == NULL) invalid_reason = "left hand side not set up";
    else invalid_reason = "right hand side not set up";
    return false;
  }

  // the left hand side must be headed by a constant
  if (!l->query_head()->query_constant()) {
    invalid_reason = "left hand side should be headed by a constant";
    return false;
  }

  // the left-hand side must be a pattern
  if (!l->query_pattern()) {
    invalid_reason = "left-hand side must be a pattern";
  }

  // both sides must have the same type
  if (!l->query_type()->equals(r->query_type())) {
    invalid_reason = "both sides should have the same type";
    return false;
  }

  // both sides must be closed
  Varset FVl = l->free_var(false);
  Varset FVr = r->free_var(false);
  if (FVl.size() != 0 || FVr.size() != 0) {
    invalid_reason = "rules should be closed (did you use variables "
      "instead of meta-variables?)";
    return false;
  }

  // all type variables from the right hand side must occur in the
  // hand side
  Varset FTVl = l->free_typevar();
  Varset FTVr = r->free_typevar();
  if (!FTVl.contains(FTVr)) {
    invalid_reason = "right-hand side contains type variables not "
      "occurring in left-hand side";
    return false;
  }

  // all meta-variables from the right hand side must occur in the
  // left hand side
  Varset FMVl = l->free_var(true);
  Varset FMVr = r->free_var(true);
  if (!FMVl.contains(FMVr)) {
    invalid_reason = "right-hand side contains type variables not "
      "occurring in left-hand side";
    return false;
  }

  return true;
}

string MatchRule :: query_invalid_reason() {
  return invalid_reason;
}

PTerm MatchRule :: apply_top(PTerm term) {
  TypeSubstitution theta;
  Substitution gamma;
  if (!query_valid() ||
      !l->instantiate(term, theta, gamma)) return term;
  PTerm ret = r->copy();
  ret->apply_type_substitution(theta);
  ret = ret->apply_substitution(gamma);
  delete term;
  return ret;
}

bool MatchRule :: applicable_top(PTerm term) {
  TypeSubstitution theta;
  Substitution gamma;
  return query_valid() && l->instantiate(term, theta, gamma);
}

string MatchRule :: to_string(bool showtypes) {
  Environment env;
  if (showtypes) {
    TypeNaming tenv;
    string ret = l->to_string(env,tenv);
    ret += " => " + r->to_string(env, tenv);
    return ret;
  }
  else {
    string ret = l->to_string(env);
    ret += " => " + r->to_string(env);
    return ret;
  }
}

PTerm MatchRule :: query_left_side() {
  return l;
}

PTerm MatchRule :: query_right_side() {
  return r;
}

void MatchRule :: replace_right_side(PTerm newright) {
  r = newright;
}
