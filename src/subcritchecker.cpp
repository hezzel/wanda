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

#include "environment.h"
#include "outputmodule.h"
#include "rulesmanipulator.h"
#include "subcritchecker.h"
#include "sat.h"
#include <cstdio>

bool SubtermCriterionChecker :: run(DPSet &set, DPSet &strict,
                                    DPSet &nonstrict,
                                    Alphabet &F,
                                    ArList &original_arities) {
  int i;

  wout.start_method("subterm criterion");

  if (!non_collapsing(set)) {
    wout.verbose_print("This set of dependency pairs is collapsing, "
      "so we cannot use the subterm criterion.\n");
    wout.abort_method("subterm criterion");
    return false;
  }

  formula = new And();
  SatSolver sat;

  get_maximal_arities(set);
  declare_nu_variables();
  declare_strictness_variables(set.size());
  map<string,int> sortordering;
  add_main_requirements(set, false, sortordering, original_arities);
  bool ret;
  PFormula form = formula;

  if (sat.solve(form)) {
    ret = true;
    get_solution(set, strict, nonstrict, F, original_arities, false);
    wout.succeed_method("subterm criterion");
  }
  else {
    ret = false;
    wout.verbose_print("Unfortunately, there is no suitable "
      "projection function to apply the subterm criterion.\n");
    wout.abort_method("subterm criterion");
  }
  
  vars.reset();
  delete form;
  formula = NULL;
  return ret;
}

bool SubtermCriterionChecker :: run_static(DPSet &set, DPSet &strict,
                                    DPSet &nonstrict,
                                    Alphabet &F,
                                    ArList &original_arities,
                                    map<string,int> &sortordering) {
  int i;

  wout.start_method("accessible subterm criterion");

  if (!non_collapsing(set)) {
    wout.verbose_print("This set of dependency pairs is collapsing, "
      "so we cannot use the accessible subterm criterion.\n");
    wout.abort_method("subterm criterion");
    return false;
  }

  formula = new And();
  SatSolver sat;

  get_maximal_arities(set);
  declare_nu_variables();
  declare_strictness_variables(set.size());
  add_main_requirements(set, true, sortordering, original_arities);
  bool ret;
  PFormula form = formula;

  if (sat.solve(form)) {
    ret = true;
    get_solution(set, strict, nonstrict, F, original_arities, true);
    wout.succeed_method("accessible subterm criterion");
  }
  else {
    ret = false;
    wout.verbose_print("Unfortunately, there is no suitable "
      "projection function to apply the accessible subterm "
      "criterion.\n");
    wout.abort_method("accessible subterm criterion");
  }
  
  vars.reset();
  delete form;
  formula = NULL;
  return ret;
}

bool SubtermCriterionChecker :: non_collapsing(DPSet &set) {
  for (int i = 0; i < set.size(); i++) {
    if (!set[i]->query_left()->query_head()->query_constant() ||
        !set[i]->query_right()->query_head()->query_constant()) {
      return false;
    }
  }
  return true;
}

void SubtermCriterionChecker :: get_maximal_arities(DPSet &set) {
  arities.clear();
  for (int i = 0; i < set.size(); i++) {
    for (int j = 0; j < 2; j++) {
      PTerm t = (j == 0 ? set[i]->query_left() : set[i]->query_right());
      vector<PTerm> partsl = t->split();
      string f = partsl[0]->to_string(false);
      if (arities.find(f) == arities.end() ||
          arities[f] >= partsl.size()) {
        arities[f] = partsl.size()-1;
      }
    }
  }
}

void SubtermCriterionChecker :: declare_nu_variables() {
  int i;
  varstart.clear();
  for (map<string,int>::iterator it = arities.begin();
       it != arities.end(); it++) {
    int start = vars.query_size();
    int end = start + it->second;
    varstart[it->first] = vars.query_size();
    vars.add_vars(it->second);
    
    // nu(f) = i for at least one i
    Or *atleastone = new Or();
    for (i = start; i < end; i++)
      atleastone->add_child(new Var(i));
    formula->add_child(atleastone);

    // nu(f) = i for at most one i
    for (i = start; i < end; i++) {
      for (int j = i+1; j < end; j++) {
        formula->add_child(new Or(new AntiVar(i),
                                  new AntiVar(j)));
      }
    }
  }
}

void SubtermCriterionChecker :: declare_strictness_variables(int setsize) {
  strictstart = vars.query_size();
  vars.add_vars(setsize);
  Or *atleastonestrict = new Or();
  for (int i = 0; i < setsize; i++) {
    atleastonestrict->add_child(new Var(strictstart+i));
  }
  formula->add_child(atleastonestrict);
}

void SubtermCriterionChecker :: add_main_requirements(DPSet &set,
                    bool accessible, map<string,int> &sortordering,
                    ArList &original_arities) {
  for (int i = 0; i < set.size(); i++) {
    vector<PTerm> lsplit = set[i]->query_left()->split();
    vector<PTerm> rsplit = set[i]->query_right()->split();
    string f = lsplit[0]->to_string(false);
    string g = rsplit[0]->to_string(false);
      // say l = f s1 ... sn and r = g t1 ... tm
    for (int j = 0; j < arities[f]; j++) {
      int Xfj = varstart[f] + j;
      for (int k = 0; k < arities[g]; k++) {
        int Xgk = varstart[g] + k;
        int chk;
        if (accessible)
          chk = check_accessible(lsplit[j+1], rsplit[k+1], sortordering,
                                 original_arities);
        else chk = check_subterm(lsplit[j+1], rsplit[k+1]);
        // if not sj |> tk, then we can't have nu(f) = i and nu(g) =
        // k, or the subterm criterion isn't satisfied!
        if (chk == -1) {
          formula->add_child(new Or(new AntiVar(Xfj),
                                    new AntiVar(Xgk)));
        }
        // if sj = tk, and nu(f) = j and nu(g) = k, then this pair
        // is not oriented strictly
        if (chk == 0) {
          Or *notstrict = new Or(new AntiVar(Xfj),
                                 new AntiVar(Xgk));
          notstrict->add_child(new AntiVar(strictstart+i));
          formula->add_child(notstrict);
        }
        // if sj |> tk, then we might have nu(f) = j and nu(g) = k!
      }
    }
  }
}

int SubtermCriterionChecker :: check_subterm(PTerm a, PTerm b) {
  if (a->equals(b)) return 0;
  if (a->query_meta()) return -1;
    // don't check subterms of a meta-variable application
  for (int i = 0; i < a->number_children(); i++) {
    if (check_subterm(a->get_child(i), b) >= 0)
      return 1;
  }
  return -1;
}

int SubtermCriterionChecker :: check_accessible(PTerm a, PTerm b,
                                   map<string,int> &sortordering,
                                   ArList &original_arities) {
  PTerm head = b->query_head();
  if (a->equals(b)) return 0;
  vector<PTerm> parts = a->split();
  if (!parts[0]->query_constant()) return -1;

  if (head->query_meta()) {
    PVariable Z = dynamic_cast<MetaApplication*>(head)->get_metavar();
    if (check_accessible_meta_or_subterm(a, NULL, Z, sortordering,
        original_arities)) return 1;
    else return -1;
  }
  int arity = original_arities[parts[0]->to_string(false)];
  for (int i = 0; i < arity && i+1 < parts.size(); i++) {
    if (check_accessible_meta_or_subterm(parts[i+1], b, NULL,
        sortordering, original_arities)) return 1;
  }

  return -1;
}

bool SubtermCriterionChecker :: check_accessible_meta_or_subterm(
                            PTerm a, PTerm b, PVariable Z,
                            map<string,int> &sortordering,
                            ArList &original_arities) {
  static RulesManipulator manip;

  if (Z != NULL) {
    PVariable X = manip.extended_variable(a);
    if (X != NULL) return Z->query_index() == X->query_index();
  }
  if (b != NULL && a->equals(b)) return true;

  if (a->query_abstraction()) {
    return check_accessible_meta_or_subterm(a->get_child(0), b, Z,
                                            sortordering,
                                            original_arities);
  }

  if (!a->query_type()->query_data()) return false;
  vector<PTerm> parts = a->split();
  if (!parts[0]->query_constant()) return false;
  string fname = parts[0]->to_string(false);
  int arity = original_arities[fname];
  if (arity+1 != parts.size()) return false;

  string output =
    dynamic_cast<DataType*>(a->query_type())->query_constructor();
  for (int i = 0; i < arity; i++) {
    PTerm sub = parts[i+1];
    if (positions_okay(sub->query_type(), output, sortordering, true)) {
      if (check_accessible_meta_or_subterm(sub, b, Z, sortordering,
                                           original_arities))
        return true;
    }
  }

  return false;
}

bool SubtermCriterionChecker :: positions_okay(PType sigma, string kappa,
                                              map<string,int> &sortord,
                                              bool positive) {
  if (sigma->query_data()) {
    string iota = dynamic_cast<DataType*>(sigma)->query_constructor();
    if (positive) return sortord[kappa] >= sortord[iota];
    else return sortord[kappa] > sortord[iota];
  }

  if (!sigma->query_composed()) return false;

  PType a = sigma->query_child(0);
  PType b = sigma->query_child(1);
  return positions_okay(a, kappa, sortord, !positive) &&
         positions_okay(b, kappa, sortord, positive);
}

void SubtermCriterionChecker :: get_solution(DPSet &set, DPSet &stricts,
                                             DPSet &nonstricts,
                                             Alphabet &F,
                                             ArList &original_arities,
                                             bool accessible) {
  wout.print("We apply the " + string(accessible ? "accessible " : "") +
    "subterm criterion with the following projection function:\n");

  // get and print projection function
  map<string,int> nu;
  wout.start_table();
  for (ArList::iterator it = varstart.begin(); it != varstart.end(); it++) {
    // get nu(f)
    string f = it->first;
    int start = it->second;
    int N = arities[f];
    for (int i = 0; i < N; i++) {
      if (vars.query_value(start+i) == TRUE) nu[f] = i+1;
    }
    // print nu(f)
    vector<string> entry;
    entry.push_back(wout.projection_symbol() + "(" + f + ")");
    entry.push_back("=");
    entry.push_back(wout.str(nu[f]));
    wout.table_entry(entry);
  }
  wout.end_table();
  
  // print projections, and split into stricts and nonstricts
  stricts.clear();
  nonstricts.clear();
  wout.print("Thus, we can orient the dependency pairs as follows:\n");
  wout.start_table();
  for (int i = 0; i < set.size(); i++) {
    // obtain values
    PTerm left = set[i]->query_left();
    PTerm right = set[i]->query_right();
    vector<PTerm> lsplit = set[i]->query_left()->split();
    vector<PTerm> rsplit = set[i]->query_right()->split();
    string f = lsplit[0]->to_string(false);
    string g = rsplit[0]->to_string(false);
    PTerm lsub = lsplit[nu[f]];
    PTerm rsub = rsplit[nu[g]];
    Environment gamma;
    string ll = lsub->to_string(gamma);
    string rr = rsub->to_string(gamma);
    //bool strict = vars.query_value(strictstart+i) == TRUE;
    bool strict = ll != rr;
      // this is more accurate than checking the variable, since the
      // variable may be false even if the requirement is strictly
      // oriented!

    // obtain values for the printer
    vector<string> entry;
    map<int,string> metanaming, freenaming, boundnaming;
    string leftprint = wout.print_term(left, original_arities, F, metanaming,
                                       freenaming, boundnaming);
    string lsubprint = wout.print_term(lsub, original_arities, F, metanaming,
                                       freenaming, boundnaming);
    string rsubprint = wout.print_term(rsub, original_arities, F, metanaming,
                                       freenaming, boundnaming);
    string rightprint = wout.print_term(right, original_arities, F, metanaming,
                                        freenaming, boundnaming);
    
    // create the table entry
    entry.push_back(wout.full_projection_symbol() + "(" + leftprint
                    + ")");
    entry.push_back("=");
    entry.push_back(lsubprint);
    if (!strict) entry.push_back("=");
    else if (!accessible) entry.push_back(wout.superterm_symbol());
    else entry.push_back(wout.rank_reduce_symbol());
    entry.push_back(rsubprint);
    entry.push_back("=");
    entry.push_back(wout.full_projection_symbol() + "(" + rightprint
                    + ")");
    wout.table_entry(entry);

    // and update the lists
    if (strict) stricts.push_back(set[i]);
    else nonstricts.push_back(set[i]);
  }
  wout.end_table();
}

