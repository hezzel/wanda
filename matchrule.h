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

/**
 * The MatchRule class forms the basis for standard user defined
 * rules.  A term s can be rewritten to t by matchrule l->r, if
 * there are a type substitution theta and a substitution gamma such
 * that s = l theta gamma, t = r theta gamma.
 */

#ifndef MATCHRULE_H
#define MATCHRULE_H

#include "rule.h"

class MatchRule : public Rule {
  private:
    bool match_left(PTerm left, PTerm with, Substitution &sub,
                    Environment bound);

  protected:
    PTerm l, r;
    bool valid;
    string invalid_reason;

    virtual bool check_valid();
    virtual PTerm apply_top(PTerm term);
    virtual bool applicable_top(PTerm term);

  public:
    MatchRule();
    MatchRule(PTerm _l, PTerm _r);
    ~MatchRule();

    MatchRule *copy();

    bool query_valid();
    string query_invalid_reason();
    string to_string(bool showtypes = false);

    PTerm query_left_side();
    PTerm query_right_side();

    void replace_right_side(PTerm newright);
};

#endif
