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
#include "rulesmanipulator.h"
#include "sat.h"
#include "substitution.h"
#include "typesubstitution.h"
#include "typer.h"
#include "outputmodule.h"
#include <sstream>
#include <iostream>

/* ========== determining standard properties ========== */

bool RulesManipulator :: left_linear(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    Varset encountered;
    if (!query_linear(rules[i]->query_left_side(), encountered))
      return false;
  }
  return true;
}

bool RulesManipulator :: query_linear(PTerm term, Varset &encountered) {
  if (term->query_meta()) {
    PVariable v = dynamic_cast<MetaApplication*>(term)->get_metavar();
    if (encountered.contains(v)) return false;
    encountered.add(v);
    return true;
  }

  for (int i = 0; i < term->number_children(); i++) {
    PTerm sub = term->get_child(i);
    if (sub != NULL) {
      if (!query_linear(sub, encountered)) return false;
    }
  }

  return true;
}

bool RulesManipulator :: algebraic(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++)
    if (!query_algebraic(rules[i]->query_left_side())) return false;
  return true;
}

bool RulesManipulator :: query_algebraic(PTerm l) {
  if (l == NULL) return true;

  if (l->query_abstraction()) {
    // only abstractions without a meta-variable in them, and
    // abstractions of the form /\x1...xn.Z[x1...xn] are allowed
    if (l->free_var(true).size() == 0) return true;
    return extended_variable(l) != NULL;
  }

  for (int i = 0; i < l->number_children(); i++)
    if (!query_algebraic(l->get_child(i))) return false;
  return true;
}

// returns Z if term = /\x1...xn.Z[x1...xn], otherwise NULL
PVariable RulesManipulator :: extended_variable(PTerm term) {
  vector<PVariable> vars;
  while (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    vars.push_back(abs->query_abstraction_variable());
    term = term->get_child(0);
  }
  if (!term->query_meta()) return NULL;
  if (term->number_children() != vars.size()) return NULL;
  for (int i = 0; i < vars.size(); i++) {
    if (!term->get_child(i)->equals(vars[i])) return NULL;
  }
  return dynamic_cast<MetaApplication*>(term)->get_metavar();
}

bool RulesManipulator :: argument_free(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    if (!query_arguments_below(rules[i]->query_left_side(), 0))
      return false;
  }
  return true;
}

bool RulesManipulator :: meta_single(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    if (!query_arguments_below(rules[i]->query_left_side(), 1))
      return false;
  }
  return true;
}

// returns whether all meta-variables have num arguments or less
bool RulesManipulator :: query_arguments_below(PTerm l, int num) {
  if (l->query_meta() && l->number_children() > num) return false;
  for (int i = 0; i < l->number_children(); i++) {
    if (!query_arguments_below(l->get_child(i), num)) return false;
  }
  return true;
}

bool RulesManipulator :: extended(PTerm term, Varset &bound) {
  if (term->query_meta()) {
    if (!term->query_pattern()) return false;
    Varset X;
    for (int i = 0; i < term->number_children(); i++) {
      PTerm sub = term->get_child(i);
      X.add(dynamic_cast<PVariable>(sub));
    }
    if (X.size() != bound.size()) return false;
    if (!X.contains(bound)) return false;
  }

  if (term->query_abstraction()) {
    PVariable x =dynamic_cast<Abstraction*>(term)->query_abstraction_variable();
    bound.add(x);
    bool ret = extended(term->subterm("1"), bound);
    bound.remove(x);
    return ret;
  }

  if (term->query_application()) {
    return extended(term->subterm("1"), bound) &&
           extended(term->subterm("2"), bound);
  }

  return true;
}

bool RulesManipulator :: fully_extended(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    Varset bound;
    if (!extended(rules[i]->query_left_side(), bound)) return false;
  }
  return true;
}

bool RulesManipulator :: monomorphic(PTerm term) {
  if (term->query_constant() || term->query_variable())
    return term->query_type()->vars().size() == 0;

  if (term->query_meta()) {
    PVariable Z = dynamic_cast<MetaApplication*>(term)->get_metavar();
    if (!monomorphic(Z)) return false;
    for (int i = 0; i < term->number_children(); i++) {
      if (!monomorphic(term->get_child(i))) return false;
    }
    return true;
  }

  if (term->query_abstraction()) {
    return term->query_type()->query_child(0)->vars().size() == 0 &&
           monomorphic(term->subterm("1"));
  }

  if (term->query_application()) {
    return monomorphic(term->subterm("1")) &&
           monomorphic(term->subterm("2"));
  }

  // no idea what this could be
  return false;
}

bool RulesManipulator :: monomorphic(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    if (!monomorphic(rules[i]->query_left_side())) return false;
    // by validity, all type variables in the right-hand side of a
    // rule must also occur in the left-hand side
  }
  return true;
}

bool RulesManipulator :: base_outputs(Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    if (!rules[i]->query_left_side()->query_type()->query_data())
      return false;
  }
  return true;
}

bool RulesManipulator :: fully_first_order(Ruleset &rules) {
  vector<PTerm> subs;
  int i;
  for (i = 0; i < rules.size(); i++) {
    subs.push_back(rules[i]->query_left_side());
    subs.push_back(rules[i]->query_right_side());
  }

  for (i = 0; i < subs.size(); i++) {
    PTerm term = subs[i];
    if (term->query_type()->query_composed()) return false;
    if (term->query_meta() && term->number_children() != 0)
      return false;
    if (term->query_application()) {
      vector<PTerm> parts = term->split();
      if (!parts[0]->query_constant()) return false;
      for (int j = 1; j < parts.size(); j++)
        subs.push_back(parts[j]);
    }
  }

  return true;
}

bool RulesManipulator :: has_critical_pairs(Ruleset &rules) {
  bool mono = monomorphic(rules);

  for (int i = 0; i < rules.size(); i++) {
    // find all subterms of the left-hand side that we'll try to
    // match against
    PTerm l = rules[i]->query_left_side();
    vector<string> positions = l->query_positions();
    vector<PTerm> subs;
    int j;
    for (j = 0; j < positions.size(); j++) {
      PTerm sub = l->subterm(positions[j]);
      if (!sub->query_head()->query_constant()) continue;
      subs.push_back(l->subterm(positions[j]));
    }
    // try matching with all other rules!
    for (j = 0; j < rules.size(); j++) {
      for (int k = 0; k < subs.size(); k++) {
        if (i == j && subs[k] == l) continue;
          // ignore the trivial critical pair
        PTerm l1 = subs[k];
        PTerm l2 = rules[j]->query_left_side();
        if (matchable(l1,l2,mono,true)) return true;
      }
    }
  }

  return false;
}

bool RulesManipulator :: matchable(PTerm s, PTerm t, bool mono,
                                   bool append) {

  // can only happen in a topmost situation
  if (append) {
    vector<PTerm> ss = s->split(), ts = t->split();
    if (ss.size() < ts.size()) return false;
    string pos = "";
    for (int k = ts.size(); k < ss.size(); k++) pos += "1";
    s = s->subterm(pos);
    
    // check whether types even match
    if (mono) {
      if (!s->query_type()->equals(t->query_type())) return false;
    }
    else {
      Typer typer;
      PType unification =
        typer.unify(s->query_type(), t->query_type());
      if (unification == NULL) return false;
      delete unification;
    }
  }

  // at this point, we may safely assume that s and t have matching
  // types

  // everything matches a meta-variable (we assume full extendedness)
  if (s->query_meta() || t->query_meta()) return true;

  if (s->query_constant()) {
    if (!t->query_constant()) return false;
    return s->to_string(false) == t->to_string(false);
  }

  if (s->query_variable()) {
    return t->query_variable();
      // TODO - lazyness, should check whether it's the same variable
      // but this would require more work with renaming of binders
  }

  if (s->query_abstraction()) {
    if (!t->query_abstraction()) return false;
    return matchable(s->subterm("1"), t->subterm("1"), mono, false);
      // lazyness
  }

  if (!s->query_application()) return true; // errrrr

  vector<PTerm> ss = s->split(), ts = t->split();
  if (ss.size() != ts.size()) return false;
  // in an application, types match if types of the head symbols match
  if (mono) {
    if (!ss[0]->query_type()->equals(ts[0]->query_type())) return false;
  }
  else {
    Typer typer;
    PType unification =
      typer.unify(ss[0]->query_type(), ts[0]->query_type());
    if (unification == NULL) return false;
    delete unification;
  }
  // check whether all subterms match!
  for (int i = 0; i < ts.size(); i++)
    if (!matchable(ss[i], ts[i], mono, false)) return false;
  return true;
}

bool RulesManipulator :: eta_long(Ruleset &rules, bool only_functional) {
  vector<PTerm> subs;
  int i;

  for (i = 0; i < rules.size(); i++) {
    subs.push_back(rules[i]->query_left_side());
    subs.push_back(rules[i]->query_right_side());
  }

  for (i = 0; i < subs.size(); i++) {
    PTerm s = subs[i];
    while (s->query_abstraction()) s = s->subterm("1");
    if (s->query_meta()) {
      for (int j = 0; j < s->number_children(); j++) {
        if (!s->get_child(j)->query_variable())
          subs.push_back(s->get_child(j));
      }
      continue;
    }
    if (s->query_type()->query_composed()) {
      if (only_functional && s->query_head()->query_constant())
        return false;
      if (!only_functional) return false;
    }
    while (s->query_application()) {
      subs.push_back(s->subterm("2"));
      s = s->subterm("1");
    }
    if (s->query_abstraction()) subs.push_back(s);
  }

  return true;
}

ArList RulesManipulator :: get_arities(Alphabet &F, Ruleset &rules) {
  ArList arities;
  int i;
  vector<PTerm> subs;

  for (i = 0; i < rules.size(); i++) {
    rules[i]->query_left_side()->adjust_arities(arities);
    rules[i]->query_right_side()->adjust_arities(arities);
  }

  return arities;
}

/* ========== testing plain function passing ========== */

// returns the variable for "sort x >= sort y"
int RulesManipulator :: sort_comparison_variable(string x, string y,
                                          map<string,int> &varmap,
                                          set<string> &allsorts) {
  string name = x + " | " + y;
  if (varmap.find(name) == varmap.end()) {
    varmap[name] = vars.query_size();
    vars.add_vars(1);
    vars.set_description(varmap[name], "||" + x + ">=" + y + "||");
    if (allsorts.find(x) == allsorts.end()) allsorts.insert(x);
    if (allsorts.find(y) == allsorts.end()) allsorts.insert(y);
  }
  return varmap[name];
}

// returns whether Pos^x(sort, type) = Pos(sort, type)
// here, x = + if positive is true, - otherwise
PFormula RulesManipulator :: positions_okay(string sort, PType type,
                           bool positive, map<string,int> &varmap,
                           set<string> &allsorts) {

  if (type->query_data() && type->query_child(0) == NULL) {
    // Pos^+(kappa, iota) = Pos(kappa, iota) if kappa >= iota
    // Pos^-(kappa, iota) = Pos(kappa, iota) if kappa > iota
    string other = type->to_string();
    if (positive) {
      return new Var(sort_comparison_variable(sort, other, varmap, allsorts));
    }
    else {
      return new And(
        new Var(sort_comparison_variable(sort, other, varmap, allsorts)),
        new AntiVar(sort_comparison_variable(other, sort, varmap, allsorts))
      );
    }
  }

  if (type->query_composed()) {
    PType input = type->query_child(0);
    PType output = type->query_child(1);
    return new And(positions_okay(sort, input, !positive, varmap, allsorts),
                   positions_okay(sort, output, positive, varmap, allsorts));
  }

  return new Bottom;
}

// returns whether pos in Acc(f)
PFormula RulesManipulator :: position_accessible(PConstant f,
                  int pos, Alphabet &Sigma, ArList &arities,
                  map<string,int> &varmap, set<string> &allsorts) {
  
  int i;
  PType postype, type = f->query_type();
  for (i = 0; i < pos && type->query_composed(); i++)
    type = type->query_child(1);
  if (!type->query_composed()) return new Bottom;
  postype = type->query_child(0);
  for (i = 0; type->query_composed(); i++)
    type = type->query_child(1);
  if (i > arities[f->query_name()]) return new Bottom;
  if (!type->query_data() || type->query_child(0) != NULL)
    return new Bottom;
  return positions_okay(type->to_string(), postype, true, varmap, allsorts);
}

// returns whether Z is accessible in term
PFormula RulesManipulator :: accessible(int Z, PTerm term,
                                        ArList &arities,
                                        map<string,int> &varmap,
                                        set<string> &allsorts) {
  // if term = /\x1...xn.Z[x1...xn], then we are done!
  PVariable testmetaabs = extended_variable(term);
  if (testmetaabs != NULL) {
    if (testmetaabs->query_index() == Z) return new Top;
    else return new Bottom;
  }

  // otherwise, if term is an abstraction, we just check its subterm
  if (term->query_abstraction()) {
    return accessible(Z, term->get_child(0), arities, varmap, allsorts);
  }

  // otherwise, if term is not a functional term, we're bound to fail
  vector<PTerm> split = term->split();
  if (!split[0]->query_constant()) return new Bottom;

  // if it is a functional term, check its strict subterms
  if (split.size()-1 != arities[split[0]->to_string()]) return new Bottom;
  Or *ret = new Or();
  string outputtype = term->query_type()->to_string();
  for (int i = 1; i < split.size(); i++) {
    PFormula ok = positions_okay(outputtype, split[i]->query_type(),
                                 true, varmap, allsorts);
    ret->add_child(new And(ok,
                   accessible(Z, split[i], arities, varmap, allsorts)));
  }

  return ret;
}

bool RulesManipulator :: check_inequalities(And *formula,
                                            map<string,int> &varmap,
                                            set<string> &allsorts,
                                            map<string,int> &sortord) {
  set<string>::iterator it1, it2, it3;

  // add the requirements that >= is transitive, reflexive and total
  for (it1 = allsorts.begin(); it1 != allsorts.end(); it1++) {
    string x = *it1;
    // x >= x
    formula->add_child(new Var(
      sort_comparison_variable(x, x, varmap, allsorts)));
    for (it2 = allsorts.begin(); it2 != allsorts.end(); it2++) {
      string y = *it2;
      if (x == y) continue;
      // x >= y \/ y >= x
      formula->add_child(new Or(
        new Var(sort_comparison_variable(x, y, varmap, allsorts)),
        new Var(sort_comparison_variable(y, x, varmap, allsorts))
      ));
      for (it3 = allsorts.begin(); it3 != allsorts.end(); it3++) {
        string z = *it3;
        if (x == z || y == z) continue;
        // x >= y /\ y >= z -> x >= z
        formula->add_child(new Or(
          new AntiVar(sort_comparison_variable(x, y, varmap, allsorts)),
          new AntiVar(sort_comparison_variable(y, z, varmap, allsorts)),
          new Var(sort_comparison_variable(x, z, varmap, allsorts))
        ));
      }
    }
  }

  // simplify the formula and send it to a sat solver!
  PFormula f = formula->simplify();
  SatSolver solver;
  bool ret = solver.solve(f);
  delete f;

  // if it's not solvable, we can return straight away
  if (!ret) return false;

  // but if it is solvable, let's find out the ordering!
  for (int number = 0; !allsorts.empty(); number++) {
    set<string> remove;
    // find all minimal elements of allsorts, put them in remove
    for (it1 = allsorts.begin(); it1 != allsorts.end(); it1++) {
      string x = *it1;
      bool minimal = true;
      for (it2 = allsorts.begin(); it2 != allsorts.end() && minimal; it2++) {
        string y = *it2;
        Valuation ygeqx =
          vars.query_value(sort_comparison_variable(y, x, varmap, allsorts));
        if (ygeqx == FALSE) minimal = false;
      }
      if (minimal) remove.insert(x);
    }
    
    // assign all minimal sorts weight number, and remove them from
    // allsorts
    for (it3 = remove.begin(); it3 != remove.end(); it3++) {
      string x = *it3;
      allsorts.erase(x);
      sortord[x] = number;
    }
  }

  return true;
}

bool RulesManipulator :: plain_function_passing_new(Ruleset &rules,
                                          ArList &arities,
                                          map<string,int> &sortord) {
  And *formula = new And();
  map<string,int> varmap;
  set<string> allsorts;
  vars.reset();

  for (int i = 0; i < rules.size(); i++) {
    PTerm left = rules[i]->query_left_side();
    PTerm right = rules[i]->query_right_side();
    Varset rightvars = right->free_var(true);
    vector<PTerm> split = left->split();
    if (!split[0]->query_constant()) continue;
    PConstant f = dynamic_cast<PConstant>(split[0]);
    int ar = arities[f->query_name()];
    if (ar == 0) continue;
    if (ar > split.size()-1) ar = split.size()-1;
    for (Varset::iterator it = rightvars.begin();
         it != rightvars.end(); it++) {
      int Z = *it;
      Or *Zacc = new Or();
      for (int j = 1; j <= ar; j++)
        Zacc->add_child(accessible(Z, split[j], arities, varmap, allsorts));
      formula->add_child(Zacc);
    }
  }

  return check_inequalities(formula, varmap, allsorts, sortord);
}

bool RulesManipulator :: plain_function_passing_old(Ruleset &rules,
                                                    ArList &arities,
                                         map<string,int> &sortord) {
  vector<PTerm> subs;
  int i;

  // for each of the immediate arguments of the left-hand side:
  // either it IS a single variable (or a lengthened form thereof)
  // or it contains no such thing

  for (i = 0; i < rules.size(); i++) {
    vector<PTerm> leftsplit = rules[i]->query_left_side()->split();
    for (int j = 0; j < leftsplit.size(); j++) {
      PTerm s = leftsplit[j];
      if (s->query_meta()) continue;
      Varset bound;
      while (s->query_abstraction()) {
        bound.add(dynamic_cast<Abstraction*>(s)->query_abstraction_variable());
        s = s->subterm("1");
      }
      if (s->query_meta()) {
        // not dangerous: /\x1...xn.Z with Z first order
        if (!s->query_type()->query_composed() && s->subterm("0") == NULL)
          continue;
        // not dangerous: /\x1...xn.Z[x1,...,xn], regardless of the type of Z
        if (extended(s, bound)) continue;
        // everything else is!
        return false;
      }
      
      // if the argument is not a single meta-variable, then it
      // shouldn't contain any higher-order meta-variables
      subs.push_back(s);
    }
  }

  for (i = 0; i < subs.size(); i++) {
    PTerm s = subs[i];
    while (s->query_abstraction()) s = s->subterm("1");
    if (s->query_meta()) {
      if (s->query_type()->query_composed()) return false;
      if (s->subterm("0") != NULL) return false;
    }
    if (s->query_application()) {
      subs.push_back(s->subterm("1"));
      subs.push_back(s->subterm("2"));
    }
  }

  // we didn't encounter anything that's wrong!
  return true;
}

bool RulesManipulator :: plain_function_passing(Ruleset &rules,
                   ArList &arities, map<string,int> &sortord) {
  return //plain_function_passing_old(rules, arities, sortord) ||
         plain_function_passing_new(rules, arities, sortord);
}

bool RulesManipulator :: strong_plain_function_passing(Ruleset &rules,
                          ArList &arities, map<string,int> &sortord) {
  if (!plain_function_passing(rules, arities, sortord)) return false;

  for (int i = 0; i < rules.size(); i++) {
    string f = rules[i]->query_left_side()->query_head()->to_string(false);

    // check whether the right-hand side has any subterms of the form
    // /\x.C[f ... D[x] ...]
    vector<PTerm> subs;
    subs.push_back(rules[i]->query_right_side());
    while (subs.size() != 0) {
      PTerm s = subs[subs.size()-1];
      subs.pop_back();
      while (s->query_abstraction()) s = s->get_child(0);
      for (int j = 0; j < s->number_children(); j++)
        subs.push_back(s->get_child(j));

      // given a non-abstraction subterm, check whether it has the
      // form f ... D[x] ... with x a variable (note that rules, in
      // general, do not contain free variables, so this variable is
      // bound in r
      PTerm shead = s->query_head();
      if (!shead->query_constant()) continue;
      Varset FV = s->free_var();
      if (FV.size() == 0) continue;
      set<string> found;
      reachable_from(shead->to_string(false), found, rules);

      // this is only dangerous if the left-hand side is reachable
      // from this point
      if (found.find(f) == found.end()) continue;
      return false;
    }
  }

  return true;
}

/* ========== completing a system ========== */

Ruleset RulesManipulator :: beta_saturate(Ruleset rules) {
  for (int i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();

    if (!r->query_abstraction()) continue;

    // if there is a headmost beta-redex, or
    // headmost-below-a-variable, add a copy where this is reduced
    vector<PVariable> xs;
    PTerm p = r;
    while (p->query_abstraction()) {
      Abstraction *abs = dynamic_cast<Abstraction*>(p);
      PVariable x = abs->query_abstraction_variable();
      xs.push_back(x);
      p = abs->get_child(0);
    }
    PTerm head = p->query_application() ? p->get_child(0) : NULL;
    string pos = "";
    while (head != NULL && head->query_application()) {
      head = head->get_child(0);
      pos += "1";
    }
    if (head != NULL && head->query_abstraction()) {
      Beta beta;
      PTerm preduced = beta.apply(p->copy(), pos);
      for (int k = xs.size()-1; k >= 0; k--) {
        preduced = new Abstraction(xs[k], preduced);
      }
      rules.push_back(new MatchRule(l->copy(),preduced));
      continue;
    }

    // if we're still here, then there is no headmost beta-redex
    if (!r->query_abstraction()) continue;

    // for a rule l -> /\x.r, add l x -> r
    Abstraction *abs = dynamic_cast<Abstraction*>(r);
    PVariable x = abs->query_abstraction_variable();
    PVariable Z = new Variable(x->query_type()->copy());
    MetaApplication *MZ = new MetaApplication(Z);
    Substitution subst;
    subst[x] = MZ->copy();
    l = new Application(l->copy(), MZ);
    r = r->subterm("1")->copy()->apply_substitution(subst);
    rules.push_back(new MatchRule(l,r));
  }

  return rules;
}

/* ========== eta-expanding a system ========== */

PTerm RulesManipulator :: eta_expand(PTerm term, bool skiptop) {
  if (!skiptop && !term->query_meta() && !term->query_abstraction()) {
    PTerm t = eta_expand(term, true);
    vector<PVariable> vars;
    while (t->query_type()->query_composed()) {
      PVariable x = new Variable(t->query_type()->query_child(0));
      vars.push_back(x);
      t = new Application(t, eta_expand(x, false));
    }
    for (int i = vars.size()-1; i >= 0; i--)
      t = new Abstraction(vars[i], t);
    return t;
  }

  // now we can ignore the top
  if (term->query_variable() || term->query_constant())
    return term->copy();

  if (term->query_application()) {
    PTerm l = eta_expand(term->get_child(0), true);
    PTerm r = eta_expand(term->get_child(1), false);
    return new Application(l, r);
  }

  if (term->query_meta()) {
    vector<PTerm> subs;
    for (int i = 0; i < term->number_children(); i++) {
      PTerm t = term->get_child(i);
      if (t->query_variable()) subs.push_back(t->copy());
      else subs.push_back(eta_expand(t, false));
    }
    PVariable Z = dynamic_cast<MetaApplication*>(term)->get_metavar();
    return new MetaApplication(Z, subs);
  }

  if (term->query_abstraction()) {
    PTerm sub = eta_expand(term->get_child(0), false);
    PVariable x =
      dynamic_cast<Abstraction*>(term)->query_abstraction_variable();
    return new Abstraction(x,sub);
  }

  // ERROR
  return term->copy();
}

Ruleset RulesManipulator :: eta_expand(Ruleset rules) {
  for (int i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();
    while (l->query_type()->query_composed()) {
      PVariable Z = new Variable(l->query_type()->query_child(0)->copy());
      PVariable ZZ = dynamic_cast<PVariable>(Z->copy());
      l = new Application(l, new MetaApplication(Z));
      r = new Application(r, new MetaApplication(ZZ));
    }
    l = eta_expand(l);
    r = eta_expand(r);
    delete rules[i];
    rules[i] = new MatchRule(l,r);
  }
  return rules;
}

/* ========== formative rules ========== */

void RulesManipulator :: Symb(PTerm s, vector<string> &f,
                              vector<PType> &type) {
  if (s->query_meta()) return;

  if (s->query_abstraction()) {
    f.push_back("#ABS");
    type.push_back(s->query_type());
    Symb(s->subterm("1"), f, type);
    return;
  }
  
  PTerm head = s->query_head();
  if (head->query_constant()) f.push_back(head->to_string(false));
  else f.push_back("#VAR");
  type.push_back(s->query_type());

  while (s != head) {
    Symb(s->subterm("2"), f, type);
    s = s->subterm("1");
  }
}

bool RulesManipulator :: add_symbol(SymbolList &list, string symbol,
                                    PType type) {
  if (list.find(symbol) == list.end()) {
    list[symbol].push_back(type->copy());
    return true;
  }

  for (int i = 0; i < (int)list[symbol].size(); i++) {
    PType othertype = list[symbol][i];
    TypeSubstitution theta1, theta2;
    if (othertype->instantiate(type, theta1)) return false;
    if (type->instantiate(othertype, theta2)) {
      list[symbol][i] = list[symbol][list[symbol].size()-1];
      list[symbol].pop_back();
      i--;
      delete othertype;
    }
  }

  // it's not already in the list - add it
  list[symbol].push_back(type->copy());

  // make sure the list doesn't get too large!
  if (list[symbol].size() >= 20) {
    for (int j = 0; j < list[symbol].size(); j++)
      delete list[symbol][j];
    list[symbol].resize(1);
    list[symbol][0] = new TypeVariable();
  }

  // we changed at least one thing
  return true;
}

bool RulesManipulator :: add_symbols(SymbolList &list, vector<string>
                                    &symbols, vector<PType> &types) {
  
  bool ret = false;
  for (int i = 0; i < symbols.size(); i++) {
    ret |= add_symbol(list, symbols[i], types[i]);
  }
  return ret;
}

bool RulesManipulator :: add_sub_symbols(SymbolList &list, PTerm l) {
  bool ret = false;
  while (l->query_application()) {
    vector<string> symbols;
    vector<PType> types;
    Symb(l->subterm("2"), symbols, types);
    ret |= add_symbols(list, symbols, types);
    l = l->subterm("1");
  }
  return ret;
}

bool RulesManipulator :: symbol_occurs(SymbolList &list, string f,
                                       PType type) {
  // if any symbol is okay, check for all symbols we know whether
  // there's a matching type
  if (f == "#ANY") {
    SymbolList::iterator it;
    for (it = list.begin(); it != list.end(); it++) {
      if (symbol_occurs(list, it->first, type)) return true;
    }
    return false;
  }

  if (list.find(f) == list.end()) return false;

  for (int i = 0; i < (int)list[f].size(); i++) {
    PType othertype = list[f][i];
    TypeSubstitution theta;
    if (othertype->instantiate(type, theta)) return true;
  }
  return false;
}

SymbolList RulesManipulator :: formative_symbols(DPSet &DP,
                                                 Ruleset &rules) {
  SymbolList list;

  // initialisation: put in all left-hand sides of pairs in DP
  for (int d = 0; d < DP.size(); d++) {
    add_sub_symbols(list, DP[d]->query_left());
  }

  bool changed = true;
  while (changed) {
    // this must end eventually, because at most 20 of each symbol
    // can be added, and there are only finitely many symbols!
    changed = false;

    for (int i = 0; i < rules.size(); i++) {
      PTerm l = rules[i]->query_left_side();
      PTerm r = rules[i]->query_right_side();

      if (r->query_abstraction() &&
          symbol_occurs(list, "#ABS", r->query_type())) {
        vector<string> symbols;
        vector<PType> types;
        Symb(l, symbols, types);
        changed |= add_symbols(list, symbols, types);
        continue;
      }

      PTerm head = r->query_head();
      PType outp = r->query_type();
      while (true) {
        // check whether this rule (with perhaps some meta-variables
        // added) should be added to the formative rules
        bool ok;
        if (head->query_constant())
          ok = symbol_occurs(list, head->to_string(false), outp);
        else
          ok = symbol_occurs(list, "#ANY", outp);

        // okay, this should be added!
        if (ok) {
          changed |= add_symbol(list, l->query_head()->to_string(false), outp);
          changed |= add_sub_symbols(list, l);
        }

        if (!outp->query_composed()) break;
        // pretend we've appended a meta-variable to both sides
        outp = outp->query_child(1);
      }
    }
  }

  return list;
}

Ruleset RulesManipulator :: formative_rules_for(SymbolList &list,
                                                Ruleset &rules) {
  Ruleset ret;

  for (int i = 0; i < rules.size(); i++) {
    PTerm r = rules[i]->query_right_side();

    if (r->query_abstraction()) {
      if (symbol_occurs(list, "#ABS", r->query_type()))
        ret.push_back(new MatchRule(rules[i]->query_left_side()->copy(),
                                    rules[i]->query_right_side()->copy()));
      continue;
    }

    PTerm head = r->query_head();
    PType outp = r->query_type();

    while (true) {
      // check whether this rule (with perhaps some meta-variables
      // added) should be added to the formative rules
      bool ok;
      if (head->query_constant())
        ok = symbol_occurs(list, head->to_string(false), outp);
      else
        ok = symbol_occurs(list, "#ANY", outp);

      // okay, this should be added!
      if (ok) {
        PTerm ll = rules[i]->query_left_side()->copy();
        PTerm rr = rules[i]->query_right_side()->copy();
        while (!rr->query_type()->equals(outp)) {
          PType inp = rr->query_type()->query_child(0)->copy();
          PVariable Z = new Variable(inp);
          MetaApplication *MZ = new MetaApplication(Z);
          ll = new Application(ll, MZ);
          rr = new Application(rr, MZ->copy());
        }
        MatchRule *newrule = new MatchRule(ll, rr);
        ret.push_back(newrule);
        break;
      }

      if (!outp->query_composed()) break;
      outp = outp->query_child(1);
    }
  }

  return ret;
}

void RulesManipulator :: free_list(SymbolList &list) {
  for (SymbolList::iterator it = list.begin(); it != list.end(); it++) {
    for (int i = 0; i < it->second.size(); i++) {
      delete it->second[i];
    }
  }
}

Ruleset RulesManipulator :: copy_rules(Ruleset &rules) {
  Ruleset ret;
  for (int i = 0; i < rules.size(); i++) {
    ret.push_back(new MatchRule(
        rules[i]->query_left_side()->copy(),
        rules[i]->query_right_side()->copy()));
  }
  return ret;
}

Ruleset RulesManipulator :: formative_rules(DPSet DP, Ruleset rules) {
  SymbolList list = formative_symbols(DP, rules);
  /* // for debugging
  cout << "=====" << endl << "Got symbols:" << endl;
  for (SymbolList::iterator it = list.begin(); it != list.end(); it++) {
    cout << it->first << " : ";
    for (int i = 0; i < it->second.size(); i++) {
      if (i != 0) cout << " , ";
      cout << it->second[i]->to_string();
    }
    cout << endl;
  }
  */
  Ruleset ret = formative_rules_for(list, rules);
  free_list(list);

  // is everything properly linear and fully extended?
  bool all_ok = true;
  for (int i = 0; i < DP.size() && all_ok; i++) {
    Varset X, Y;
    if (!query_linear(DP[i]->query_left(), X)) all_ok = false;
    else if (!extended(DP[i]->query_left(), Y)) all_ok = false;
  }

  if (all_ok) return ret;
  
  for (int j = 0; j < ret.size(); j++) delete ret[j];
  return copy_rules(rules);
}

/* ========== usable rules ========== */

void RulesManipulator :: reachable_from(string symb, set<string>
                                        &found, Ruleset &rules) {
  found.insert(symb);

  for (int i = 0; i < rules.size(); i++) {
    if (rules[i]->query_left_side()->query_head()->to_string() != symb)
      continue;
    vector<PTerm> subs;
    subs.push_back(rules[i]->query_right_side());
    for (int j = 0; j < subs.size(); j++) {
      PTerm s = subs[j];
      while (s->query_abstraction()) s = s->get_child(0);
      for (int k = 0; k < s->number_children(); k++)
        subs.push_back(s->get_child(k));
      if (!s->query_constant()) continue;
      string f = s->to_string(false);
      if (found.find(f) != found.end()) continue;
      reachable_from(f, found, rules);
    }
  }
}

/*
SymbolList RulesManipulator :: usable_symbols(DPSet &DP,
                                              Ruleset &rules) {
  SymbolList list;

  // initialisation: put in all right-hand sides of pairs in DP
  for (int d = 0; d < DP.size(); d++) {
    add_sub_symbols(list, DP[d]->query_right());
  }

  bool changed = true;
  while (changed) {
    // this must end eventually, because at most 20 of each symbol
    // can be added, and there are only finitely many symbols!
    changed = false;

    for (int i = 0; i < rules.size(); i++) {
      PTerm l = rules[i]->query_left_side();
      PTerm r = rules[i]->query_right_side();

      PTerm head = l->query_head();
      PType outp = l->query_type();
      while (true) {
        // check whether this rule (with perhaps some meta-variables
        // added) should be added to the usable rules
        bool ok = symbol_occurs(list, head->to_string(false), outp);

        // okay, this should be added!
        if (ok) {
          changed |= add_symbol(list, l->query_head()->to_string(false), outp);
          changed |= add_sub_symbols(list, l);
        }

        if (!outp->query_composed()) break;
        // pretend we've appended a meta-variable to both sides
        outp = outp->query_child(1);
      }
    }
  }

  return list;
}
*/

Ruleset RulesManipulator :: usable_rules(DPSet DP, Ruleset rules) {
  int i, j;

  // find usable symbols
  set<string> symbols;
  for (i = 0; i < DP.size(); i++) {
    vector<PTerm> split = DP[i]->query_right()->split();
    if (!split[0]->query_constant()) return rules;
      // collapsing cycle => all rules are usable
    for (j = 1; j < split.size(); j++)
      if (!split[j]->query_pattern()) return rules;
      // risky dependency pair => all rules are usable

    vector<PTerm> subs;
    for (j = 1; j < split.size(); j++) subs.push_back(split[j]);
    for (j = 0; j < subs.size(); j++) {
      PTerm s = subs[j];
      while (s->query_abstraction()) s = s->get_child(0);
      for (int k = 0; k < s->number_children(); k++)
        subs.push_back(s->get_child(k));

      // for all symbols in the right-hand side, save the symbol and
      // everything it reduces to as "usable"
      if (s->query_constant())
        reachable_from(s->to_string(false), symbols, rules);
    }
  }

  // save the resulting usable rules, and balk if one of them is risky
  Ruleset ret;
  for (i = 0; i < rules.size(); i++) {
    string lhead = rules[i]->query_left_side()->query_head()->to_string(false);
    if (symbols.find(lhead) == symbols.end()) continue;
      // only consider usable rules
    if (rules[i]->query_right_side()->query_abstraction())
      return rules;
    if (rules[i]->query_right_side()->query_meta() &&
        !rules[i]->query_right_side()->query_type()->query_data())
      return rules;
      // right-hand of a usable rule may not be an abstraction or
      // functional meta-variable
    if (!rules[i]->query_right_side()->query_pattern())
      return rules;
      // right-hand sides of usable rules should not be risky
    ret.push_back(rules[i]);
  }
  return ret;
}

/* ========== simplifying applications ========== */

set<string> RulesManipulator :: identify_possible_applications(Ruleset &rules) {
  // find the possible encoded applications
  set<string> ret;
  for (int i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();
    // see whether this is <A> X1 ... Xn -> <B> X1 ... Xn
    // where <B> is not an application
    bool ok = true;
    while (r->query_application()) {
      if (!l->query_application()) { ok = false; break; }
      PTerm lsub = l->get_child(1);
      PTerm rsub = r->get_child(1);
      if (lsub == NULL || rsub == NULL || !lsub->equals(rsub) ||
          !lsub->query_meta() || lsub->number_children() != 0) {
        ok = false;
        break;
      }
      l = l->get_child(0);
      r = r->get_child(0);
    }
    // see whether <A> = f B, for some symbol f
    if (ok) {
      if (!l->query_application()) ok = false;
      else if (!l->get_child(1)->equals(r)) ok = false;
      else ok = l->get_child(0)->query_constant();
    }
    // is B a metavariable (or perhaps an extended one?)
    if (ok) ok = (extended_variable(r) != NULL);
    // then it seems that f is a potential encoded application!
    if (ok) {
      PConstant f = dynamic_cast<PConstant>(l->get_child(0));
      ret.insert(f->query_name());
    }
  }

  // if any of them is also used as a root elsewhere, it is not suitable
  set<string>::iterator it = ret.begin();
  while (it != ret.end()) {
    string f = *it;
    int count = 0;
    for (int i = 0; i < rules.size(); i++) {
      PTerm l = rules[i]->query_left_side();
      PTerm lhead = l->query_head();
      if (lhead->query_constant()) {
        PConstant g = dynamic_cast<PConstant>(lhead);
        if (g->query_name() == f) count++;
      }
      else count++;
    }
    it++;
    if (count != 1) ret.erase(f);
  }

  return ret;
}

vector<PTerm> RulesManipulator :: find_symbol_subterms(string f, PTerm s) {
  vector<PTerm> ret;

  vector<PTerm> queue;
  queue.push_back(s);
  for (int i = 0; i < queue.size(); i++) {
    PTerm t = queue[i];
    
    while (t->query_application()) {
      queue.push_back(t->get_child(1));
      t = t->get_child(0);
    }
    if (t->to_string(false,false) == f) ret.push_back(queue[i]);

    for (int j = 0; j < t->number_children(); j++) {
      queue.push_back(t->get_child(j));
    }
  }

  return ret;
}

PVariable RulesManipulator :: query_fake_application(string ap, PTerm t,
                                                     vector<PTerm> &args) {
  vector<PTerm> tmp;
  if (!t->query_application()) return NULL;

  while (t->get_child(0)->query_application()) {
    tmp.push_back(t->get_child(1));
    t = t->get_child(0);
  }

  if (t->get_child(0)->to_string(false,false) != ap) return NULL;
  if (!t->get_child(1)->query_meta()) return NULL;
  if (t->get_child(1)->number_children() != 0) return NULL;
  PVariable Z = dynamic_cast<MetaApplication*>(t->get_child(1))->get_metavar();
  for (int i = tmp.size()-1; i >= 0; i--) args.push_back(tmp[i]);
  return Z;
}

bool RulesManipulator :: find_encoded_metavars(string ap, PTerm l,
                                               map<int,int> &arities) {
  vector<PTerm> apsubs = find_symbol_subterms(ap, l);
  for (int i = 0; i < apsubs.size(); i++) {
    vector<PTerm> args;
    PVariable Z = query_fake_application(ap, apsubs[i], args);
    if (Z == NULL) return false;
    int id = Z->query_index();
    if (arities.find(id) == arities.end()) arities[id] = args.size();
    if (arities[id] != args.size()) return false;
    for (int j = 0; j < args.size(); j++) {
      if (!args[j]->query_variable()) return false;
      for (int k = j+1; k < args.size(); k++) {
        if (args[j]->equals(args[k])) return false;
      }
    }
  }

  return true;
}

bool RulesManipulator :: check_encoded_metavar_use(string ap, PTerm term,
                                                   map<int,int> &arities) {
  // meta-variables in arities cannot occur on their own
  if (term->query_meta()) {
    int id = dynamic_cast<MetaApplication*>(term)->get_metavar()->query_index();
    if (arities.find(id) != arities.end()) return false;
      // id occurs: this metavar should never occur on its own, only wrapped
      // in ap!
  }
  
  // the "application" constant also cannot occur on its own
  if (term->query_constant()) {
    return term->to_string(false,false) != ap;
  }

  // otherwise, for abstractions and meta-applications, and also for
  // applications whose head is not ap, just check their children
  if (!term->query_application() ||
      term->query_head()->to_string(false,false) != ap) {
    for (int i = 0; i < term->number_children(); i++) {
      if (!check_encoded_metavar_use(ap, term->get_child(i), arities)) {
        return false;
      }
    }
    return true;
  }

  // so we have something of the form ap s0 ... sn; start by checking
  // s1...sn
  int n = 0;
  while (term->query_application() &&
         term->get_child(0)->query_application()) {
    n++;
    if (!check_encoded_metavar_use(ap, term->get_child(1), arities))
      return false;
    term = term->get_child(0);
  }
  // now, if s0 is a meta-variable, then it is allowed to occur even if
  // it has an "arity", provided n >= arities[s0]
  PTerm snd = term->get_child(1);
  if (!snd->query_meta() || snd->number_children() > 0) {
    return check_encoded_metavar_use(ap, snd, arities);
  }
  PVariable Z = dynamic_cast<MetaApplication*>(snd)->get_metavar();
  int id = Z->query_index();
  return arities.find(id) == arities.end() || arities[id] <= n;
}

bool RulesManipulator :: used_as_application(string ap, Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();
    if (l->query_head()->to_string(false, false) == ap) continue;
      // rule may be ignored because it's the rule defining ap
    
    map<int,int> arities;
    if (!find_encoded_metavars(ap, l, arities)) return false;
    if (!check_encoded_metavar_use(ap, r, arities)) return false;
  }
  return true;
}

PTerm RulesManipulator :: replace_encoded_applications(string ap, PTerm s,
                                                       map<int,int> &arities) {
  vector<PTerm> args;

  // if it's a fake application, turn it into a meta-application
  PVariable Z = query_fake_application(ap, s, args);
  if (Z != NULL) {
    int id = Z->query_index();
    if (arities.find(id) == arities.end()) arities[id] = args.size();
    if (arities[id] == args.size()) {
      for (int k = 0; k < args.size(); k++) {
        args[k] = replace_encoded_applications(ap, args[k]->copy(),
                                               arities);
      }
      PVariable Zcopy = dynamic_cast<PVariable>(Z->copy());
      MetaApplication *ret = new MetaApplication(Zcopy, args);
      delete s;
      return ret;
    }
  }

  // other headmost occurrences of ap are simply removed
  if (s->query_application() &&
      s->get_child(0)->to_string(false,false) == ap) {
    delete s->replace_child(0, NULL);
    PTerm t = s->replace_child(1, NULL);
    delete s;
    return replace_encoded_applications(ap, t, arities);
  }

  // replace all the children
  for (int i = 0; i < s->number_children(); i++) {
    PTerm child = s->get_child(i);
    PTerm replacement = replace_encoded_applications(ap, child, arities);
    s->replace_child(i, replacement);
  }
  return s;
}

void RulesManipulator :: replace_encoded_applications(string ap,
                                                      Ruleset &rules) {
  for (int i = 0; i < rules.size(); i++) {
    PTerm lhs = rules[i]->query_left_side();
    PTerm rhs = rules[i]->query_right_side();
    // remove the rule defining ap
    if (lhs->query_head()->to_string(false,false) == ap) {
      delete rules[i];
      rules.erase(rules.begin()+i);
      i--;
      continue;
    }
    map<int,int> arities;
    replace_encoded_applications(ap, lhs, arities);
    PTerm r = replace_encoded_applications(ap, rhs, arities);
    if (r != rhs) {
      rules[i]->replace_right_side(r);
    }
  }
}

bool RulesManipulator :: remove_redundant_rules(Ruleset &rules) {
  bool found_duplicate = false;
  for (int i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();
    bool duplicate = false;
    for (int j = 0; !duplicate && j < rules.size(); j++) {
      if (i == j) continue;
      if (rules[j]->applicable(l)) {
        PTerm newr = rules[j]->apply(l->copy());
        duplicate = r->equals(newr);
        delete newr;
      }
    }
    if (duplicate) {
      delete rules[i];
      rules[i] = rules[rules.size()-1];
      rules.pop_back();
      found_duplicate = true;
      i--;
    }
  }
  return found_duplicate;
}

// note that only monomorphic applications will be considered
bool RulesManipulator :: simplify_applications(Alphabet &Sigma, Ruleset &rules) {
  vector<string> whatidid;

  set<string> candidates = identify_possible_applications(rules);
  for (set<string>::iterator it = candidates.begin();
       it != candidates.end(); it++) {
    string ap = *it;
    // only consider monomorphic symbols for now
    if (!Sigma.query_type(ap)->vars().empty()) continue;
    if (!used_as_application(ap, rules)) continue;
    replace_encoded_applications(ap, rules);
    Sigma.remove(ap);
    whatidid.push_back(ap);
  }

  bool removed_something = remove_redundant_rules(rules);

  // print what happened
  if (whatidid.size() != 0 || removed_something) {
    std::stringstream ss;
    if (whatidid.size() == 1) {
      ss << "Symbol " << whatidid[0] << " is an encoding for "
        "application that is only used in innocuous ways.  We can "
        "simplify the program (without losing non-termination) by "
        "removing it.";
    }
    else if (whatidid.size() > 1) {
      ss << "Symbols ";
      for (int i = 0; i < whatidid.size()-1; i++) {
        ss << whatidid[i] << ", ";
      }
      ss << "and " << whatidid[whatidid.size()-1] << " are encodings "
        "for application that are only used in innocuous ways.  We "
        "can simplify the program (without losing non-termination) "
        "by removing them.\n";
    }
    if (removed_something) {
      if (whatidid.size() > 0) ss << "  Additionally, we can remove "
        << "some (now-)redundant rules.";
      else ss << "We can remove some redundant rules (which are an "
        << "instance of another rule).";
    }
    ss << "  This gives:\n";
    string problem = ss.str();
    wout.print(problem);
    wout.print_system(Sigma, rules);
  }
  return whatidid.size() > 0;
}

