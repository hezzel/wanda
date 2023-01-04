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

#include "ruleremover.h"
#include "requirement.h"
#include "polymodule.h"
#include "rulesmanipulator.h"
#include "horpo.h"
#include "outputmodule.h"

RuleRemover :: RuleRemover(bool use_pol, bool use_hor, bool use_prod,
                           bool use_ar, bool formal) {
  use_poly = use_pol;
  use_horpo = use_hor;
  use_polyprod = use_prod;
  use_arities = use_ar;
  formal_output = formal;
}

bool RuleRemover :: remove_rules(Alphabet &F, Ruleset &R) {
  bool removed_something = false;

  wout.verbose_print("Doing rule removal...\n");

  while (true) {
    wout.start_method("rule removal");
    wout.print("We use rule removal, following " +
      wout.cite("Kop12", "Theorem 2.23") + ".\n");
    if (attempt_rule_removal(F, R)) {
      removed_something = true;
      wout.succeed_method("rule removal");
    }
    else {
      wout.abort_method("rule removal");
      break;
    }
    if (R.empty()) {
      if (removed_something)
        wout.print("All rules were succesfully removed.  Thus, "
          "termination of the original system has been reduced to "
          "termination of the beta-rule, which is well-known to "
          "hold.\n");
      break;
    }
  }
  return removed_something;
}

bool RuleRemover :: attempt_rule_removal(Alphabet &F, Ruleset &R) {
  // make an ordering problem to send into the horpo module
  vars.reset();
  OrderingProblem *prob = new PlainOrderingProblem(R, F, use_arities);

  wout.print("This gives the following requirements (possibly using "
    "Theorems 2.25 and 2.26 in " + wout.cite("Kop12") + "):\n");
  prob->print();

  // try rule removal with linear polynomial interpretations
  if (use_poly && poly_handle(prob, F, R, false))
    return true;

  wout.verbose_print("About to try horpo.\n");

  // no? Try horpo
  if (use_horpo && horpo_handle(prob, F, R))
    return true;

  wout.verbose_print("About to try polyprod.\n");

  // try rule removal with product polynomial interpretations
  if (use_poly && use_polyprod && poly_handle(prob, F, R, true))
    return true;

  wout.print("about to return\n");
  delete prob;

  // nothing worked :(
  return false;
}

bool RuleRemover :: poly_handle(OrderingProblem *prob, Alphabet &F,
                                Ruleset &R, bool products) {
  // basic data
  PolyModule pols;

  wout.start_method("poly attempt");

  // use the poly-tool!
  pols.set_use_products(products);
  vector<int> ok = pols.orient(prob);
  map<string,int> arities = prob->arities;
  
  if (ok.size() != 0) {
    wout.print("We can thus remove the following rules:\n");
    vector<MatchRule*> Rok;
    int j;
    for (j = 0; j < ok.size(); j++) {
      Rok.push_back(R[ok[j]]);
      R[ok[j]] = NULL;
    }
    wout.print_rules(Rok, F, arities);
    if (formal_output) {
      wout.formal_print("Removed: ");
      wout.formal_print_rules(Rok, F);
      wout.formal_print("\n");
    }
    for (j = 0; j < Rok.size(); j++) delete Rok[j];
    wout.print("\n");
    Ruleset Rmod;
    for (int k = 0; k < R.size(); k++) {
      if (R[k] != NULL) Rmod.push_back(R[k]);
    }
    R = Rmod;
    wout.succeed_method("poly attempt");
    return true;
  }
  wout.verbose_print("Could not remove more rules using polynomial "
    "interpretations");
  if (!products) wout.verbose_print(" (without variable products)");
  wout.verbose_print("\n");
  wout.abort_method("poly attempt");
  return false;
}

bool RuleRemover :: horpo_handle(OrderingProblem *prob, Alphabet &F,
                                 Ruleset &R) {
  Horpo horpo;

  wout.start_method("horpo attempt");

  // use the horpo-tool!
  vector<int> ok = horpo.orient(prob);
  map<string,int> arities = prob->arities;
  
  if (ok.size() != 0) {
    wout.print("We can thus remove the following rules:\n");
    vector<MatchRule*> Rok;
    int j;
    for (j = 0; j < ok.size(); j++) {
      Rok.push_back(R[ok[j]]);
      R[ok[j]] = NULL;
    }
    wout.print_rules(Rok, F, arities);
    for (j = 0; j < Rok.size(); j++) delete Rok[j];
    wout.print("\n");
    Ruleset Rmod;
    for (int k = 0; k < R.size(); k++) {
      if (R[k] != NULL) Rmod.push_back(R[k]);
    }
    R = Rmod;
    wout.succeed_method("horpo attempt");
    return true;
  }
  wout.verbose_print("Could not remove more rules using StarHorpo.\n");
  wout.abort_method("horpo attempt");
  return false;
}

