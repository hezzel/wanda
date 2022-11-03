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

#include "reqmodifier.h"
#include "environment.h"
#include "formula.h"
#include "term.h"
#include "beta.h"
#include "substitution.h"
#include "nonterminator.h"
#include "outputmodule.h"
#include "rulesmanipulator.h"
#include <iostream>
#include <fstream>

PConstant RequirementModifier :: fresh_constant(PType type) {
  char numnum[10];
  sprintf(numnum, "~c%d", Ccounter);
  Ccounter++;
  return new Constant(numnum, type->copy());
}

RequirementModifier :: RequirementModifier() {
  Ccounter = 0;
}

PTerm RequirementModifier :: substitute_variables(PTerm term,
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

Reqlist RequirementModifier :: create_basic_requirements(DPSet &D,
                                          Ruleset &A, bool extend) {
  int i;
  Reqlist ret;

  for (i = 0; i < D.size(); i++) {
    PTerm left = D[i]->query_left()->copy();
    PTerm right = D[i]->query_right();
    if (extend && right->query_application() &&
        right->query_head()->query_meta()) {
      vector<PTerm> parts = right->split();
      vector<PTerm> children;
      int i;
      for (i = 0; i < parts[0]->number_children(); i++) {
        children.push_back(parts[0]->get_child(i)->copy());
      }
      for (i = 1; i < parts.size(); i++) children.push_back(parts[i]);
      PVariable Z = dynamic_cast<MetaApplication*>(parts[0])->get_metavar();
      Z = dynamic_cast<PVariable>(Z->copy());
      right = new MetaApplication(Z, children);
    }
    else right = right->copy();
    Substitution gamma;
    set<int> binders;
    right = substitute_variables(right, gamma, binders);
    ret.push_back(new OrderingRequirement(left, right, ">?", i));
  }

  for (i = 0; i < A.size(); i++) {
    PTerm left = A[i]->query_left_side()->copy();
    Substitution gamma;
    set<int> binders;
    PTerm right = substitute_variables(A[i]->query_right_side()->copy(),
                                       gamma, binders);
    ret.push_back(new OrderingRequirement(left, right, ">="));
  }

  Ccounter = 0;
  return ret;
}

string RequirementModifier :: make_base(Reqlist &reqs) {
  string comment;

  for (int i = 0; i < reqs.size(); i++) {
    if (reqs[i]->desc != ">?") continue;
    bool changed = false;

    PTerm l = reqs[i]->left;
    PTerm r = reqs[i]->right;

    while (l->query_type()->query_composed()) {
      changed = true;
      PType inpt = l->query_type()->query_child(0);
      PVariable Z = new Variable(inpt->copy());
      MetaApplication *ma = new MetaApplication(Z);
      l = new Application(l, ma);
    }
    while (r->query_type()->query_composed()) {
      changed = true;
      PType inpt = r->query_type()->query_child(0);
      r = new Application(r, fresh_constant(inpt));
    }

    if (changed) {
      Environment gamma;
      comment += "Replacing dependency pair requirement: " +
                  reqs[i]->left->to_string(gamma) + " >? ";
      comment += reqs[i]->right->to_string(gamma) + "\nBy: ";
      Environment delta;
      comment += l->to_string(delta) + " >? ";
      comment += r->to_string(delta) + "\n\n";

      reqs[i]->left = l;
      reqs[i]->right = r;
    }
  }
  return comment;
}

void RequirementModifier :: lower(Reqlist &reqs) {
  vector<PTerm> subs;

  for (int i = 0; i < reqs.size(); i++) {
    subs.push_back(reqs[i]->left);
    subs.push_back(reqs[i]->right);
  }
  for (int j = 0; j < subs.size(); j++) {
    PTerm s = subs[j];
    for (int k = 0; k < s->number_children(); k++) {
      subs.push_back(s->get_child(k));
    }
    if (s->query_constant()) {
      PConstant f = dynamic_cast<PConstant>(s);
      string name = f->query_name();
      if (name[name.length()-1] == '#')
        name = name.substr(0,name.length()-1);
      f->rename(name);
    }
  }
}

PType RequirementModifier :: rename_and_collapse(PType type, string a,
                                                 PType renaming) {
  if (type->query_data()) {
    if (type->to_string() == a) return renaming->collapse();
    return type->collapse();
  }

  if (type->query_composed()) {
    PType lt = rename_and_collapse(type->query_child(0), a, renaming);
    PType rt = rename_and_collapse(type->query_child(1), a, renaming);
    return new ComposedType(lt, rt);
  }

  return type->copy();
}

PTerm RequirementModifier :: type_collapse(PTerm term,
                                  string renamable, PType renaming) {
  string a = renamable;
  PType b = renaming;

  if (term->query_variable()) {
    int index = dynamic_cast<PVariable>(term)->query_index();
    return new Variable(rename_and_collapse(term->query_type(), a, b), index);
  }
  if (term->query_constant()) {
    string name = dynamic_cast<PConstant>(term)->query_name();
    return new Constant(name, rename_and_collapse(term->query_type(), a, b));
  }
  if (term->query_application()) {
    return new Application(type_collapse(term->subterm("1"), a, b),
                           type_collapse(term->subterm("2"), a, b));
  }
  if (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    PVariable x = dynamic_cast<PVariable>(type_collapse(
                              abs->query_abstraction_variable(), a, b));
    PTerm sub = type_collapse(abs->subterm("1"), a, b);
    return new Abstraction(x,sub);
  }
  if (term->query_meta()) {
    MetaApplication *meta = dynamic_cast<MetaApplication*>(term);
    PVariable Z = dynamic_cast<PVariable>(type_collapse(meta->get_metavar(), a, b));
    vector<PTerm> subs;
    for (int i = 0; i < meta->number_children(); i++) {
      PTerm sub = meta->get_child(i);
      subs.push_back(type_collapse(sub, a, b));
    }
    return new MetaApplication(Z, subs);
  }
  return NULL;
}

PTerm RequirementModifier :: function_hider(PTerm term, bool top) {
  if (!top && term->query_type()->query_data()) {
    // does this base-type term have a functional meta-variable as a child?
    vector<PTerm> args = term->split();
    for (int i = 1; i < args.size(); i++) {
      if (args[i]->query_meta() &&
          args[i]->query_type()->query_composed()) return term;
      while (args[i]->query_abstraction())
        args[i] = args[i]->get_child(0);
      if (args[i]->query_meta() && args[i]->number_children() > 0)
        return term;
    }
  }

  // check for hiders in the children!
  for (int i = 0; i < term->number_children(); i++) {
    PTerm subhider = function_hider(term->get_child(i), false);
    if (subhider != NULL) return subhider;
  }

  return NULL;
}

bool RequirementModifier :: type_contains(PType big, DataType *small) {
  if (big->query_data()) return big->equals(small);
  if (big->query_composed())
    return type_contains(big->query_child(0), small) ||
           type_contains(big->query_child(1), small);
  return true;
    // wuh - polymorphism, might contain anything!
}

string RequirementModifier :: collapse_types(Reqlist &reqs, Alphabet &F,
                                             Alphabet &newF, ArList &ars) {
  string substtype;
  PType substitution = NULL;
  int i;
  bool monomorphic = true;
  string ret;

  // if the system isn't monomorphic, we're not going to rename anything
  for (i = 0; i < reqs.size() && monomorphic; i++) {
    if (!reqs[i]->left->free_typevar().empty() ||
        !reqs[i]->right->free_typevar().empty()) {
      monomorphic = false;
    }
  }

  // if collapsing a rule would lead to non-termination, try changing
  // one of the types to be functional
  for (i = 0; i < reqs.size() && monomorphic; i++) {
    PTerm hider = function_hider(reqs[i]->left);
    if (hider != NULL) {
      PTerm l = type_collapse(reqs[i]->left, "", NULL);
      PTerm r = type_collapse(reqs[i]->right, "", NULL);
      MatchRule *rule = new MatchRule(l,r);
      vector<MatchRule*> rules;
      rules.push_back(rule);
      wout.start_method("non-termination test");
      NonTerminator NT(F, rules, false);
      if (NT.lambda_calculus(rule)) {
        // collapsing causes non-termination!

        // find out the type of the offending argument
        vector<PTerm> splitted = hider->split();
        PType offending_type = NULL;
        for (int j = 1; j < splitted.size() && offending_type == NULL; j++) {
          if (!splitted[j]->query_type()->query_composed()) continue;
          if (!splitted[j]->query_meta() && !splitted[j]->query_abstraction())
            continue;
          offending_type = splitted[j]->query_type();
          if (splitted[j]->query_meta()) break;
          while (splitted[j]->query_abstraction())
            splitted[j] = splitted[j]->get_child(0);
          if (!splitted[j]->query_meta() ||
              splitted[j]->number_children() == 0) offending_type = NULL;
        }

        // is this a valid type renaming?
        if (offending_type != NULL && type_contains(offending_type,
            dynamic_cast<DataType*>(hider->query_type()))) {
          offending_type = NULL;
        }

        // everything looks good, just do a type substitution
        if (offending_type != NULL) {
          substtype = hider->query_type()->to_string();
          substitution = offending_type;
          ret += "Using type change function: mapping " + substtype +
                 " to " + substitution->to_string() + ".\n\n";
        }
        delete rule;
      }
      wout.abort_method("non-termination test");
    }
  }

  // collapse all (remaining) base types!
  for (i = 0; i < reqs.size(); i++) {
    PTerm l = type_collapse(reqs[i]->left, substtype, substitution);
    PTerm r = type_collapse(reqs[i]->right, substtype, substitution);
    delete reqs[i]->left;
    delete reqs[i]->right;
    reqs[i]->left = l;
    reqs[i]->right = r;
  }

  // also rename the alphabet
  for (ArList::iterator it = ars.begin(); it != ars.end(); it++) {
    string f = it->first;
    if (F.contains(f)) newF.add(f,
      rename_and_collapse(F.query_type(f), substtype, substitution));
    else {
      string original_f = f.substr(0, f.length()-1);
      if (original_f.substr(0,2) != "~c")
        newF.add(f, rename_and_collapse(F.query_type(original_f),
          substtype, substitution));
    }
  }

  return ret;
}

bool RequirementModifier :: symbol_occurs(string symbol, PTerm term) {
  if (term->query_constant()) return term->to_string(false) == symbol;
  for (int i = 0; i < term->number_children(); i++)
    if (symbol_occurs(symbol, term->get_child(i))) return true;
  return false;
}

string RequirementModifier :: remove_consequences(Reqlist &reqs) {
  int i, j;
  string ret;

  for (i = 0; i < reqs.size(); i++) {
    if (reqs[i] == NULL) continue;
    for (j = 0; j < reqs.size(); j++) {
      if (i == j || reqs[j] == NULL) continue;
      // we will remove reqs[j] if it is implied by reqs[i]

      if (reqs[i]->desc == ">=" && reqs[j]->desc == ">?") continue;
      Renaming rn;
      if (!always_ge(reqs[j]->left, reqs[i]->left, rn, true)) continue;
      if (!always_ge(reqs[i]->right, reqs[j]->right, rn, false)) continue;

      // so reqs[i] = l R r and reqs[j] = u R' v, and
      // u >= l R r >= v, and R is stronger than R' => remove u R' v
      Environment gamma, delta;
      string req1 = reqs[i]->left->to_string(gamma) + " " +
                    reqs[i]->desc + " ";
      req1 += reqs[i]->right->to_string(gamma);
      string req2 = reqs[j]->left->to_string(delta) + " " +
                    reqs[j]->desc + " ";
      req2 += reqs[j]->right->to_string(delta);
      if (req1 == req2)
        ret += "Removing duplicate requirement [" + req1 + "]\n";
      else {
        ret += "Requirement [" + req2 + "] is implied by " +
               "requirement [" + req1 + "], removing it.\n";
      }
      if (reqs[i]->desc == ">?" && reqs[j]->desc == ">?") {
        for (int k = 0; k < reqs[j]->data.size(); k++) {
          reqs[i]->data.push_back(reqs[j]->data[k]);
        }
      }
      delete reqs[j];
      reqs[j] = NULL;
    }
  }

  for (i = 0, j = 0; i < reqs.size(); i++) {
    if (reqs[i] == NULL) continue;
    reqs[j] = reqs[i];
    j++;
  }
  while (i > j) {
    reqs.pop_back();
    i--;
  }

  return ret;
}

bool RequirementModifier :: always_ge(PTerm a, PTerm b, Renaming &rn,
                                      bool lhs) {
  // only terms of equal types can be compared
  if (!a->query_type()->equals(b->query_type())) return false;

  if (b->query_constant() &&
      dynamic_cast<PConstant>(b)->query_name().substr(0,2) == "~c")
    return true;

  // the case when a is a variable (potentially a meta-variable):
  // in the left-hand side, a >= b if and only if a = b (after
  // possibly renaming b, if b hasn't already been renamed)
  // in the right-hand side, a >= b if and only if a = b (using the
  // existing renaming for a)
  if (a->query_variable()) {
    if (!b->query_variable()) return false;
    int id1 = dynamic_cast<PVariable>(a)->query_index();
    int id2 = dynamic_cast<PVariable>(b)->query_index();
    if (lhs) {
      if (rn.find(id2) == rn.end()) rn[id2] = id1;
      return rn[id2] == id1;
    }
    else {
      if (rn.find(id1) == rn.end()) return false;
      return rn[id1] == id2;
    }
  }

  if (a->query_meta()) {
    PVariable avar = dynamic_cast<MetaApplication*>(a)->get_metavar();
    if (!lhs && rn.find(avar->query_index()) == rn.end()) return true;
      // if a is a fresh meta-variable, it might be instantiated with
      // anything, so is, in principle, infinitely large
    if (!b->query_meta()) return false;
    PVariable bvar = dynamic_cast<MetaApplication*>(b)->get_metavar();
    if (!always_ge(avar, bvar, rn, lhs)) return false;
    if (a->number_children() != b->number_children()) return false;
    for (int i = 0; i < a->number_children(); i++) {
      if (!always_ge(a->get_child(i), b->get_child(i), rn, lhs))
        return false;
    }
    return true;
  }

  if (a->query_abstraction()) {
    if (!b->query_abstraction()) return false;
    PVariable avar =
      dynamic_cast<Abstraction*>(a)->query_abstraction_variable();
    PVariable bvar =
      dynamic_cast<Abstraction*>(b)->query_abstraction_variable();
    if (lhs) rn[bvar->query_index()] = avar->query_index();
    else rn[avar->query_index()] = bvar->query_index();
    bool ret = always_ge(a->get_child(0), b->get_child(0), rn, lhs);
    if (lhs) rn.erase(bvar->query_index());
    else rn.erase(avar->query_index());
    return ret;
  }

  if (a->query_constant()) return a->equals(b);

  if (!a->query_application()) return false;    // errrr

  // it's an application - first check whether all the parts of the
  // applicatiion compare placewise
  Renaming backup = rn;
  vector<PTerm> asplit = a->split();
  vector<PTerm> bsplit = b->split();
  bool ok = asplit.size() == bsplit.size();
  for (int i = 0; ok && i < asplit.size(); i++) {
    if (!always_ge(asplit[i], bsplit[i], rn, lhs)) ok = false;
  }
  if (ok) return true;
  rn = backup;

  // No? If the head is an abstraction, try a beta-reduct.
  if (asplit[0]->query_abstraction()) {
    PTerm ac = a->copy();
    Beta beta;
    string pos = "";
    for (int j = 2; j < asplit.size(); j++) pos += "1";
    ac = beta.apply(ac, pos);
    bool ret = always_ge(ac, b, rn, lhs);
    delete ac;
    if (ret) return ret;
    else rn = backup;
  }

  // otherwise, check whether it might be a subterm, but not if the
  // head is a constant that might be filtered
  if (asplit[0]->query_constant()) {
    string name = dynamic_cast<PConstant>(asplit[0])->query_name();
    if (name == "" || (name[name.length()-1] != '-' &&
        name.substr(0,2) != "~c"))
      return false;
  }

  PType btype = b->query_type();
  for (int j = 1; j < asplit.size(); j++) {
    PTerm sub = asplit[j];
    if (btype->query_data()) {
      while (sub->query_abstraction())
        sub = sub->get_child(0);
    }
    if (always_ge(sub, b, rn, lhs)) return true;
    else rn = backup;
  }

  // all options are exhausted!
  return false;
}

void RequirementModifier :: get_all_symbols(PTerm term, set<string> &symbs) {
  if (term->query_constant()) symbs.insert(term->to_string(false));
  for (int i = 0; i < term->number_children(); i++)
    get_all_symbols(term->get_child(i), symbs);
}

bool RequirementModifier :: simple_argument_functions(Reqlist &reqs,
                                                        Alphabet &F,
                                                        ArList &arities) {
  int i, j, k;
  RulesManipulator manip;

  // we make a set of all defined symbols, and a separate set of
  // all symbols occurring below the root in a left-hand side
  set<string> allconstants;
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
    if (f[f.length()-1] == '-') continue;
    if (!F.query_type(f)->vars().empty()) continue;
    // store f - it's a reasonable defined symbol!
    allconstants.insert(f);
  }

  // symbols which occur below a root on the left are not suitable
  // for argument functions
  for (set<string>::iterator cit = belowroot.begin();
       cit != belowroot.end(); cit++) {
    allconstants.erase(*cit);
  }
  if (allconstants.empty()) return false;

  // we filter the set to only contain defined symbols which occur
  // only in rules of the form f x1 ... xn -> r
  for (i = 0; i < reqs.size(); i++) {
    vector<PTerm> split = reqs[i]->left->split();
    string f = split[0]->to_string(false);
    if (allconstants.find(f) == allconstants.end()) continue;
    bool ok = true;
    for (j = 1; j < split.size() && ok; j++) {
      ok &= (manip.extended_variable(split[j]) != NULL);
    }
    if (ok) {
      Varset leftmeta = reqs[i]->left->free_var(true);
      Varset rightmeta = reqs[i]->right->free_var(true);
      if (!leftmeta.contains(rightmeta)) ok = false;
    }
    if (!ok) allconstants.erase(f);
  }
  if (allconstants.empty()) return false;

  // make a mapping of the likely symbols to the function symbols
  // occurring in their right-hand sides
  map<string, set<string> > constantmapping;
  set<string>::iterator it, it2;
  set<string> remove;
  for (it = allconstants.begin(); it != allconstants.end(); it++) {
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
    allconstants.erase(*it);

  // orient the symbols, so if f <= g, then g does not occur in a
  // right-hand side of f
  vector<string> orientedsymbols;
  bool go_on = true;
  while (go_on && !allconstants.empty()) {
    go_on = false;
    for (it = allconstants.begin(); it != allconstants.end(); it++) {
      string f = *it;
      bool ok = true;
      for (it2 = allconstants.begin();
           it2 != allconstants.end() && ok; it2++) {
        string g = *it2;
        if (constantmapping[f].find(g) != constantmapping[f].end())
          ok = false;
      }
      if (ok) {
        orientedsymbols.push_back(f);
        allconstants.erase(f);
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
    PType ftype = F.query_type(f);
    PTerm left = F.get(f);
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
    while (F.contains(symbol)) symbol += "'";
    PType type = ftype->copy();
    for (j = 0; j < parts.size(); j++)
      type = new ComposedType(ftype->copy(), type);
    F.add(symbol, type);
    arities[symbol] = parts.size();

    // create the right-hand side! ?
    PTerm right = F.get(symbol);
    for (j = 0; j < parts.size(); j++)
      right = new Application(right, parts[j]);

    // and show it!
    map<int,string> meta, free, bound;
    vector<string> entry;
    entry.push_back(wout.pi_symbol() + "(");
    entry.push_back(wout.print_term(left, arities, F, meta, free, bound));
    entry.push_back(")");
    entry.push_back("=");
    entry.push_back(wout.print_term(right, arities, F, meta, free, bound));
    wout.table_entry(entry);

    // execute the argument function
    execute_argument_function(reqs, left, right);
  }

  wout.end_table();
  wout.succeed_method("argument function");

  return true;
}

Varset RequirementModifier :: unfiltered_metavars(PTerm r, ArList &arities) {
  Varset ret;

  vector<PTerm> subs;
  subs.push_back(r);

  while (!subs.empty()) {
    PTerm s = subs.back();
    subs.pop_back();
    while (s->query_abstraction()) s = s->get_child(0);
    if (s->query_meta()) {
      ret.add(dynamic_cast<MetaApplication*>(s)->get_metavar());
      continue;
    }
    if (!s->query_application()) continue;

    vector<PTerm> children = s->split();
    if (!children[0]->query_constant()) {
      for (int i = 0; i < children.size(); i++)
        subs.push_back(children[i]);
      continue;
    }

    string f = dynamic_cast<PConstant>(children[0])->query_name();
    if (f != "" && f[f.length()-1] == '-') {
      for (int i = 1; i < children.size() && i <= arities[f]; i++)
        subs.push_back(children[i]);
    }
    int ar = 0;
    if (f == "" || f.substr(0,2) != "~c") ar = arities[f];
    for (int i = ar+1; i < children.size(); i++)
      subs.push_back(children[i]);
  }

  return ret;
}

void RequirementModifier :: execute_argument_function(Reqlist &reqs,
                                                  PTerm l, PTerm r) {
  MatchRule rule(l,r);
  for (int i = 0; i < reqs.size(); i++) {
    reqs[i]->left = rule.normalise(reqs[i]->left);
    reqs[i]->right = rule.normalise(reqs[i]->right);
  }
}

ArList RequirementModifier :: get_arities_in(Reqlist &reqs, ArList &original) {
  ArList ret;
  vector<PTerm> subs;
  int i;

  for (i = 0; i < reqs.size(); i++) {
    subs.push_back(reqs[i]->left);
    subs.push_back(reqs[i]->right);
  }
  for (i = 0; i < subs.size(); i++) {
    PTerm s = subs[i];
    while (s->query_abstraction()) s = s->get_child(0);
    for (int j = 0; j < s->number_children(); j++) {
      subs.push_back(s->get_child(j));
    }
    if (s->query_constant()) {
      string f = dynamic_cast<PConstant>(s)->query_name();
      if (f == "") ret[f] = original[f];
      else if (f.length() > 2 && f[f.length()-1] == '#' &&
               f[f.length()-2] == '^')
        ret[f] = original[f.substr(0, f.length()-2)];
      else if (f[f.length()-1] == '-' || f[f.length()-1] == '#')
        ret[f] = original[f.substr(0,f.length()-1)];
      else if (f.substr(0,2) != "~c") ret[f] = original[f];
    }
  }

  return ret;
}

void RequirementModifier :: replace_unfiltered_constants(PTerm term,
                                                    ArList &arities,
                                           bool below_abstraction) {
  vector<PTerm> parts = term->split();
  for (int i = 1; i < parts.size(); i++) {
    replace_unfiltered_constants(parts[i], arities, below_abstraction);
  }

  if (parts[0]->query_abstraction()) {
    replace_unfiltered_constants(parts[0]->get_child(0), arities, true);
  }

  else if (parts[0]->query_meta()) {
    for (int j = 0; j < parts[0]->number_children(); j++)
      replace_unfiltered_constants(parts[0]->get_child(j), arities, true);
  }

  else if (parts[0]->query_constant() && below_abstraction) {
    bool inabstraction = false;
    string f = parts[0]->to_string(false);
    int arity = arities[f];
    for (int j = 0; j < arity && j+1 < parts.size(); j++) {
      if (parts[j+1]->free_var().size() != 0) {
        inabstraction = true;
        break;
      }
    }
    if (inabstraction) {
      dynamic_cast<Constant*>(parts[0])->rename(f + "-");
    }
  }
}

void RequirementModifier :: tag_right_sides(Reqlist &reqs,
                                            ArList &arities) {
  for (int i = 0; i < reqs.size(); i++) {
    replace_unfiltered_constants(reqs[i]->right, arities, false);
  }
}

void RequirementModifier :: tag_all_symbols(Reqlist &reqs) {
  // put all subterms of a requirement in the list "subs", and
  // for all constants occurring in the list, tag them
  vector<PTerm> subs;
  for (int i = 0; i < reqs.size(); i++) {
    subs.push_back(reqs[i]->left);
    subs.push_back(reqs[i]->right);
  }
  while (!subs.empty()) {
    PTerm s = subs[subs.size()-1];
    subs.pop_back();
    for (int i = 0; i < s->number_children(); i++)
      subs.push_back(s->get_child(i));
    if (s->query_constant()) {
      PConstant c = dynamic_cast<PConstant>(s);
      c->rename(c->query_name() + "-");
    }
  }
}

void RequirementModifier :: add_untagging_reqs(Reqlist &reqs,
                                Alphabet &F, ArList &arities) {
  // find all tagged symbols occurring anywhere in a requirement
  // and all untagged and marked symbols
  set<string> tagged;
  set<string> marked;
  set<string> untagged;
  vector<PTerm> subs;
  for (int i = 0; i < reqs.size(); i++) {
    subs.push_back(reqs[i]->left);
    subs.push_back(reqs[i]->right);
  }
  while (!subs.empty()) {
    PTerm s = subs[subs.size()-1];
    subs.pop_back();
    for (int i = 0; i < s->number_children(); i++)
      subs.push_back(s->get_child(i));
    if (s->query_constant()) {
      string name = dynamic_cast<Constant*>(s)->query_name();
      if (name == "") { untagged.insert(name); continue; }
      else if (name[name.length()-1] == '-')
        tagged.insert(name.substr(0, name.length()-1));
      else if (name.length() > 2 && name[name.length()-1] == '#' &&
               name[name.length()-2] == '^')
        marked.insert(name.substr(0, name.length()-2));
      else untagged.insert(name);
    }
  }

  // for all tagged symbols, add rules and a function symbol!
  for (set<string>::iterator it = tagged.begin(); it != tagged.end();
       it++) {
    string name = *it;
    for (int m = 0; m <= 1; m++) {
      if (m == 0 && untagged.find(name) == untagged.end()) continue;
      if (m == 1 && marked.find(name) == marked.end()) continue;

      PType type = F.query_type(name);
      PTerm l = new Constant(name + "-", type->copy());
      PTerm r = new Constant(m == 0 ? name : name + "^#", type->copy());
      int n = arities[name];
      for (int i = 0; i < n && l->query_type()->query_composed(); i++) {
        PVariable v = new Variable(l->query_type()->query_child(0)->copy());
        MetaApplication *Z = new MetaApplication(v);
        l = new Application(l, Z);
        r = new Application(r, Z->copy());
      }
      reqs.push_back(new OrderingRequirement(l, r, ">="));
    }
    if (!F.contains(name + "-")) {
      F.add(name + "-", F.query_type(name)->copy());
      arities[name + "-"] = arities[name];
    }
  }
}

void RequirementModifier :: add_upping_reqs(Reqlist &reqs, Alphabet &F) {
  ArList arities;
  vector<string> names;
  vector<int> argnums;
  int i;

  // find all marked symbols actually occurring in a rule
  for (i = 0; i < reqs.size(); i++) {
    vector<PTerm> lsplit = reqs[i]->left->split();
    vector<PTerm> rsplit = reqs[i]->right->split();
    if (lsplit[0]->query_constant()) {
      names.push_back(lsplit[0]->to_string(false));
      argnums.push_back(lsplit.size()-1);
    }
    if (rsplit[0]->query_constant()) {
      names.push_back(rsplit[0]->to_string(false));
      argnums.push_back(rsplit.size()-1);
    }
  }

  // determine original names and arities
  for (i = 0; i < names.size(); i++) {
    string name = names[i];
    if (name.length() < 2) continue;
    if (name[name.length()-2] != '^' || name[name.length()-1] != '#')
      continue;
    name = name.substr(0, name.length()-2);
    if (arities.find(name) == arities.end())
      arities[name] = argnums[i];
    else if (arities[name] > argnums[i]) arities[name] = argnums[i];
  }

  // for all symbols we found, add rules!
  for (ArList::iterator it = arities.begin(); it != arities.end();
       it++) {
    string name = it->first;
    int arity = it->second;
    PType type = F.query_type(name);
    PTerm l = new Constant(name, type->copy());
    PTerm r = new Constant(name + "^#", type->copy());
    for (int i = 0; i < arity && l->query_type()->query_composed(); i++) {
      PVariable v = new Variable(l->query_type()->query_child(0)->copy());
      MetaApplication *Z = new MetaApplication(v);
      l = new Application(l, Z);
      r = new Application(r, Z->copy());
    }
    reqs.push_back(new OrderingRequirement(l, r, ">="));
  }
}

void RequirementModifier :: eta_expand(Reqlist &reqs) {
  RulesManipulator manip;
  for (int i = 0; i < reqs.size(); i++) {
    PTerm left = manip.eta_expand(reqs[i]->left);
    PTerm right = manip.eta_expand(reqs[i]->right);
    delete reqs[i]->left;
    delete reqs[i]->right;
    reqs[i]->left = left;
    reqs[i]->right = right;
  }
}

PTerm RequirementModifier :: eta_everything_in(PTerm term, ArList &ars) {
  for (int i = 0; i < term->number_children(); i++) {
    term->replace_child(i, eta_everything_in(term->get_child(i), ars));
  }
  if (term->query_type()->query_composed()) {
    vector<PTerm> parts = term->split();
    if (!parts[0]->query_constant() ||
        ars[parts[0]->to_string()] <= parts.size()-1) {
      PType xtype = term->query_type()->query_child(0)->copy();
      PVariable x = new Variable(xtype);
      return new Abstraction(x, new Application(term, x->copy()));
    }
  }
  return term;
}

void RequirementModifier :: eta_everything(Reqlist &reqs, ArList &ars) {
  for (int i = 0; i < reqs.size(); i++) {
    reqs[i]->left = eta_everything_in(reqs[i]->left, ars);
    reqs[i]->right = eta_everything_in(reqs[i]->right, ars);
  }
}

void RequirementModifier :: remove_marks(Reqlist &reqs) {
  for (int i = 0; i < reqs.size(); i++) {
    PTerm lhead = reqs[i]->left->query_head();
    if (lhead->query_constant()) {
      PConstant f = dynamic_cast<PConstant>(lhead);
      string fname = f->query_name();
      if (fname != "" && fname[fname.length()-1] == '#')
        f->rename(fname.substr(0, fname.length()-1));
    }
    PTerm rhead = reqs[i]->right->query_head();
    if (rhead->query_constant()) {
      PConstant g = dynamic_cast<PConstant>(rhead);
      string gname = g->query_name();
      if (gname != "" && gname[gname.length()-1] == '#')
        g->rename(gname.substr(0, gname.length()-1));
    }
  }
}

