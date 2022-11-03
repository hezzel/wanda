/**************************************************************************
   Copyright NEWYEAR Cynthia Kop

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

#include "orderingproblem.h"
#include "formula.h"
#include "outputmodule.h"
#include "substitution.h"
#include "rulesmanipulator.h"

OrderingProblem :: OrderingProblem(DPSet &dps, Ruleset &rules,
                                   Alphabet &F, int uprop) {
  get_actual_arities(dps, rules);
  unfiltered_properties = uprop;

  vector<string> symbs = F.get_all();
  for (int i = 0; i < symbs.size(); i++)
    alphabet.add(symbs[i], F.query_type(symbs[i])->copy());
}

OrderingProblem :: OrderingProblem(Ruleset &rules, Alphabet &F) {
  DPSet dps;
  get_actual_arities(dps, rules);
  unfiltered_properties = 2;

  vector<string> symbs = F.get_all();
  for (int i = 0; i < symbs.size(); i++)
    alphabet.add(symbs[i], F.query_type(symbs[i])->copy());
}

OrderingProblem :: ~OrderingProblem() {
  for (int i = 0; i < constraints.size(); i++) delete constraints[i];
  for (int j = 0; j < reqs.size(); j++) delete reqs[j];
}

void OrderingProblem :: get_actual_arities(DPSet &dps,
                                           Ruleset &rules) {
  int i;
  for (i = 0; i < dps.size(); i++) {
    dps[i]->query_left()->adjust_arities(arities);
    dps[i]->query_right()->adjust_arities(arities);
  }
  for (i = 0; i < rules.size(); i++) {
    rules[i]->query_left_side()->adjust_arities(arities);
    rules[i]->query_right_side()->adjust_arities(arities);
  }
}

bool OrderingProblem :: changed_arities(map<string,int> original) {
  for (map<string,int>::iterator it = arities.begin();
       it != arities.end(); it++) {
    if (it->second < original[it->first]) return true;
  }
  return false;
}

void OrderingProblem :: require_atleastone(int start, int end) {
  Or *req = new Or();
  for (int i = start; i <= end; i++) req->add_child(new Var(i));
  constraints.push_back(req);
}

void OrderingProblem :: set_filterable(string f) {
  int ar = arities[f];
  filterable[f] = vars.query_size();
  vars.add_vars(ar);
  for (int i = 1; i <= ar; i++) {
    vars.set_description(filtered_variable(f, i),
      "ArgFiltered[" + f + "," + wout.str(i) + "]");
  }
}

int OrderingProblem :: filtered_variable(string f, int index) {
  if (index < 1 || index > arities[f]) return -1;
  if (filterable.find(f) == filterable.end()) return vars.false_var();
  return filterable[f] + index - 1;
}

PFormula OrderingProblem :: is_filtered_away(string f, int index) {
  int var = filtered_variable(f, index);
  if (var == -1) {
    wout.print("ERROR! Illegal filter position " + f + ": " + wout.str(index) + "!\n");
    wout.print("arities[f] = " + wout.str(arities[f]) + "\n");
    return new Bottom;
  }
  return new Var(var);
}

PFormula OrderingProblem :: not_filtered_away(string f, int index) {
  int var = filtered_variable(f, index);
  if (var == -1) {
    wout.print("ERROR! Illegal filter position!\n");
    return new Top;
  }
  return new AntiVar(var);
}

void OrderingProblem :: set_unfiltered_properties(int value) {
  unfiltered_properties = value;
}

bool OrderingProblem :: unfilterable(string symbol) {
  return filterable.find(symbol) == filterable.end();
}

bool OrderingProblem :: unfilterable_application() {
  return unfiltered_properties > 0;
}

bool OrderingProblem :: unfilterable_strongly_monotonic() {
  return unfiltered_properties == 2;
}

bool OrderingProblem :: unfilterable_substeps_permitted() {
  return unfiltered_properties == 1;
}

int OrderingProblem :: arity(string symbol) {
  return arities[symbol];
}

int OrderingProblem :: arity(PConstant symbol) {
  return arities[symbol->query_name()];
}

PType OrderingProblem :: symbol_type(string symbol) {
  return alphabet.query_type(symbol);
}

string OrderingProblem :: print_term(PTerm term, map<int,string> env,
                      map<int,string> freename, map<int,string> boundname) {
  return wout.print_term(term, arities, alphabet, env, freename, boundname);
}

const vector<PFormula> OrderingProblem :: query_constraints() {
  return constraints;
}

const vector<OrderRequirement*> OrderingProblem :: orientables() {
  return reqs;
}

vector<int> OrderingProblem :: strictly_oriented() {
  vector<int> ret;
  for (int i = 0; i < reqs.size(); i++) {
    if (!reqs[i]->definite_requirement()) continue;
    if (reqs[i]->condition_valuation() == TRUE) {
      for (int j = 0; j < reqs[i]->data_total(); j++)
        ret.push_back(reqs[i]->get_data(j));
    }
  }
  return ret;
}

void OrderingProblem :: justify_orientables() {
  // don't do anything
}

void OrderingProblem :: print() {
  wout.start_table();
  for (int i = 0; i < reqs.size(); i++) {
    map<int,string> metanaming, freenaming, boundnaming;
    vector<string> entry;
    Valuation val = reqs[i]->condition_valuation();
    entry.push_back(wout.print_term(reqs[i]->left, arities, alphabet,
                    metanaming, freenaming, boundnaming));
    if (reqs[i]->definite_requirement()) {
      if (val == TRUE) entry.push_back(wout.gterm_symbol());
      else if (val == FALSE) entry.push_back(wout.geqterm_symbol());
      else entry.push_back(wout.geqorgterm_symbol());
    }
    else {
      if (val == FALSE) continue;
      entry.push_back(wout.geqterm_symbol());
    }
    entry.push_back(wout.print_term(reqs[i]->right, arities, alphabet,
                    metanaming, freenaming, boundnaming));
    wout.table_entry(entry);
  }
  wout.end_table();
}


PlainOrderingProblem :: PlainOrderingProblem(Ruleset &rules, Alphabet &F)
  :OrderingProblem(rules, F) {
  
  // nothing may be filtered, so just add constraints
  int base = vars.query_size();
  vars.add_vars(rules.size());
  for (int i = 0; i < rules.size(); i++) {
    PTerm left = rules[i]->query_left_side()->copy();
    PTerm right = rules[i]->query_right_side()->copy();
    int v = base + i;
    vars.set_description(v, "StrictRule[" + wout.str(i+1) + "]");
    reqs.push_back(OrderRequirement::maybe_greater(left, right, v, i));
  }
  require_atleastone(base, vars.query_size() - 1);
}


DPOrderingProblem :: DPOrderingProblem(DPSet &dps, Ruleset &rules,
                                       Alphabet &F, bool &use_tagging,
                                       bool use_usable, bool use_formative)
  :OrderingProblem(dps, rules, F, 1), Ccounter(0), metas_extended(true) {
  
  int i;
  map<string,int>::iterator it;
  bool col;

  // no matter what kind of ordering problem this is, we always need
  // to add requirements for the dependency pairs and the rules
  make_dp_requirements(dps);
  make_rule_requirements(rules);

  col = collapsing(dps);
  use_usable &= !col;
  use_tagging &= col;

  // in the uncollapsing case, filtering is essentially unrestricted
  // (whether or not use_tagging is indicated; this is better!)
  if (!col) handle_collapsing_case();

  // in the case of collapsing DPs, where we aren't allowed to tag,
  // we can only filter the upped symbols, and we'll need "rules" of
  // the form f(x1,...,xn) -> f#(x1,...,xn)
  else if (!use_tagging) handle_untagged_case(dps);

  // in the case of collapsing DPs when we are allowed to use tagging,
  // we can filter all basic symbols, but replace some symbols in the
  // requirements by tagged symbols, which cannot be filtered
  else handle_tagged_case();

  // for now, we want to force all rules; later, we'll do usable and
  // formative rules for this
  if (use_usable) {
    add_usable_rules_requirements();
    used_wrt = "usable rules";
  }
  else {
    for (int i = 0; i < reqs.size(); i++) {
      if (!reqs[i]->definite_requirement())
        constraints.push_back(reqs[i]->orient_geq());
    }
    used_wrt = "";
  }
}

PConstant DPOrderingProblem :: fresh_constant(PType type) {
  while (true) {
    string name = "~c" + wout.str(Ccounter++);
    if (!alphabet.contains(name)) {
      arities[name] = 0;
      alphabet.add(name, type->copy());
      return new Constant(name, type->copy());
    }
  }
}

bool DPOrderingProblem :: collapsing(DPSet &dps) {
  for (int i = 0; i < dps.size(); i++) 
    if (dps[i]->query_right()->query_head()->query_meta())
      return true;
  return false;
}

int DPOrderingProblem :: find_meta_arity(PTerm term, PVariable Z) {
  if (term->query_meta()) {
    PVariable X = dynamic_cast<MetaApplication*>(term)->get_metavar();
    if (X->equals(Z)) return term->number_children();
  }
  for (int i = 0; i < term->number_children(); i++) {
    int attempt = find_meta_arity(term->get_child(i), Z);
    if (attempt != -1) return attempt;
  }
  return -1;
}

PTerm DPOrderingProblem :: meta_extend(int index, PTerm right) {
  if (!right->query_application()) return right->copy();
  if (!right->query_head()->query_meta()) return right->copy();

  vector<PTerm> parts = right->split();
  vector<PTerm> children;
  int i;

  meta_arities[index] = parts[0]->number_children();
  
  for (i = 0; i < parts[0]->number_children(); i++)
    children.push_back(parts[0]->get_child(i)->copy());
  for (i = 1; i < parts.size(); i++) 
    children.push_back(parts[i]->copy());
  PVariable Z = dynamic_cast<MetaApplication*>(parts[0])->get_metavar();
  Z = dynamic_cast<PVariable>(Z->copy());
  return new MetaApplication(Z, children);
}

PTerm DPOrderingProblem :: meta_alter(int index, PTerm term) {
  if (meta_arities.find(index) == meta_arities.end()) return term;

  PVariable Z;
  int current_arity;
  int new_arity = meta_arities[index];
  vector<PTerm> parts;
  PTerm backup = term;
  int i;

  // get meta-variable and application parts from term
  while (term->query_application()) {
    parts.push_back(term->replace_child(1, NULL));
    term = term->get_child(0);
  }
  Z = dynamic_cast<MetaApplication*>(term)->get_metavar();
  Z = dynamic_cast<PVariable>(Z->copy());
  current_arity = term->number_children();
  for (i = current_arity-1; i >= 0; i--)
    parts.push_back(term->replace_child(i, NULL));
  delete backup;
  meta_arities[index] = current_arity;

  // split into new meta-variable parts and application parts
  vector<PTerm> children, rest;
  for (i = 0; i < parts.size(); i++) {
    int k = parts.size()-i-1;
    if (i < new_arity) children.push_back(parts[k]);
    else rest.push_back(parts[k]);
  }

  // make meta-variable application, and apply it on the rest!
  term = new MetaApplication(Z, children);
  for (i = 0; i < rest.size(); i++)
    term = new Application(term, rest[i]);
  return term;
}

PTerm DPOrderingProblem :: make_functional(PConstant f) {
  int arity = arities[f->query_name()];
  PTerm term = f;
  for (int i = 0; i < arity; i++) {
    PType inp = term->query_type()->query_child(0)->copy();
    PVariable X = new Variable(inp);
    term = new Application(term, new MetaApplication(X));
  }
  return term;
}

PTerm DPOrderingProblem :: substitute_variables(PTerm term,
                                                Substitution &gamma,
                                                set<int> &binders) {
  if (term->query_variable()) {
    int x = dynamic_cast<PVariable>(term)->query_index();
    if (binders.find(x) != binders.end()) return term;
    if (!gamma.contains(x)) {
      gamma[x] = fresh_constant(term->query_type());
    }   
    delete term;
    return gamma[x]->copy();
  }

  int bind = -1; 
  if (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    bind = abs->query_abstraction_variable()->query_index();
    if (binders.find(bind) == binders.end())
      binders.insert(bind);
    else bind = -1; 
  }

  for (int i = 0; i < term->number_children(); i++) {
    PTerm child = term->get_child(i);
    PTerm newchild = substitute_variables(child, gamma, binders);
    term->replace_child(i, newchild);
  }

  if (bind != -1) binders.erase(bind);

  return term;
}

void DPOrderingProblem :: make_dp_requirements(DPSet &dps) {
  int i;

  // create variables for DPs
  int base = vars.query_size();
  vars.add_vars(dps.size());
  for (i = 0; i < dps.size(); i++)
    vars.set_description(base+i, "StrictDP[" + wout.str(i+1) + "]");

  // create requirements for each DP, making them have base type
  // while we're at it
  for (i = 0; i < dps.size(); i++) {
    PTerm l = dps[i]->query_left()->copy();
    PTerm r = meta_extend(i, dps[i]->query_right());
    while (l->query_type()->query_composed()) {
      PType inpt = l->query_type()->query_child(0);
      PVariable Z = new Variable(inpt->copy());
      l = new Application(l, new MetaApplication(Z));
    }   
    while (r->query_type()->query_composed()) {
      PType inpt = r->query_type()->query_child(0);
      r = new Application(r, fresh_constant(inpt));
    }   
    // replace free variables by fresh constants as well
    Substitution gamma;
    set<int> binders;
    r = substitute_variables(r, gamma, binders);
    reqs.push_back(OrderRequirement::maybe_greater(l, r, base+i, i));
  }

  // require that at least one DP is oriented strictly
  require_atleastone(base, vars.query_size()-1);
}

void DPOrderingProblem :: make_rule_requirements(Ruleset &rules) {
  int base = vars.query_size();
  vars.add_vars(rules.size());
  for (int i = 0; i < rules.size(); i++) {
    PTerm left = rules[i]->query_left_side()->copy();
    PTerm right = rules[i]->query_right_side()->copy();
    reqs.push_back(OrderRequirement::geq(left, right));
  }
}

void DPOrderingProblem :: tag_below_abstraction(PTerm term,
                bool below_abstraction, map<string,string> &tagged) {
  
  // if we're not currently below an abstraction, we don't need to do
  // much - just delegate to the children!
  if (!below_abstraction) {
    bool b = below_abstraction || term->query_abstraction();
    for (int i = 0; i < term->number_children(); i++)
      tag_below_abstraction(term->get_child(i), b, tagged);
  }

  // if we are below an abstraction, we have to handle applications
  // rooted by a function symbols, as those might correspond to
  // functional terms
  else {
    vector<PTerm> parts = term->split();
    int i;
    if (parts[0]->query_constant()) {
      string f = parts[0]->to_string(false);
      int arity = arities[f];
      for (i = 0; i < arity; i++) {
        if (parts[i+1]->free_var().size() != 0) {
          string newname = f + "-";
          while (alphabet.contains(newname))
            newname = newname + "-";
          tagged[f] = newname;
          dynamic_cast<PConstant>(parts[0])->rename(newname);
          break;
        }
      }
    }
    for (i = 0; i < parts[0]->number_children(); i++)
      tag_below_abstraction(parts[0]->get_child(i), true, tagged);
    for (i = 1; i < parts.size(); i++)
      tag_below_abstraction(parts[i], true, tagged);
  }
}

void DPOrderingProblem :: handle_collapsing_case() {
  set_unfiltered_properties(0);
  for (map<string,int>::iterator it = arities.begin();
       it != arities.end(); it++)
    set_filterable(it->first);
}

void DPOrderingProblem :: handle_untagged_case(DPSet &dps) {
  // only marked symbols are filterable
  set<string> ups;
  for (int i = 0; i < dps.size(); i++) {
    ups.insert(dps[i]->query_left()->query_head()->to_string(false));
    PTerm rhead = dps[i]->query_right()->query_head();
    if (rhead->query_constant()) ups.insert(rhead->to_string(false));
  }
  for (set<string>::iterator si = ups.begin(); si != ups.end(); si++)
    set_filterable(*si);
  
  // add requirements f(x1,...,xn) >= f#(x1,...,xn)
  map<string,int>::iterator it;
  for (it = arities.begin(); it != arities.end(); it++) {
    string f = it->first;
    PConstant fc = alphabet.get(f);
    string upf = wout.up_symbol(fc);
    if (arities.find(upf) == arities.end()) continue;
    PTerm l = make_functional(fc);
    PTerm r = l->copy();
    dynamic_cast<PConstant>(r->query_head())->rename(upf);
    reqs.push_back(OrderRequirement::geq(l, r));
  }
}

void DPOrderingProblem :: handle_tagged_case() {
  map<string,int>::iterator it;

  // anything there is now may be filtered
  for (it = arities.begin(); it != arities.end(); it++)
    set_filterable(it->first);

  // mark symbols below an abstraction
  map<string,string> tagged;
  for (int j = 0; j < reqs.size(); j++)
    tag_below_abstraction(reqs[j]->right, false, tagged);

  // add tagged symbols to alphabet and arities; we do not allow
  // filtering of the tagged symbols!
  map<string,string>::iterator ti;
  for (ti = tagged.begin(); ti != tagged.end(); ti++) {
    string f = ti->first;
    string ftagged = ti->second;
    alphabet.add(ftagged, alphabet.query_type(f)->copy());
    arities[ftagged] = arities[f];
  }

  // and add new requirements for the tagged symbols
  for (ti = tagged.begin(); ti != tagged.end(); ti++) {
    string original = ti->first;
    string tagged = ti->second;

    PConstant org = alphabet.get(original);
    PTerm o = make_functional(org);
    PTerm t = o->copy();
    dynamic_cast<PConstant>(t->query_head())->rename(tagged);
    reqs.push_back(OrderRequirement::geq(t, o));
    string upped = wout.up_symbol(org);
    if (arities.find(upped) != arities.end()) {
      t = t->copy();
      PTerm u = o->copy();
      dynamic_cast<PConstant>(u->query_head())->rename(upped);
      reqs.push_back(OrderRequirement::geq(t, u));
    }
  }
}

bool DPOrderingProblem :: meta_extend_stricts() {
  if (metas_extended) return false;
  bool changed = false;
  for (int i = 0; i < reqs.size(); i++) {
    string tmp = reqs[i]->right->to_string();
    reqs[i]->right = meta_alter(i, reqs[i]->right);
    changed |= tmp == reqs[i]->right->to_string();
  }
  metas_extended = true;
  return changed;
}

bool DPOrderingProblem :: meta_deextend_stricts() {
  if (!metas_extended) return false;
  bool changed = false;
  for (int i = 0; i < reqs.size(); i++) {
    string tmp = reqs[i]->right->to_string();
    reqs[i]->right = meta_alter(i, reqs[i]->right);
    changed |= tmp == reqs[i]->right->to_string();
  }
  metas_extended = false;
  return changed;
}

bool DPOrderingProblem :: constructor_match(PTerm from, PTerm to,
                                            set<string> &defs) {
  if (from->query_meta() || to->query_meta()) return true;
  if (from->query_constant()) {
    if (defs.find(from->to_string(false)) != defs.end())
      return true;
    else return from->equals(to);
  }

  if (from->query_variable()) return to->query_variable();

  if (from->query_abstraction()) {
    return to->query_abstraction() &&
           constructor_match(from->get_child(0),
                             to->get_child(0), defs);
  }
  if (from->query_application()) {
    vector<PTerm> fparts = from->split();
    if (!fparts[0]->query_constant()) return true;
    if (defs.find(fparts[0]->to_string(false)) != defs.end()) return true;
    vector<PTerm> tparts = to->split();
    if (fparts.size() != tparts.size()) return false;
    if (!fparts[0]->equals(tparts[0])) return false;
    for (int i = 1; i < fparts.size(); i++)
      if (!constructor_match(fparts[i], tparts[i], defs)) return false;
  }
  return true;
}

void DPOrderingProblem :: add_usable_rules_requirements() {
  vector< pair<Or*,PTerm> > rhss, relevants;
  vector<OrderRequirement*> rulereqs;
  set<string> defs;
  int i;

  // rhss contains pairs (phi, term) where all subterms of term must
  // be usable if phi is false
  for (i = 0; i < reqs.size(); i++) {
    if (reqs[i]->definite_requirement()) {
      pair<Or*,PTerm> p(new Or(), reqs[i]->right);
      rhss.push_back(p);
    }
    else {
      pair<Or*,PTerm> p(new Or(reqs[i]->orient_at_all()->negate()),
                        reqs[i]->right);
      rhss.push_back(p);
      rulereqs.push_back(reqs[i]);
      defs.insert(reqs[i]->left->query_head()->to_string(false));
    }
  }

  // we make relevants contain all subterms of these right-hand sides
  // which might match some left-hand side
  for (i = 0; i < rhss.size(); i++) {
    PTerm term = rhss[i].second;

    // constants may be left-hand sides!
    if (term->query_constant() &&
        defs.find(term->to_string(false)) != defs.end()) {
      relevants.push_back(rhss[i]);
      continue;
    }

    // otherwise, if it's not an application we just look at the
    // children, which are reachable under the same condition as
    // rhss[i] is
    if (!term->query_application()) {
      if (term->number_children() == 0) delete rhss[i].first;
      else rhss.push_back(pair<Or*,PTerm>(rhss[i].first,
                                          term->get_child(0)));
      for (int j = 1; j < term->number_children(); j++) {
        pair<Or*,PTerm> p(dynamic_cast<Or*>(rhss[i].first->copy()),
                          term->get_child(j));
        rhss.push_back(p);
      }
      continue;
    }

    // if it's an application but not headed by a function symbol,
    // we're also only interested in the children
    vector<PTerm> parts = term->split();
    if (!parts[0]->query_constant()) {
      rhss.push_back(pair<Or*,PTerm>(rhss[i].first, parts[0]));
      for (int j = 1; j < parts.size(); j++) {
        rhss.push_back(pair<Or*,PTerm>(
          dynamic_cast<Or*>(rhss[i].first->copy()), parts[j]));
      }
      continue;
    }

    // term of the form f(p_1,...,p_ar) ... p_n
    string f = parts[0]->to_string(false);
    int ar = arities[f];
    for (int j = 1; j < parts.size(); j++) {
      Or *orcp = dynamic_cast<Or*>(rhss[i].first->copy());
      if (j <= ar) orcp->add_child(is_filtered_away(f, j));
      rhss.push_back(pair<Or*,PTerm>(orcp, parts[j]));
    }
    if (defs.find(f) == defs.end())
      delete rhss[i].first;
    else relevants.push_back(rhss[i]);
  }

  // and now we add the requirements to propagate usable rules!
  // (mostly, we can get this straight from relevants)
  for (i = 0; i < relevants.size(); i++) {
    vector<PTerm> rsplit = relevants[i].second->split();
    for (int j = 0; j < rulereqs.size(); j++) {
      vector<PTerm> lsplit = rulereqs[j]->left->split();
      if (rsplit.size() != lsplit.size()) continue;
      if (!rsplit[0]->equals(lsplit[0])) continue;
      bool all_match = true;
      for (int k = 1; k < lsplit.size() && all_match; k++)
        all_match &= constructor_match(rsplit[k], lsplit[k], defs);
      if (!all_match) continue;
      Or *orcp = dynamic_cast<Or*>(relevants[i].first->copy());
      orcp->add_child(rulereqs[j]->orient_at_all());
      constraints.push_back(orcp->simplify());
    }
    delete relevants[i].first;
  }
}

void DPOrderingProblem :: get_all_symbols(PTerm term, set<string> &symbs) {
  if (term->query_constant()) symbs.insert(term->to_string(false));
  for (int i = 0; i < term->number_children(); i++)
    get_all_symbols(term->get_child(i), symbs);
}

void DPOrderingProblem :: execute_argument_function(PTerm l, PTerm r) {
  MatchRule rule(l,r);
  for (int i = 0; i < reqs.size(); i++) {
    reqs[i]->left = rule.normalise(reqs[i]->left);
    reqs[i]->right = rule.normalise(reqs[i]->right);
  }
}

void DPOrderingProblem :: update_metavar_subs(PTerm term, PVariable x,
                          vector< vector< pair<string,int> > > &ret) {

  if (term->query_meta()) {
    if (term->free_var(true).contains(x)) {
      vector< pair<string,int> > empty;
      ret.push_back(empty);
    }
  }

  else if (term->query_abstraction()) {
    update_metavar_subs(term->get_child(0), x, ret);
  }

  else if (term->query_application()) {
    vector<PTerm> parts = term->split();
    int start = 0;
    if (parts[0]->query_constant()) {
      int i, j, k = ret.size();
      string f = parts[0]->to_string(false);
      start = arities[f] + 1;
      for (i = 1; i <= arities[f] && i < parts.size(); i++) {
        update_metavar_subs(parts[i], x, ret);
        for (j = k; j < ret.size(); j++) {
          pair<string,int> p(f, i);
          ret[j].push_back(p);
        }
        k = ret.size();
      }
    }
    for (int i = start; i < parts.size(); i++) {
      update_metavar_subs(parts[i], x, ret);
    }
  }
}

bool DPOrderingProblem :: simple_argument_functions() {
  int i, j, k;
  RulesManipulator manip;

  // we make a set of all left roots, and of all symbols occurring
  // below a root in a left-hand side
  set<string> leftroots;
  set<string> belowroot;
  for (i = 0; i < reqs.size(); i++) {
    // basics
    vector<PTerm> split = reqs[i]->left->split();
    if (!split[0]->query_constant()) return false;   // can't deal with that!
    string f = split[0]->to_string(false);
    // get symbols below the root
    for (j = 1; j < split.size(); j++)
      get_all_symbols(split[j], belowroot);
    // don't do argument functions for stuff that cannot be filtered
    if (unfilterable(f)) continue;
    if (!alphabet.query_type(f)->vars().empty()) continue;
    // okay, store f for now
    leftroots.insert(f);
  }

  // symbols which occur below a root on the left are not suitable
  // for argument functions
  for (set<string>::iterator cit = belowroot.begin();
       cit != belowroot.end(); cit++) {
    leftroots.erase(*cit);
  }
  if (leftroots.empty()) return false;

  // we filter leftroots to only contain defined symbols which occur
  // only in rules of the form f x1 ... xn -> r
  for (i = 0; i < reqs.size(); i++) {
    vector<PTerm> split = reqs[i]->left->split();
    string f = split[0]->to_string(false);
    if (leftroots.find(f) == leftroots.end()) continue;
    bool ok = true;
    for (j = 1; j < split.size() && ok; j++) {
      ok &= (manip.extended_variable(split[j]) != NULL);
    }
    if (ok) {
      Varset leftmeta = reqs[i]->left->free_var(true);
      Varset rightmeta = reqs[i]->right->free_var(true);
      if (!leftmeta.contains(rightmeta)) ok = false;
    }
    if (!ok) leftroots.erase(f);
  }
  if (leftroots.empty()) return false;

  // make a mapping of the likely symbols to the function symbols
  // occurring in their right-hand sides
  map<string, set<string> > constantmapping;
  set<string>::iterator it, it2;
  set<string> remove;
  for (it = leftroots.begin(); it != leftroots.end(); it++) {
    set<string> symbols;
    string symb = *it;
    for (i = 0; i < reqs.size(); i++) {
      if (reqs[i]->left->split()[0]->to_string(false) == symb)
        get_all_symbols(reqs[i]->right, symbols);
    }
    if (symbols.find(symb) == symbols.end())
      constantmapping[*it] = symbols;
    else remove.insert(symb);
  }
  for (it = remove.begin(); it != remove.end(); it++)
    leftroots.erase(*it);

  // orient the symbols, so if f <= g, then g does not occur in a
  // right-hand side of f
  vector<string> orientedsymbols;
  bool go_on = true;
  while (go_on && !leftroots.empty()) {
    go_on = false;
    for (it = leftroots.begin(); it != leftroots.end(); it++) {
      string f = *it;
      bool ok = true;
      for (it2 = leftroots.begin();
           it2 != leftroots.end() && ok; it2++) {
        string g = *it2;
        if (constantmapping[f].find(g) != constantmapping[f].end())
          ok = false;
      }
      if (ok) {
        orientedsymbols.push_back(f);
        leftroots.erase(f);
        go_on = true;
        break;
      }
    }
  }
  if (orientedsymbols.size() == 0) return false;

  wout.start_method("argument function");
  wout.print("We apply " + wout.cite("Kop12", "Thm. 6.75") +
    " and use the following argument functions:\n");
  wout.start_table();

  // for every symbol in the list, choose a suitable argument
  // function!
  for (i = 0; i < orientedsymbols.size(); i++) {
    string f = orientedsymbols[i];
    vector<PTerm> parts;
    // create variables x1 ... xn with n = arity(f), and form f x1 ... xn 
    vector<MetaApplication*> xs;
    PType ftype = alphabet.query_type(f);
    PTerm left = alphabet.get(f);
    for (j = 0; j < arities[f]; j++) {
      PVariable xj = new Variable(ftype->query_child(0)->copy());
      MetaApplication *zj = new MetaApplication(xj);
      xs.push_back(zj);
      left = new Application(left, zj);
      ftype = ftype->query_child(1);
    }
    // store corresponding right-hand sides in parts
    for (j = 0; j < reqs.size(); j++) {
      vector<PTerm> lsplit = reqs[j]->left->split();
      if (lsplit[0]->to_string(false) != f) continue;
      PTerm right = reqs[j]->right->copy();
      vector<PVariable> abstraction;
      Substitution subst;
      for (k = 0; k < lsplit.size()-1; k++) {
        PVariable Z = manip.extended_variable(lsplit[k+1]);
        if (k < arities[f]) subst[Z] = xs[k]->copy();
        else {
          abstraction.push_back(dynamic_cast<PVariable>(Z->copy()));
          subst[Z] = Z->copy();
        }
      }
      right = right->apply_substitution(subst);
      for (k = (int)(abstraction.size())-1; k >= 0; k--) {
        right = new Abstraction(abstraction[k], right);
      }
      parts.push_back(right);
    }
    // shouldn't happen
    if (parts.size() == 0) {
      delete left;
      continue;
    }

    // create the symbol for the argument function
    string symbol = "#argfun-" + f + "#";
    while (alphabet.contains(symbol)) symbol += "'";
    PType type = ftype->copy();
    for (j = 0; j < parts.size(); j++)
      type = new ComposedType(ftype->copy(), type);
    alphabet.add(symbol, type);
    arities[symbol] = parts.size();
    set_filterable(symbol);

    // create the right-hand side! ?
    PTerm right = alphabet.get(symbol);
    for (j = 0; j < parts.size(); j++)
      right = new Application(right, parts[j]);

    // if an argument is filtered away, it had better not occur on
    // the right!
    for (j = 0; j < xs.size(); j++) {
      PVariable xj = xs[j]->get_metavar();
      vector< vector< pair<string,int> > > combis;
      update_metavar_subs(right, xj, combis);
      for (k = 0; k < combis.size(); k++) {
        Or *somethingfiltered = new Or(not_filtered_away(f, j+1));
        for (int z = 0; z < combis[k].size(); z++) {
          string g = combis[k][z].first;
          int id = combis[k][z].second;
          somethingfiltered->add_child(is_filtered_away(g, id));
        }
        constraints.push_back(somethingfiltered->simplify());
      }
    }

    // and show it!
    map<int,string> meta, free, bound;
    vector<string> entry;
    entry.push_back(wout.pi_symbol() + "(");
    entry.push_back(wout.print_term(left, arities, alphabet, meta,
                                    free, bound));
    entry.push_back(")");
    entry.push_back("=");
    entry.push_back(wout.print_term(right, arities, alphabet, meta,
                                    free, bound));
    wout.table_entry(entry);

    // execute the argument function
    execute_argument_function(left, right);
  }

  wout.end_table();
  wout.succeed_method("argument function");
  return true;
}

void DPOrderingProblem :: justify_orientables() {
  if (used_wrt == "") return;
  int i;

  bool skipped_anything = false;
  for (i = 0; i < reqs.size() && !skipped_anything; i++) {
    PFormula form = reqs[i]->orient_at_all()->simplify();
    skipped_anything = form->query_top();
    delete form;
  }
  if (!skipped_anything) return;

  wout.print("We consider " + used_wrt + " with respect to the following "
    "argument filtering:\n");
  wout.start_table();
  vector<string> entry;

  for (map<string,int>::iterator it = arities.begin();
       it != arities.end(); it++) {
    string f = it->first;
    int arity = it->second;
    vector<int> unfiltered;

    if (unfilterable(f)) continue;
    for (i = 1; i <= arity; i++) {
      PFormula filtered = is_filtered_away(f, i)->simplify();
      if (!filtered->query_top()) unfiltered.push_back(i);
      delete filtered;
    }

    if (unfiltered.size() == arity) continue;
    vector<string> entry;
    string txt = f + "(x_1";
    for (i = 2; i <= arity; i++) txt += ",x_" + wout.str(i);
    txt += ")";
    entry.push_back(txt);
    entry.push_back("=");
    txt = f + "(";
    for (i = 0; i < unfiltered.size(); i++) {
      if (i > 1) txt += ",";
      txt += "x_" + wout.str(unfiltered[i]);
    }
    txt += ")";
    entry.push_back(txt);
    wout.table_entry(entry);
  }
  wout.end_table();

  wout.print("This leaves the following ordering requirements:\n");
  print();
}

