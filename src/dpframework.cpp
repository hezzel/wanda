/**************************************************************************
   Copyright 2012, 2013, 2019 Cynthia Kop

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
#include "dpframework.h"
#include "environment.h"
#include "horpo.h"
#include "outputmodule.h"
#include "polymodule.h"
#include "subcritchecker.h"
#include "substitution.h"
#include "term.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

/* ================ initialising and destructing ================= */

DependencyFramework :: DependencyFramework(Alphabet &Sigma,
                                           Ruleset &rules,
                                           string FOtool,
                                           string FOnontool,
                                           bool _allow_static,
                                           bool _allow_dynamic)
     : allow_static(_allow_static), allow_dynamic(_allow_dynamic),
       allow_graph(true), allow_subcrit(true),
       allow_polynomials(true), allow_horpo(true),
       allow_product_polynomials(true), allow_usable(true),
       allow_formative(true), allow_uwrt(true), allow_fwrt(true),
       splitter(Sigma, rules, FOtool, FOnontool),
       found_counterexample(false), FOstatus(0), expanded(false) {

  int i;

  // copy constants
  vector<string> constants = Sigma.get_all();
  for (i = 0; i < constants.size(); i++)
    F.add(constants[i], Sigma.query_type(constants[i])->copy());
  
  // save original rules and check their first-order status
  for (i = 0; i < rules.size(); i++) {
    original_rules.push_back(rules[i]);
  }
  original_firstorder = manip.fully_first_order(rules);
  if (!original_firstorder) allow_uwrt = allow_fwrt = false;

  // determine arities
  arities = manip.get_arities(Sigma, rules);
  
  // determine which symbols are defined symbols
  for (i = 0; i < rules.size(); i++) {
    defineds.insert(rules[i]->query_left_side()->query_head()->to_string(false));
  }
  
  // find relevant properties for dependency pair framework
  leftlinear = manip.left_linear(rules);
  fullyextended = manip.fully_extended(rules);
  afs = manip.argument_free(rules);
  bool simplefullmeta = manip.algebraic(rules);
  bool technical = afs || manip.meta_single(rules) ||
                   manip.base_outputs(rules);
  abstraction_simple = leftlinear && simplefullmeta && technical;

  formative_flag = leftlinear && fullyextended;
  static_flag = 0;
  if (allow_static && manip.eta_long(rules, true) &&
      manip.monomorphic(rules) &&
      manip.strong_plain_function_passing(rules, arities, sortordering)) {
    if (sortordering.size() == 0) static_flag = 1;
    else static_flag = 2;
  }
}

DependencyFramework :: ~DependencyFramework() {
  free_memory();
}

void DependencyFramework :: free_memory() {
  int i, j;
  for (i = 0; i < Ps.size(); i++) {
    for (j = 0; j < Ps[i].size(); j++)
      delete Ps[i][j];
  }
  Ps.clear();
  for (i = 0; i < Rs.size(); i++) {
    for (j = 0; j < Rs[i].size(); j++)
      delete Rs[i][j];
  }
  Rs.clear();
  for (i = 0; i < problems.size(); i++) {
    delete problems[i];
  }
  problems.clear();
}

/* ========================== settings =========================== */

void DependencyFramework :: disable_graph() {
  allow_graph = false;
}

void DependencyFramework :: disable_subcrit() {
  allow_subcrit = false;
}

void DependencyFramework :: disable_polynomials() {
  allow_polynomials = false;
}

void DependencyFramework :: disable_product_polynomials() {
  allow_product_polynomials = false;
}

void DependencyFramework :: disable_horpo() {
  allow_horpo = false;
}

void DependencyFramework :: disable_usable() {
  allow_usable = allow_uwrt = false;
}

void DependencyFramework :: disable_formative() {
  allow_formative = allow_fwrt = false;
}

void DependencyFramework :: disable_uwrt() {
  allow_uwrt = false;
}

void DependencyFramework :: disable_fwrt() {
  allow_fwrt = false;
}

void DependencyFramework :: disable_abssimple() {
  abstraction_simple = false;
}

/* ================== creating dependency pairs ================== */

PTerm DependencyFramework :: up(PTerm term) {
  if (!term->query_head()->query_constant()) return term->copy();
  vector<PTerm> parts = term->split();
  PConstant f = dynamic_cast<PConstant>(parts[0]);
  if (arities[f->query_name()] != parts.size()-1) return term->copy();
  
  // make sure an upped version of f exists
  string upf = wout.up_symbol(f);
  if (!F.contains(upf)) {
    F.add(upf, F.query_type(f->query_name())->copy());
    arities[upf] = arities[f->query_name()];
  }
  else if (arities.find(upf) == arities.end())
    arities[upf] = arities[f->query_name()];
  
  PTerm ret = new Constant(upf, f->query_type()->copy());
  for (int i = 1; i < parts.size(); i++)
    ret = new Application(ret, parts[i]->copy());
  return ret;
}

void DependencyFramework :: add_pair(PTerm l, PTerm r, int style,
                                     vector<int> noneating,
                                     DPSet &DP) {

  if (style == 0) { l = up(l); r = up(r); }
  else { l = l->copy(); r = r->copy(); }

  DependencyPair *p = new DependencyPair(l, r, style);
  for (int i = 0; i+1 < noneating.size(); i += 2) {
    p->set_noneating(noneating[i], noneating[i+1]);
  }
  DP.push_back(p);
}

void DependencyFramework :: add_top_dp(PTerm l, PTerm r, DPSet &DP) {
  if (r->query_type()->query_data()) return;

  PTerm head = r->query_head();
  bool shouldadd = head->query_meta();
  shouldadd |= head->query_constant() &&
               defineds.find(head->to_string(false)) != defineds.end();
  if (!shouldadd) return;

  if (!l->query_type()->query_composed()) {
    vector<int> noneating;
    add_pair(l, r, 1, noneating, DP);
    return;
  }

  while (l->query_type()->query_composed()) {
    PVariable X = new Variable(l->query_type()->query_child(0)->copy());
    PTerm MX = new MetaApplication(X);
    PTerm MY = MX->copy();
    l = new Application(l->copy(), MX);
    r = new Application(r->copy(), MY);
    int style = l->query_type()->query_typevar() ? 1 : 0;
    DependencyPair *p = new DependencyPair(l, r, style);
    DP.push_back(p);
  }
}

void DependencyFramework :: add_normal_dp(PTerm l, PTerm r,
                                          vector<int> noneating,
                                          DPSet &DP) {
  PTerm head = r->query_head();
  
  // Cand(/\x.s) = Cand(s) if dynamic
  // Cand(/\x.s) = Cand(s[x:=X]) if static
  if (r->query_abstraction()) {
    if (static_flag == 0) {
      add_normal_dp(l, r->get_child(0), noneating, DP);
      return;
    }
    Abstraction *abs = dynamic_cast<Abstraction*>(r);
    PVariable x = abs->query_abstraction_variable();
    PVariable Z = new Variable(x->query_type()->copy());
    Substitution gamma;
    gamma[x] = new MetaApplication(Z);
    PTerm subt = abs->get_child(0)->copy();
    subt = subt->apply_substitution(gamma);
    add_normal_dp(l, subt, noneating, DP);
    delete subt;
    return;
  }

  // Cand(Z[]) = empty
  if (r->query_meta() && r->number_children() == 0) return;

  // right-hand side is a beta-reduct: look at its redex
  if (head->query_abstraction()) {
    string headpos = "";
    PTerm t = r->get_child(0);
    while (t->query_application()) {
      t = t->get_child(0);
      headpos += "1";
    }
    add_normal_dp(l, r->subterm(headpos + "2"), noneating, DP);
    Beta beta;
    PTerm rr = beta.apply(r->copy(), headpos);
    add_normal_dp(l, rr, noneating, DP);
    delete rr;
    return;
  }
  
  // cand(Z[s1...sn]s_{n+1}...sk) with k > 0 is the term itself
  // united with Cand(s1) U ... U Cand(sk)
  // if, however, we are looking at static DP, we do not include it
  if (head->query_meta()) {
    if (static_flag == 0) add_pair(l, r, 0, noneating, DP);
    vector<PTerm> parts = r->split();
    int i;
    for (i = 1; i < parts.size(); i++) {
      add_normal_dp(l, parts[i], noneating, DP);
    }
    MetaApplication *mapp = dynamic_cast<MetaApplication*>(head);
    noneating.push_back(mapp->get_metavar()->query_index());
    noneating.push_back(-1);
    for (int i = 0; i < mapp->number_children(); i++) {
      noneating[noneating.size()-1] = i;
      add_normal_dp(l, mapp->get_child(i), noneating, DP);
    }
    noneating.pop_back();
    noneating.pop_back();
  }
  
  // Cand(f(s1,...,sn)s_{n+1}...sk) is the term itself if f is a
  // defined symbol, united with Cand(s1) U ... U Cand(sk) in any
  // case
  if (head->query_constant() || head->query_variable()) {
    string name;
    if (head->query_constant()) name = head->to_string(false);
    else name = "";
    vector<PTerm> parts = r->split();
    if (defineds.find(name) != defineds.end()) {
      add_pair(l, r, 0, noneating, DP);
      if (name != "" && arities[name] < parts.size()-1) {
        add_normal_dp(l, r->get_child(1), noneating, DP);
        add_normal_dp(l, r->get_child(0), noneating, DP);
        return;
      }
    }
    for (int i = 1; i < parts.size(); i++)
      add_normal_dp(l, parts[i], noneating, DP);
  }
}

/* =============== analysis of the first-order part ============== */

bool DependencyFramework :: search_firstorder_counterexample(Ruleset &FOR,
                                                             string &explain) {
  string result = splitter.determine_nontermination(FOR, false, explain);
  if (result == "NO") return true;
  return false;
}

bool DependencyFramework :: first_order_part_terminating(Ruleset &R) {
  Ruleset FOR = splitter.first_order_part(R);

  if (FOR.size() == 0) return true;

  bool orthogonal = leftlinear && manip.fully_extended(R) &&
                    !manip.has_critical_pairs(R);

  wout.start_method("first-order tool");
  wout.print("We observe that the rules contain a first-order subset:\n");
  wout.print_rules(FOR, F, arities);

  wout.print("Moreover, the system is " +
    string((orthogonal ? "orthogonal" : "finitely branching")) +
    ".  Thus, by " + wout.cite("Kop12", "Thm. 7.55") + ", we may "
    "omit all first-order dependency pairs from the dependency pair "
    "problem (DP(R), R) if this first-order part is " +
    (orthogonal ? "" : "Ce-") + "terminating when seen as a "
    "many-sorted first-order TRS.\n");

  // not orthogonal: we need Ce-termination, so add the rules
  // PAIR x y -> x and PAIR x y -> y
  if (!orthogonal) {
    PType type = new ComposedType(new DataType("o"),
            new ComposedType(new DataType("o"),new DataType("o")));
    PConstant pair = new Constant("~PAIR", type);
    PTerm x = new MetaApplication(new Variable(new DataType("o")));
    PTerm y = new MetaApplication(new Variable(new DataType("o")));
    PTerm l = new Application(new Application(pair,x),y);
    MatchRule *rule1 = new MatchRule(l->copy(), x->copy());
    MatchRule *rule2 = new MatchRule(l->copy(), y->copy());
    FOR.push_back(rule1);
    FOR.push_back(rule2);
    delete l;
  }

  wout.verbose_print("Sending...\n");
  string reason;
  string result = splitter.determine_termination(FOR, false, reason);
  
  // delete those two new rules (they aren't referenced elsewhere)
  if (!orthogonal) {
    delete FOR[FOR.size()-1];
    FOR.pop_back();
    delete FOR[FOR.size()-1];
    FOR.pop_back();
  }

  if (result == "YES") {
    FOstatus = 1;
    string prover = splitter.query_tool_name();
    if (prover == "firstorderprover")
      prover = "the external first-order termination prover";
    wout.print("According to " + prover + ", this system is indeed " +
      string(orthogonal ? "" : "Ce-") + "terminating:\n");
    wout.start_box();
    wout.literal_print(reason);
    wout.end_box();
    wout.succeed_method("first-order tool");
    return true;
  }

  if (orthogonal && result == "NO") {
    FOstatus = 2;
    found_counterexample = true;
    orthogonality_for_counter = true;
    counterexample = reason;
  }

  else if ( //result == "NO" &&    // <-- outcommented because we may use a
                                   // terminationtool that is weak at NT
           search_firstorder_counterexample(FOR, counterexample)) {
    FOstatus = 2;
    found_counterexample = true;
    orthogonality_for_counter = false;
  }

  else {
    wout.verbose_print("However, the first-order tool returns " +
                       result + ".\n");
  }
  
  wout.abort_method("first-order tool");

  return false;
}

/* ================= termination analysis: basics ================ */

void DependencyFramework :: setup(bool skipFO) {
  int i;

  // copy and complete the rules
  Ruleset R;
  for (i = 0; i < original_rules.size(); i++) {
    R.push_back(new MatchRule(original_rules[i]->query_left_side()->copy(),
                              original_rules[i]->query_right_side()->copy()));
  }
  R = manip.beta_saturate(R);

  // calculate dependency pairs of non-first-order rules
  DPSet DP;
  for (i = 0; i < R.size(); i++) {
    if (skipFO && splitter.first_order(R[i])) continue;

    vector<int> noneating;
    add_top_dp(R[i]->query_left_side(), R[i]->query_right_side(), DP);
    add_normal_dp(R[i]->query_left_side(), R[i]->query_right_side(),
                  noneating, DP);
  }

  // and initialise the very first dependency pair problem!
  Ps.push_back(DP);
  Rs.push_back(R);
  problems.push_back(new DPProblem(0,0));
}

void DependencyFramework :: user_information() {
  // say what we're doing
  string start = "We use the dependency pair framework as "
    "described in " + wout.cite("Kop12", "Ch. 6/7") + ", with ";
  if (static_flag == 0) start += "dynamic dependency pairs";
  else {
    start += "static dependency pairs (see " +
      wout.cite("KusIsoSakBla09") + " and the adaptation for ";
    if (static_flag == 1) {
      start += "AFSMs in " + wout.cite("Kop12", "Ch. 7.8");
    }
    else {
      start += "AFSMs and accessible arguments in " +
        wout.cite("FuhKop19");
    }
    start += ")";
  }
  wout.print(start + ".\n");

  // explain about eta-expansion
  if (expanded) {
    wout.print("In order to do so, we start by " + wout.eta_symbol() +
      "-expanding the system, which gives:\n");
    wout.print_rules(Rs[0], F, arities);
  }

  // explain about completion
  if (original_rules.size() < Rs[0].size()) {
    wout.print("To start, the system is " + wout.beta_symbol () +
      "-saturated by adding the following rules:\n");
    vector<MatchRule*> tmp;
    tmp.insert(tmp.begin(), Rs[0].begin()+original_rules.size(), Rs[0].end());
    wout.print_rules(tmp, F, arities);
  }
}

void DependencyFramework :: print_problem(DPProblem *prob) {
  wout.start_box();
  wout.print("Dependency Pairs P" + wout.sub(wout.str(prob->P)) + ":\n");
  wout.print_DPs(Ps[prob->P], F, arities, true);
  wout.print("Rules R" + wout.sub(wout.str(prob->R)) + ":\n");
  wout.print_rules(Rs[prob->R], F, arities);
  wout.end_box();
}

string DependencyFramework :: print_problem_brief(DPProblem *prob) {
  return "(P_" + wout.str(prob->P) + ", R_" + wout.str(prob->R) +
         ", " + string(static_flag == 2 ? "computable" : "minimal") +
         ", " + string(formative_flag ? "formative" : "all") + ")";
}

bool DependencyFramework :: terminating() {
  // don't do dynamic dependency pairs if we're forced to static!
  if (static_flag == 0 && !allow_dynamic) {
    if (allow_static) return force_static_approach();
    else return false;
  }

  string method;
  if (static_flag > 0) method = "static dependency pairs";
  else method = "dynamic dependency pairs";
  wout.start_method(method);

  // deal with first-order rules; note: (DP(R), R) = problems[0]
  bool FOterminating;
  if (FOstatus == 1) FOterminating = true;
  else if (FOstatus == 2) FOterminating = false;
  else FOterminating = first_order_part_terminating(original_rules);
  if (found_counterexample) {
    wout.abort_method(method);
    return false;
  }

  // create dependency pairs and problems
  setup(FOterminating);
  user_information();

  // and show them!
  bool specialcollapsing = false;
  for (int i = 0; i < Ps[0].size(); i++) {
    if (Ps[0][i]->query_right()->query_application() &&
        Ps[0][i]->query_right()->query_head()->query_meta())
      specialcollapsing = true;
  }
  if (specialcollapsing) {
    wout.print("After applying " + wout.cite("Kop12", "Thm. 7.22") +
      " to denote collapsing dependency pairs in an extended form, "
      "we");
  }
  else wout.print("We");
  wout.print(" thus obtain the following dependency pair problem " +
    print_problem_brief(problems[0]) + ":\n");
  print_problem(problems[0]);

  bool ret = termination_loop();

  if (ret) wout.succeed_method(method);
  else {
    wout.abort_method(method);
    if (static_flag == 0 && allow_static)
      return force_static_approach();
  }

  return ret;
}

bool DependencyFramework :: force_static_approach() {
  int i;

  // copy the rules
  int firstordersize = splitter.first_order_part(original_rules).size();
  Ruleset R;
  for (i = 0; i < original_rules.size(); i++)
    R.push_back(new MatchRule(original_rules[i]->query_left_side()->copy(),
                              original_rules[i]->query_right_side()->copy()));
  
  // eta-expand the rules
  if (!manip.eta_long(R, true)) R = manip.eta_expand(R);
  for (i = 0; i < R.size() && i < original_rules.size(); i++) {
    if (!R[i]->query_left_side()->equals(original_rules[i]->query_left_side()))
      expanded = true;
    if (!R[i]->query_right_side()->equals(original_rules[i]->query_right_side()))
      expanded = true;
  }
  arities = manip.get_arities(F, R);

  // check whether we've actually been given a PFP system
  if (!manip.plain_function_passing(R, arities, sortordering)) {
    wout.verbose_print("Nor can we succeed with static dependency "
      "pairs, as the (eta-expanded) rules are not plain function "
      "passing.\n");
    for (i = 0; i < R.size(); i++) delete R[i];
    return false;
  }
  if (sortordering.size() == 0) static_flag = 1;
  else static_flag = 2;
  wout.verbose_print("Thus, instead we move to the static case.\n");

  // check whether this caused a different first-order part
  splitter = FirstOrderSplitter(F, R, splitter.query_tool(),
                                splitter.query_non_tool());
  int newsize = splitter.first_order_part(R).size();
  if (newsize != firstordersize) FOstatus = 0;

  free_memory();
  Ruleset backup = original_rules;
  original_rules = R;
  bool ret = terminating();
  original_rules = backup;
  for (i = 0; i < R.size(); i++) delete R[i];
  return ret;
}

void DependencyFramework :: list_problems() {
  string problemslist = "";
  for (int i = 0; i < problems.size(); i++) {
    if (i > 0 && i == problems.size()-1) problemslist += " and ";
    else if (i > 0) problemslist += ", ";
    problemslist += print_problem_brief(problems[i]);
  }

  wout.print("Thus, the original system is terminating if ");
  if (problems.size() > 1) wout.print("each of ");
  wout.print(problemslist + " is finite.\n");
}

bool DependencyFramework :: termination_loop() {
  bool graph_optimal = false;

  // always problems 0..current-1 are assumed to be graph-optimal,
  // but perhaps the last problem is not, yet
  while (problems.size() > 0) {
    list_problems();
    DPProblem *prob = problems[problems.size()-1];
    problems.pop_back();
    wout.print("We consider the dependency pair problem " +
      print_problem_brief(prob) + ".\n");

    int old_size = problems.size();
    bool unsetoptimal = false;

    if (!graph_optimal && graph_processor(prob)) graph_optimal = true;
    else if (empty_processor(prob)) {}
    else if (subcrit_processor(prob)) unsetoptimal = true;
    else if (static_processor(prob)) unsetoptimal = true;
    else if (formative_processor(prob)) unsetoptimal = true;
    else if (redpair_processor(prob)) unsetoptimal = true;
    
    else {
      problems.push_back(prob);
      wout.verbose_print("Unfortunately, none of the methods we "
        "attempted could succeed in simplifying this problem.\n");
      return false;
    }
    if (unsetoptimal && problems.size() != old_size)
      graph_optimal = false;
    delete prob;
  }

  wout.print("As all dependency pair problems were succesfully "
    "simplified with sound (and complete) processors until nothing "
    "remained, we conclude termination.\n");

  return true;
}

/* ================== dependency pair processors ================= */

bool DependencyFramework :: graph_processor(DPProblem *prob) {
  if (!allow_graph) return false;

  DependencyGraph graph(F, Ps[prob->P], Rs[prob->R]);
  vector<DPSet> sccs = graph.get_sccs();

  if (sccs.size() == 1 && sccs[0].size() == Ps[prob->P].size()) {
    return false;
  }

  wout.start_method("dependency graph");
  wout.print("We place the elements of P in a dependency graph "
    "approximation G (see e.g. " +
    wout.cite("Kop12", "Thm. 7.27, 7.29") + ", as follows:\n");
  graph.print_self();

  if (sccs.size() == 0) {
    wout.print("This graph has no strongly connected components.  "
      "By " + wout.cite("Kop12", "Thm. 7.31") + ", this implies "
      "finiteness of the dependency pair problem.\n");
    wout.succeed_method("dependency graph");
    return true;
  }

  wout.print("This graph has the following strongly connected "
    "components:\n");
  wout.start_box();

  string newprobs = "";
  string Rstring = "R_" + wout.str(prob->R);
  int i;
  for (i = 0; i < sccs.size(); i++) {
    int Pnum = Ps.size();
    wout.print("P_" + wout.str(Pnum) + ":\n");
    wout.print_DPs(sccs[i], F, arities, false);
    for (int j = 0; j < sccs[i].size(); j++)
      sccs[i][j] = sccs[i][j]->copy();
    Ps.push_back(sccs[i]);
    problems.push_back(new DPProblem(Pnum, prob->R));
    string probstring = "(P_" + wout.str(Pnum) + ", " + Rstring + ", m, f)";
    if (i == 0) newprobs = probstring;
    else if (i == sccs.size()-1) newprobs += " and " + probstring;
    else newprobs += ", " + probstring;
  }

  wout.end_box();
  wout.print("By " + wout.cite("Kop12", "Thm. 7.31") + ", we may " +
    "replace any dependency pair problem (P_" + wout.str(prob->P) +
    ", " + Rstring + ", m, f) by " + newprobs + ".\n");

  wout.succeed_method("dependency graph");
  return true;
}

bool DependencyFramework :: empty_processor(DPProblem *prob) {
  if (Ps[prob->P].size() == 0) {
    wout.print("By the empty set processor " +
      wout.cite("Kop12", "Thm. 7.15") + " this problem may be "
      "immediately removed.\n");
    return true;
  }
  return false;
}

bool DependencyFramework :: subcrit_processor(DPProblem *prob) {
  if (!allow_subcrit) return false;

  SubtermCriterionChecker scchecker;
  DPSet strict, equal;
  if (!scchecker.run(Ps[prob->P], strict, equal, F, arities))
    return false;

  string Pstring = "P_" + wout.str(prob->P);
  string Rstring = "R_" + wout.str(prob->R);
  string theorem = wout.cite("FuhKop19", "Thm. 61");
  string minflag = "minimal";
  if (static_flag == 2) {
    minflag = "computable";
  }

  if (equal.size() == 0) {
    wout.print("By " + theorem + ", we may replace a dependency pair "
      "problem (" + Pstring + ", " + Rstring + ", " + minflag + ", f) "
      "by (" + wout.empty_set_symbol() + ", " + Rstring + ", " +
      minflag + ", f).  By the empty set processor " +
      wout.cite("Kop12", "Thm. 7.15") + " this problem may be "
      "immediately removed.\n");
  }
  else {
    DPSet newset;
    for (int i = 0; i < equal.size(); i++) {
      newset.push_back(equal[i]->copy());
    }
    int num = Ps.size();
    Ps.push_back(newset);
    
    string newPstring = "P_" + wout.str(num);
    wout.print("By " + theorem + ", we may replace a dependency pair "
      "problem (" + Pstring + ", " + Rstring + ", " + minflag + ", f) "
      "by (" + newPstring + ", " + Rstring + ", " + minflag + ", f), "
      "where " + newPstring + " contains:\n");
    wout.print_DPs(newset, F, arities, false);

    problems.push_back(new DPProblem(num, prob->R));
  }

  return true;
}

bool DependencyFramework :: static_processor(DPProblem *prob) {
  if (static_flag != 2) return false;
  if (!allow_subcrit) return false;

  SubtermCriterionChecker scchecker;
  DPSet strict, equal;
  if (!scchecker.run_static(Ps[prob->P], strict, equal, F, arities,
                            sortordering))
    return false;

  string Pstring = "P_" + wout.str(prob->P);
  string Rstring = "R_" + wout.str(prob->R);

  if (equal.size() == 0) {
    wout.print("By " + wout.cite("FuhKop19", "Thm. 63") + ", we may replace "
      "a dependency pair problem (" + Pstring + ", " + Rstring +
      ", computable, f) by (" + wout.empty_set_symbol() + ", " + Rstring +
      ", computable, f).  By the empty set processor " + wout.cite("Kop12",
      "Thm. 7.15") + " this problem may be immediately removed.\n");
  }
  else {
    DPSet newset;
    for (int i = 0; i < equal.size(); i++) {
      newset.push_back(equal[i]->copy());
    }
    int num = Ps.size();
    Ps.push_back(newset);
    
    string newPstring = "P_" + wout.str(num);
    wout.print("By " + wout.cite("FuhKop19", "Thm. 7.6") + ", we may "
      "replace a dependency pair problem (" + Pstring + ", " +
      Rstring + ", computable, f) by (" + newPstring + ", " + Rstring +
      ", computable, f), where " + newPstring + " contains:\n");
    wout.print_DPs(newset, F, arities, false);

    problems.push_back(new DPProblem(num, prob->R));
  }

  return true;
}

bool DependencyFramework :: formative_processor(DPProblem *prob) {
  if (!allow_formative || !formative_flag) return false;

  int i, j;

  // calculate formative rules, abort if they're just the same as the
  // usual rules
  Ruleset R = Rs[prob->R];
  Ruleset FR = manip.formative_rules(Ps[prob->P], R);
  if (FR.size() == R.size()) {
    for (i = 0; i < FR.size(); i++) delete FR[i];
    return false;
  }

  // determine whether we've seen this set of formative rules before;
  // if not, just add it to the Rs list
  int FRnum = -1;
  for (i = 0; i < Rs.size() && FRnum == -1; i++) {
    if (Rs[i].size() != FR.size()) continue;
    bool equal = true;
    for (j = 0; j < FR.size() && equal; j++) {
      PTerm left1 = FR[j]->query_left_side();
      PTerm left2 = Rs[i][j]->query_left_side();
      PTerm right1 = FR[j]->query_right_side();
      PTerm right2 = Rs[i][j]->query_right_side();
      if (!left1->equals(left2)) equal = false;
      else if (!right1->equals(right2)) equal = false;
    }
    if (equal) FRnum = i;
  }
  bool added = FRnum == -1;
  if (FRnum == -1) {
    FRnum = Rs.size();
    Rs.push_back(FR);
  }
  else {
    for (i = 0; i < FR.size(); i++) delete FR[i];
  }

  // tell the user what we've been doing!
  if (Rs[FRnum].size() == 0) {
    wout.print("This combination (P_" + wout.str(prob->P) + ", R_" +
      wout.str(prob->R) + ") has no formative rules!");
    if (added) wout.print("  We will name the empty set of rules:");
    else wout.print("  This is exactly ");
    wout.print("R_" + wout.str(FRnum) + ".\n");
  }
  else {
    wout.print("The formative rules of (P_" + wout.str(prob->P) +
      ", R_" + wout.str(prob->R) + ") are ");
    if (added) wout.print("R_" + wout.str(FRnum) + " ::=\n");
    else wout.print("exactly R_" + wout.str(FRnum) + ":\n");
    wout.print_rules(Rs[FRnum], F, arities);
  }

  DPProblem *newprob = new DPProblem(prob->P, FRnum);
  problems.push_back(newprob);

  wout.print("By " + wout.cite("Kop12", "Thm. 7.17") + ", we may "
    "replace the dependency pair problem " +
    print_problem_brief(prob) + " by " + print_problem_brief(newprob)
    + ".\n");

  return true;
}

void DependencyFramework :: add_reduced_problem(DPProblem *prob,
                                                vector<int> &ok) {
  set<int> removed;
  int i;

  wout.print("By the observations in " + wout.cite("Kop12",
    "Sec. 6.6") + ", this reduction pair suffices; we may thus "
    "replace ");

  if (ok.size() == Ps[prob->P].size()) {
    wout.print("a dependency pair problem (P_" + wout.str(prob->P) +
      ", R_" + wout.str(prob->R) + ") by (" + wout.empty_set_symbol() +
      ", R_" + wout.str(prob->R) + ").  By the empty set processor " +
      wout.cite("Kop12", "Thm. 7.15") + " this problem may be "
      "immediately removed.\n");
  }
  else {
    // determine the new dependency pair set
    for (i = 0; i < ok.size(); i++) removed.insert(ok[i]);
    DPSet Pnew;
    for (i = 0; i < Ps[prob->P].size(); i++) {
      if (removed.find(i) != removed.end()) continue;
      Pnew.push_back(Ps[prob->P][i]->copy());
    }
    int Pnum = Ps.size();
    Ps.push_back(Pnew);

    // determine the new problem
    DPProblem *newprob = new DPProblem(Pnum, prob->R);
    problems.push_back(newprob);
    
    // and comment!
    wout.print("the dependency pair problem " +
      print_problem_brief(prob) + " by " +
      print_problem_brief(newprob) + ", where P_" + wout.str(Pnum) +
      " consists of:\n");
    wout.print_DPs(Pnew, F, arities, false);
  }
}

bool DependencyFramework :: redpair_processor(DPProblem *prob) {
  Ruleset UR;
  if (allow_usable) UR = manip.usable_rules(Ps[prob->P], Rs[prob->R]);
  else UR = Rs[prob->R];

  bool tagged = formative_flag && abstraction_simple;
  DPOrderingProblem *op =
    new DPOrderingProblem(Ps[prob->P], UR, F, tagged,
                          allow_uwrt, allow_fwrt);

  wout.start_method("redpair");

  // explain what we've been doing
  if (UR.size() != Rs[prob->R].size()) {
    wout.print("We will use the reduction pair processor with usable "
      "rules " + wout.cite("Kop12", "Thm. 7.44") + ".  ");
    if (UR.size() > 0) {
      wout.print("The usable rules of (P_" + wout.str(prob->P) +
        ", R_" + wout.str(prob->R) + ") are:\n");
      wout.print_rules(UR, F, arities);
    }
    else wout.print("(P_" + wout.str(prob->P) + ", R_" +
      wout.str(prob->R) + ") has no usable rules.\n");
  }
  else {
    wout.print("We will use the reduction pair processor " +
      wout.cite("Kop12", "Thm. 7.16") + ".  ");
  }
  if (tagged) {
    wout.print("As the system is abstraction-simple and the "
      "formative flag is set, it suffices to find a tagged reduction "
      "pair " + wout.cite("Kop12", "Def. 6.70") + ".  ");
  }
  else {
    wout.print("It suffices to find a standard reduction pair " +
      wout.cite("Kop12", "Def. 6.69") + ".  ");
  }
  if (op->changed_arities(arities)) {
    wout.print("By " + wout.cite("Kop12", "Thm. 2.26") + ", we can "
      "freely change the arities in the requirements this gives, "
      "choosing arities as high as possible without conflicts.\n");
  }
  wout.print("Thus, we must orient:\n");
  op->print();

  // use argument functions
  if (!allow_uwrt && !allow_fwrt) op->simple_argument_functions();
    // TODO: in the future we will want both argument functions and
    // usable / formative rules wrt to work; this will require just
    // some restructuring, in that the requirements for orientation
    // must be added AFTER executing argument functions!

  bool success = poly_processor(prob, op, false) ||
                 horpo_processor(prob, op, false) ||
                 (allow_product_polynomials && poly_processor(prob, op, true));
  delete op;
  if (success) wout.succeed_method("redpair");
  else wout.abort_method("redpair");
  return success;
}

bool DependencyFramework :: poly_processor(DPProblem *prob,
                               DPOrderingProblem *ord, bool pprod) {
  if (!allow_polynomials) return false;
  pprod &= allow_product_polynomials;

  wout.start_method("poly redpair");

  // use polynomial interpretations
  PolyModule pols;
  pols.set_use_products(pprod);
  vector<int> ok = pols.orient(ord);

  if (ok.size() != 0) {
    add_reduced_problem(prob, ok);
    wout.succeed_method("poly redpair");
    return true;
  }
  else {
    wout.verbose_print("Unfortunately, our attempt with POLY met "
      "with little succes.\n");
    wout.abort_method("poly redpair");
    return false;
  }
}

bool DependencyFramework :: horpo_processor(DPProblem *prob,
                               DPOrderingProblem *ord, bool etaall) {
  if (!allow_horpo) return false;

  wout.start_method("horpo redpair");
  if (ord->meta_deextend_stricts()) {
    wout.print("Since this representation is not advantageous for "
      "the higher-order recursive path ordering, we present the "
      "strict requirements in their unextended form, which is "
      "not problematic since for any F, s and substituion " +
      wout.gamma_symbol() + ": (F s)" + wout.gamma_symbol() + " " +
      wout.beta_symbol() + "-reduces to F(s)" + wout.gamma_symbol() +
      ".)\n");
  }

  /*
  if (we are supposed to eta-extend everything first) {
    bool monomorphic = true;
    bool worthit = false;
    int i;
    for (i = 0; i < reqs.size() && monomorphic; i++) {
      if (reqs[i]->right->query_type()->query_composed() &&
          !reqs[i]->right->query_abstraction()) worthit = true;
      if (!manip.monomorphic(reqs[i]->left)) monomorphic = false;
      if (!manip.monomorphic(reqs[i]->right)) monomorphic = false;
    }
    if (!monomorphic || !worthit) {
      free_redpair_junk(reqs);
      wout.verbose_print("Actually, we already did that, never mind.\n");
      wout.abort_method("horpo redpair");
      return false;
    }
    modif.eta_expand(reqs);
    new_arities.clear();
    for (i = 0; i < reqs.size(); i++) {
      reqs[i]->left->adjust_arities(new_arities);
      reqs[i]->right->adjust_arities(new_arities);
    }
    modif.eta_everything(reqs, new_arities);
    wout.print("We do a complete (blunt) eta-expansion of the "
      "requirements, obtaining:\n");
    wout.print_requirements(reqs, F, new_arities);
  }
  */

  // use a recursive path ordering
  Horpo horpo;
  vector<int> ok = horpo.orient(ord);

  // undo the meta-deextension
  ord->meta_extend_stricts();

  if (ok.size() != 0) {
    add_reduced_problem(prob, ok);
    wout.succeed_method("horpo redpair");
    return true;
  }
  else {
    wout.verbose_print("Unfortunately, our attempt with HORPO met "
      "with little succes.\n");
    wout.abort_method("horpo redpair");
    return false;
  }
}

/* =================== non-termination analysis ================== */

bool DependencyFramework :: first_order_non_terminating() {
  if (FOstatus == 0) first_order_part_terminating(original_rules);
  return found_counterexample;
}

bool DependencyFramework :: proved_non_terminating() {
  return found_counterexample;
}

void DependencyFramework :: document_non_terminating() {
  if (orthogonality_for_counter) {
    string disprover = splitter.query_tool_name();
    if (disprover == "firstorderprover")
      disprover = "our external first-order termination checker";
    wout.print("Since the system is orthogonal, its first-order "
      "part is non-terminating if it is innermost non-terminating "
      "by " + wout.cite("Gra95") + ", which is the case if it is "
      "innermost non-terminating without regarding types by " +
      wout.cite("FuhGieParSchSwi11") + ".  Again by " +
      wout.cite("Gra95") + " (this time applied without types), "
      "we thus obtain non-terminating if the restriction to its "
      "first-order sub-set is non-terminating without regarding "
      "types.  According to " + disprover + " we have exactly "
      "that:\n");
  }
  else {
    string disprover = splitter.query_non_tool_name();
    if (disprover == "firstordernonprover")
      disprover = "our external first-order non-termination checker";
    wout.print("This system is non-terminating, as demonstrated "
      "by " + disprover + ":\n");
  }

  wout.start_box();
  wout.literal_print(counterexample);
  wout.end_box();
}

