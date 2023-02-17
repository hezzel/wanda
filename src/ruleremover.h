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

#include "alphabet.h"
#include "matchrule.h"
#include "orderingproblem.h"

typedef vector<MatchRule*> Ruleset;

/**
 * This class takes care of rule removal, using strongly monotonic
 * polynomial interpretations, and horpo without argument functions.
 */

class RuleRemover {
  private:
    bool verbose;
    bool use_poly;
    bool use_polyprod;
    bool use_horpo;
    bool use_arities;
    bool formal_output;

    bool attempt_rule_removal(Alphabet &F, Ruleset &R);
      // main functionality: tries to remove one or more rules from
      // the given set, and returns true if this was succesful
    bool poly_handle(OrderingProblem *problem, Alphabet &F,
                     Ruleset &rules, bool products);
      // tries polynomial interpretations, with or without products
    bool horpo_handle(OrderingProblem *problem, Alphabet &F,
                      Ruleset &R);
      // temporary use of external horpo solver

  public:
    RuleRemover(bool use_poly, bool use_horpo, bool use_polyprod,
                bool use_arities = true, bool formal_output = false);

    bool remove_rules(Alphabet &F, Ruleset &R);
      // returns true if rules were succesfully removed
};

