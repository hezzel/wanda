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

#include "horpo.h"
#include "substitution.h"
#include <cstdio>
#include <iostream>

string HorpoConstraintList :: print_horpo_constraint(PTerm left, PTerm right,
              string relation, PTerm restriction_term, int restriction_num) {
  string ret = left->to_string(environment, true) + " " + relation + " " +
               right->to_string(environment, true);
  if (restriction_term != NULL) {
    char num[] = " ";
    num[0] = '0' + restriction_num;
    ret += " RST (" + restriction_term->to_string(environment, true) + "," +
                 string(num) + ")";
  }
  return ret;
}

string HorpoConstraintList :: print_horpo_constraint(HorpoConstraint *hc) {
  return print_horpo_constraint(hc->left, hc->right, hc->relation,
                                hc->restriction_term, hc->restriction_num);
}

HorpoConstraint :: HorpoConstraint(PTerm _left, PTerm _right,
                     string _relation, PTerm _resterm, int _resnum,
                     int _varindex) {
  left = _left;
  right = _right;
  relation = _relation;
  restriction_term = _resterm;
  restriction_num = _resnum;
  variable_index = _varindex;
}

HorpoConstraint :: ~HorpoConstraint() {
  delete left;
  delete right;
  if (restriction_term != NULL) delete restriction_term;
}

HorpoConstraintList :: HorpoConstraintList(Horpo *_master) {
  master = _master;
  handled = 0;
}

void HorpoConstraintList :: reset() {
  int i;

  handled = 0;
  for (i = 0; i < formulas.size(); i++) delete formulas[i];
  formulas.clear();
  for (i = 0; i < constraints.size(); i++) delete constraints[i];
  constraints.clear();
}

int HorpoConstraintList :: add_constraint(PTerm left, PTerm right,
                    string relation, PTerm restriction, int limit) {
  int num = vars.query_size();
  HorpoConstraint *constraint = new HorpoConstraint(left, right,
                                  relation, restriction, limit, num);
  string printed = print_horpo_constraint(constraint);
  vars.add_vars(1);
  char strid[10];
  sprintf(strid, "%d", constraints.size());
  vars.set_description(num, "constraint " + string(strid));
  //vars.set_description(num, "[[" + printed + "]]");

  constraint_lookup[printed] = constraints.size();
  constraints.push_back(constraint);

  return num;
}

int HorpoConstraintList :: add_geq(PTerm left, PTerm right) {
  return add_constraint(left, right, ">=", NULL, 0);
}

int HorpoConstraintList :: add_greater(PTerm left, PTerm right) {
  return add_constraint(left, right, ">", NULL, 0);
}

void HorpoConstraintList :: add_formula(PFormula formula) {
  formulas.push_back(formula);
}

int HorpoConstraintList :: index_of_constraint(PTerm left, PTerm right,
        string relation, PTerm restriction_term, int restriction_num) {

  string tmp = print_horpo_constraint(left, right, relation,
                                      restriction_term, restriction_num);
  if (constraint_lookup.find(tmp) == constraint_lookup.end()) {
    // the constraint doesn't exist yet - add it!
    add_constraint(left->copy(), right->copy(), relation,
                   restriction_term == NULL ? NULL : restriction_term->copy(),
                   restriction_num);
    return constraints.size()-1;
  }
  else return constraint_lookup[tmp];
}

int HorpoConstraintList :: var_for_constraint(PTerm left, PTerm right,
                         string relation, PTerm resterm, int resnum) {
  
  int constraint_index = index_of_constraint(left, right, relation,
                                             resterm, resnum);
  return constraints[constraint_index]->variable_index;
}

int HorpoConstraintList :: query_constraint_variable(PTerm left, PTerm
                                             right, string relation) {
  int index = constraint_lookup[print_horpo_constraint(left, right,
                                                relation, NULL, 0)];
  return constraints[index]->variable_index;
}

And *HorpoConstraintList :: generate_complete_formula() {
  And *ret = new And();
  for (int i = 0; i < formulas.size(); i++)
    ret->add_child(formulas[i]->copy());
  return ret;
}

/*
bool HorpoConstraintList :: occurs(int var, PFormula form, bool pos) {
  if (form->query_variable() || form->query_antivariable()) {
    int id = dynamic_cast<Atom*>(form)->query_index();
    if (id != var) return false;
    if (form->query_variable() && !pos) return false;
    if (form->query_antivariable() && pos) return false;
    return true;
  }

  if (form->query_conjunction() || form->query_disjunction()) {
    AndOr *andor = dynamic_cast<AndOr*>(form);
    bool ok = false;
    for (int i = 0; i < andor->query_number_children() && !ok; i++) {
      ok |= occurs(var, andor->query_child(i), pos);
    }
    return ok;
  }

  if (form->query_negation())
    return occurs(var, dynamic_cast<Not*>(form)->query_child(), !pos);

  return false;
}

vector<PFormula> HorpoConstraintList :: get_occurrences(int variable,
                                                     bool positive) {
  vector<PFormula> ret;
  
  for (int i = 0; i < formulas.size(); i++) {
    if (occurs(variable, formulas[i], positive))
      ret.push_back(formulas[i]);
  }

  return ret;
}
*/

HorpoConstraint *HorpoConstraintList :: constraint_by_index(int index) {
  for (int i = 0; i < constraints.size(); i++) {
    if (constraints[i]->variable_index == index)
      return constraints[i];
  }
  return NULL;
}

string HorpoConstraintList :: print() {
  string ret = "";
  for (int i = 0; i < constraints.size(); i++) {
    char tmp[10];
    sprintf(tmp, "%d", i); //constraints[i]->variable_index);
    ret += string(tmp) + "] " + print_horpo_constraint(constraints[i]) + "\n";
  }
  ret += "\n";
  for (int j = 0; j < formulas.size(); j++) {
    ret += formulas[j]->to_string() + "\n";
  }
  return ret;
}

/* ========== SIMPLIFYING ========== */

bool HorpoConstraintList :: markable(PTerm term) {
  while (term->query_abstraction())
    term = term->get_child(0);
  while (term->query_application())
    term = term->get_child(0);

  if (!term->query_constant()) return false;
  string name = dynamic_cast<PConstant>(term)->query_name();
  return name[name.length()-1] != '*';
}

PConstant HorpoConstraintList :: get_main_symbol(PTerm term) {
  while (term->query_abstraction())
    term = term->get_child(0);
  while (term->query_application())
    term = term->get_child(0);

  if (!term->query_constant())
    cout << "Error in get_main_symbol!" << endl;

  return dynamic_cast<PConstant>(term);
}

Or *HorpoConstraintList :: bigOr(PFormula a, PFormula b,
                                 PFormula c, PFormula d) {
  Or *ret = new Or(a,b,c);
  ret->add_child(d);
  return ret;
}

Or *HorpoConstraintList :: bigOr(PFormula a, PFormula b,
                                 PFormula c, PFormula d,
                                 PFormula e) {
  Or *ret = new Or(a,b,c);
  ret->add_child(d);
  ret->add_child(e);
  return ret;
}

PTerm HorpoConstraintList :: retype(PTerm term, PType newtype) {
  if (term->query_constant())
    return new Constant(term->to_string(false), newtype);
  
  if (!term->query_application()) {
    cout << "ERROR: trying to retype something of a strange form" << endl;
    return NULL;
  }

  PTerm right = term->get_child(1);
  newtype = new ComposedType(right->query_type()->copy(), newtype);
  PTerm left = retype(term->get_child(0), newtype);
  return new Application(left, right);
}

void HorpoConstraintList :: destroy_retyped(PTerm term) {
  if (term->query_application()) {
    destroy_retyped(term->replace_child(0, NULL));
    term->replace_child(1, NULL);
  }
  delete term;
}

void HorpoConstraintList :: handle_greater(PTerm left,
                                           PTerm right,
                                           int index) {
  // if left isn't markable, then it still isn't markable
  // after filtering!
  if (!markable(left)) {
    add_formula(new AntiVar(index));
    return;
  }

  // find (and maybe save) the constraint left* >= right
  PConstant f = get_main_symbol(left);
  string fname = f->query_name();
  
  f->rename(fname + "*");
  int newid = var_for_constraint(left, right, ">=");
  f->rename(fname);

  // either f is filtered away, of we should mark it
  // (in this case it cannot be minimal)
  add_formula(new Or(
      new AntiVar(index),
      new AntiVar(master->minimal(fname))
    ));
  add_formula(new Or(
      new AntiVar(index),
      new Var(master->symbol_filtered(fname)),
      new Var(newid)
    ));

  // if f is filtered away, the correct child must be > right
  PTerm leftsub = left;
  string subpos = "";
  while (leftsub->query_abstraction()) {
    leftsub = leftsub->get_child(0);
    subpos += "1";
  }
  vector<PTerm> split = leftsub->split();

  for (int i = 1; i < split.size(); i++) {
    PTerm li = split[i];
    if (leftsub == left) {
      newid = var_for_constraint(li, right, ">");
    }
    else {
      PTerm tmp = left->replace_subterm(li, subpos);
      newid = var_for_constraint(left, right, ">");
      left->replace_subterm(tmp, subpos);
    }
    add_formula(bigOr(
        new AntiVar(index),
        new AntiVar(master->symbol_filtered(fname)),
        new Var(master->arg_filtered(fname, i)),
        new Var(newid)
      ));
  }
}

void HorpoConstraintList :: handle_basic_geq(PTerm left,
                                             PTerm right,
                                             int index) {
  // if the right-hand side is not functional, then it is "standard"
  vector<PTerm> split = right->split();
  if (!split[0]->query_constant()) {
    add_formula(new Or(
        new AntiVar(index),
        new Var(var_for_constraint(left, right, ">=stdr"))
      ));
    return;
  }
  
  // either the main character of the right-hand side is minimal,
  // or it is filtered away, or the right-hand side is standard
  PConstant g = dynamic_cast<PConstant>(split[0]);
  string gname = g->query_name();

  add_formula(bigOr(
      new AntiVar(index),
      new Var(master->symbol_filtered(gname)),
      new Var(master->minimal(gname)),
      new Var(var_for_constraint(left, right, ">=stdr"))
    ));

  // if g is filtered away, then left should still be >= its child
  for (int i = 1; i < split.size(); i++) {
    add_formula(bigOr(
        new AntiVar(index),
        new AntiVar(master->symbol_filtered(gname)),
        new Var(master->arg_filtered(gname, i)),
        new Var(var_for_constraint(left, split[i], ">="))
      ));
  }
}

void HorpoConstraintList :: handle_standard_right(PTerm left,
                                                  PTerm right,
                                                  int index,
                                                  PTerm resterm,
                                                  int resnum) {
  
  // if left == right, then we don't need to add any constraints
  // note that variables are considered equal if their indexes are
  // equal, because the types might actually be different (due to
  // type changing functions, e.g. collapsing of base types)
  if (left->query_variable() && right->query_variable() &&
      dynamic_cast<PVariable>(left)->query_index() ==
      dynamic_cast<PVariable>(right)->query_index()) return;

  // if left == Z(l1,...,ln) and right == Z(r1,...,rn), then we
  // merely need that l1 >= r1, ..., ln >= rn
  if (left->query_meta() && right->query_meta()) {
    PVariable Zl = dynamic_cast<MetaApplication*>(left)->get_metavar();
    PVariable Zr = dynamic_cast<MetaApplication*>(right)->get_metavar();
    if (Zl->equals(Zr) &&
        left->number_children() == right->number_children()) {
      for (int i = 0; i < left->number_children(); i++) {
        add_formula(new Or(
            new AntiVar(index),
            new Var(var_for_constraint(left->get_child(i),
                                       right->get_child(i), ">="))
          ));
      }
      return;
    }
  }

  // if left == /\x.l and right == /\y.r, then we merely need that
  // l >= r[y:=x] -- note, however, that x and y only have the same
  // type modulo the type changing function!
  if (left->query_abstraction() && right->query_abstraction()) {
    Abstraction *l = dynamic_cast<Abstraction*>(left);
    Abstraction *r = dynamic_cast<Abstraction*>(right);
    int varlid = l->query_abstraction_variable()->query_index();
    PVariable varr = r->query_abstraction_variable();
    PTerm subl = l->get_child(0);
    PTerm subr = r->get_child(0);

    Substitution delta;
    delta[varr->query_index()] =
      new Variable(varr->query_type()->copy(), varlid);
    subr = subr->apply_substitution(delta);

    add_formula(new Or(
        new AntiVar(index),
        new Var(var_for_constraint(subl, subr, ">="))
      ));

    delta.remove(varr);
    delta[varlid] = varr->copy();
    subr = subr->apply_substitution(delta);
    r->replace_child(0, subr);
    return;
  }

  if (left->query_abstraction()) {
    add_formula(new Or(
        new AntiVar(index),
        new Var(var_for_constraint(left, right, ">=eta"))
      ));
    return;
  }

  // now left must have the form f l1 ... ln
  vector<PTerm> lparts = left->split();
  if (!lparts[0]->query_constant()) {
    add_formula(new AntiVar(index));
    return;
  }
  PConstant f = dynamic_cast<PConstant>(lparts[0]);
  string fname = f->query_name();

  // if f is marked, we decide what rule to use
  if (fname[fname.length()-1] == '*') {
    add_formula(bigOr(
        new AntiVar(index),
        new Var(var_for_constraint(left, right, ">=select", resterm, resnum)),
        new Var(var_for_constraint(left, right, ">=fabs")),
        new Var(var_for_constraint(left, right, ">=copy")),
        new Var(var_for_constraint(left, right, ">=stat"))
      ));
  }

  // if f is not marked, it might be filtered away; otherwise, we
  // need the Fun clause, or we can mark f
  else {
    // since the right-hand side is "standard", so not mapped to
    // bottom, f cannot be mapped to bottom
    add_formula(new Or(
        new AntiVar(index),
        new AntiVar(master->minimal(fname))
      ));

    // if pi(f l1 ... ln) = li, then li >= r (standard right) as
    // r is not affected by any filtering of f!
    for (int i = 1; i < lparts.size(); i++) {
      int newconstraint;
      if (resterm == NULL) {
        newconstraint = var_for_constraint(lparts[i], right, ">=stdr");
      }
      else {
        newconstraint = var_for_constraint(lparts[i], right, ">=RST",
          resterm, resnum);
      }

      add_formula(bigOr(
          new AntiVar(index),
          new AntiVar(master->symbol_filtered(fname)),
          new Var(master->arg_filtered(fname, i)),
          new Var(newconstraint)
        ));
    }

    // if f is not filtered away, then we use either Fun, or mark it
    f->rename(fname + "*");
    int markedconstraint;
    if (resterm == NULL) {
      markedconstraint = var_for_constraint(left, right, ">=stdr");
    }
    else {
      markedconstraint = var_for_constraint(left, right, ">=RST",
                                            resterm, resnum);
    }
    f->rename(fname);
    add_formula(bigOr(
        new AntiVar(index),
        new Var(master->symbol_filtered(fname)),
        new Var(var_for_constraint(left, right, ">=fun")),
        new Var(markedconstraint)
      ));
  }
}

void HorpoConstraintList :: handle_fun(PTerm left,
                                       PTerm right,
                                       int index) {
  vector<PTerm> lsplit = left->split();
  vector<PTerm> rsplit = right->split();
  int i,j,k;
  
  if (!lsplit[0]->query_constant() ||
      !rsplit[0]->query_constant()) {
    add_formula(new AntiVar(index));
    return;
  }
  PConstant f = dynamic_cast<PConstant>(lsplit[0]);
  PConstant g = dynamic_cast<PConstant>(rsplit[0]);
  string fname = f->query_name();
  string gname = g->query_name();

  // we must have that f = g
  add_formula(new Or(
      new AntiVar(index),
      new Var(master->precequal(f,g))
    ));

  // they must have the same number of arguments;
  for (i = 1; i < lsplit.size() && i < rsplit.size(); i++) {
    add_formula(new Or(
        new AntiVar(index),
        new AntiVar(master->arg_length_min(fname, i)),
        new Var(master->arg_length_min(gname, i))
      ));
    add_formula(new Or(
        new AntiVar(index),
        new AntiVar(master->arg_length_min(gname, i)),
        new Var(master->arg_length_min(fname, i))
      ));
  }
  if (lsplit.size() > rsplit.size()) {
    add_formula(new Or(
        new AntiVar(index),
        new AntiVar(master->arg_length_min(fname, rsplit.size()))
      ));
  }
  if (rsplit.size() > lsplit.size()) {
    add_formula(new Or(
        new AntiVar(index),
        new AntiVar(master->arg_length_min(gname, lsplit.size()))
      ));
  }

  // in the case of lex, we merely need that when li and rj are
  // mapped to the same position, then li >= rj
  for (i = 1; i < lsplit.size(); i++) {
    for (k = 1; k < rsplit.size(); k++) {
      for (j = 1; j < lsplit.size() && j < rsplit.size(); j++) {
        add_formula(bigOr(
            new AntiVar(index),
            new AntiVar(master->lex(fname)),
            new AntiVar(master->permutation(fname, i, j)),
            new AntiVar(master->permutation(gname, k, j)),
            new Var(var_for_constraint(lsplit[i], rsplit[k], ">="))
          ));
      }
    }
  }

  // in the case of mul, we have to deal with an unknown permutation
  int A = vars.query_size();
  int n = lsplit.size()-1;
  int m = rsplit.size()-1;
  vars.add_vars(n*m);
    // A + (i-1) * m + j-1: r[j] must be compared with l[i]
  
  // domain and range of A cover only unfiltered arguments
  for (i = 1; i <= n; i++) {
    for (j = 1; j <= m; j++) {
      int Aij = A + (i-1) * m + j-1;
      add_formula(new Or(
          new AntiVar(master->arg_filtered(fname, i)),
          new AntiVar(Aij)
        ));
      add_formula(new Or(
          new AntiVar(master->arg_filtered(gname, j)),
          new AntiVar(Aij)
        ));
    }
  }
  
  // domain and range of A cover all unfiltered arguments, unless
  // the status is not Mul (in this case, taking all Aij false
  // suffices
  for (i = 1; i <= n; i++) {
    Or *full_domain =
      new Or(new AntiVar(index), new Var(master->lex(fname)));
    full_domain->add_child(new Var(master->arg_filtered(fname, i)));
    for (j = 1; j <= m; j++) {
      int Aij = A + (i-1) * m + j-1;
      full_domain->add_child(new Var(Aij));
    }
    add_formula(full_domain);
  }
  for (j = 1; j <= m; j++) {
    Or *full_range =
      new Or(new AntiVar(index), new Var(master->lex(fname)));
    full_range->add_child(new Var(master->arg_filtered(gname, j)));
    for (i = 1; i <= n; i++) {
      int Aij = A + (i-1) * m + j-1;
      full_range->add_child(new Var(Aij));
    }
    add_formula(full_range);
  }

  // no two points are mapped to the same value; this also implies
  // that a point is only mapped to one value, because the two terms
  // have the same number of arguments (as already required)
  for (i = 1; i < n; i++) {
    for (k = i+1; k <= n; k++) {
      for (j = 1; j <= m; j++) {
        int Aij = A + (i-1) * m + j-1;
        int Akj = A + (k-1) * m + j-1;
        add_formula(new Or(
            new AntiVar(Aij),
            new AntiVar(Akj)
          ));
      }
    }
  }

  // if status(f) = Mul, then l_{A(j)} >= r_j whenever j is not
  // filtered away!
  for (i = 1; i <= n; i++) {
    for (j = 1; j <= m; j++) {
      int Aij = A + (i-1) * m + j-1;
      add_formula(bigOr(
          new AntiVar(index),
          new Var(master->lex(fname)),
          new AntiVar(Aij),
          new Var(var_for_constraint(lsplit[i], rsplit[j], ">="))
        ));
    }
  }
}

void HorpoConstraintList :: handle_eta(PTerm left,
                                       PTerm right,
                                       int index,
                                       map<string,int> &arities) {
  int i;

  // get sides, split in parts
  Abstraction *l = dynamic_cast<Abstraction*>(left);
  PVariable x = l->query_abstraction_variable();
  PTerm lsub = l->get_child(0);
  vector<PTerm> subparts = lsub->split();
  if (!subparts[0]->query_constant()) {
    add_formula(new AntiVar(index));
    return;
  }
  PConstant f = dynamic_cast<PConstant>(subparts[0]);
  string fname = f->query_name();
  if (fname[fname.length()-1] == '*')
    fname = fname.substr(0, fname.length()-1);

  // now: /\x.f s1 ... sn >=eta t

  // if f is filtered away, then this really just says /\x.si >= t
  int f_filtered = master->symbol_filtered(fname);
  for (i = 1; i <= arities[fname] && i < subparts.size(); i++) {
    int filtered = master->arg_filtered(fname, i);
    PTerm si = subparts[i];
    l->replace_child(0, si);
    int newc = var_for_constraint(l, right, ">=eta");
    l->replace_child(0, lsub);
    Or *newform = new Or(
        new AntiVar(index),
        new AntiVar(f_filtered),
        new Var(filtered)
      );
    newform->add_child(new Var(newc));
    add_formula(newform);
  }

  // if f is not filtered away, then one of the unfiltered arguments
  // must be x itself
  vector<int> xposses;
  for (i = 1; i < subparts.size(); i++)
    if (subparts[i]->equals(x)) xposses.push_back(i);
  if (xposses.size() == 0) {
    add_formula(new Or(new AntiVar(index), new Var(f_filtered)));
    return;
  }
  if (xposses[xposses.size()-1] < arities[fname]) {
    Or *atleastone = new Or(new AntiVar(index), new Var(f_filtered));
    for (i = 0; i < xposses.size(); i++)
      atleastone->add_child(new AntiVar(master->arg_filtered(fname, i)));
    add_formula(atleastone);
  }

  // moreover, one of the unfiltered other arguments must have the
  // right type, and be above t
  Or *atleastone = new Or(new AntiVar(index), new Var(f_filtered));
  for (i = 1; i < subparts.size(); i++) {
    if (subparts[i]->equals(x)) continue;
    PType t1 = subparts[i]->query_type();
    PType t2 = left->query_type();
    vector<Or*> equal_types = master->force_type_equality(t1, t2);
    if (equal_types.size() == 1 &&
        equal_types[0]->query_number_children() == 0) {
      delete equal_types[0];
      continue;
    }
    int newindex = vars.query_size();
    vars.add_vars(1);
    atleastone->add_child(new Var(newindex));
    for (int j = 0; j < equal_types.size(); j++) {
      equal_types[j]->add_child(new AntiVar(newindex));
      add_formula(equal_types[j]);
    }
    if (i < arities[fname]+1) {
      add_formula(new Or(new AntiVar(newindex),
                         new AntiVar(master->arg_filtered(fname, i))));
    }
    add_formula(new Or(
        new AntiVar(newindex),
        new Var(var_for_constraint(subparts[i], right, ">=stdr"))
      ));
  }
  add_formula(atleastone);
}

void HorpoConstraintList :: handle_lex(PTerm left, PTerm right,
                                       vector<PTerm> &lsplit,
                                       vector<PTerm> &rsplit,
                                       int index,
                                       string fname, string gname,
                                       int n, int k, int m) {
  // in this function, left = f* l1 ... lk ... ln and
  // right = g r1 ... rm; this is just a helping function for
  // handle_stat.  The handle_stat function deals with the parts of
  // the stat steps which are shared with mul.

  int i;

  // for i in { 1 .. min(k,m+1)+1 } introduce a variable Pi which
  // indicates: the point p such that l_p > r_p for the first time
  // is at least i (it might also be that p <= len(l) and p > len(r)
  int maxp = m+1;
  if (k < maxp) maxp = k;
  int Pstart = vars.query_size()-1;
  vars.add_vars(maxp+1);

  add_formula(new Or(     // p >= 1
      new AntiVar(index),
      new AntiVar(master->lex(fname)),
      new Var(Pstart+1)
    ));
  add_formula(new AntiVar(Pstart+maxp+1));  // not p > k or p > m+1
  for (i = 1; i <= maxp; i++) {   // if p >= i, then f has at least i arguments
    add_formula(new Or(
        new AntiVar(Pstart+i),
        new Var(master->arg_length_min(fname, i))
      ));
  }
  for (i = 1; i < maxp; i++) {   // if p >= i, then g has at least i-1 args
    add_formula(new Or(
        new AntiVar(Pstart+i+1),
        new Var(master->arg_length_min(gname, i))
      ));
  }
  for (i = 1; i <= k && i <= m; i++) {
    // if p >= i+1, then p >= i
    add_formula(new Or(
          new Var(Pstart+i),
          new AntiVar(Pstart+i+1)
      ));
  }

  // express that li >= rj or li > rj when necessary
  for (int a = 1; a <= k; a++) {
    for (int b = 1; b <= m; b++) {
      for (i = 1; i <= k && i <= m; i++) {
        add_formula(bigOr(  // if p > i, then li >= ri
            new AntiVar(Pstart+i+1),
            new AntiVar(master->permutation(fname, a, i)),
            new AntiVar(master->permutation(gname, b, i)),
            new Var(var_for_constraint(lsplit[a], rsplit[b], ">="))
          ));
        add_formula(bigOr(  // if p = i, then li > ri, if ri exists
            new AntiVar(Pstart+i),
            new Var(Pstart+i+1),
            new AntiVar(master->permutation(fname, a, i)),
            new AntiVar(master->permutation(gname, b, i)),
            new Var(var_for_constraint(lsplit[a], rsplit[b], ">"))
          ));
      }
    }
  }

  // we must have that f* l1 ... ln >= each ri
  for (i = 1; i < rsplit.size(); i++) {
    PTerm lretyped = retype(left, rsplit[i]->query_type()->copy());
    add_formula(bigOr(
        new AntiVar(index),
        new AntiVar(master->lex(fname)),
        new Var(master->arg_filtered(gname, i)),
        new Var(var_for_constraint(lretyped, rsplit[i], ">="))
      ));
    destroy_retyped(lretyped);
  }
}

void HorpoConstraintList :: handle_mul(PTerm left, PTerm right,
                                       vector<PTerm> &lsplit,
                                       vector<PTerm> &rsplit,
                                       int index,
                                       string fname, string gname,
                                       int n, int k, int m) {
  // in this function, left = f* l1 ... lk ... ln and
  // right = g r1 ... rm; this is just a helping function for
  // handle_stat.  The handle_stat function deals with the parts of
  // the stat steps which are shared with lex.
  int i,j;

  int C = vars.query_size()-1;
  vars.add_vars(k);
    // Ci = C+i for 1 <= i <= k
    // this variable indicates that the elements of C are made
    // strictly smaller
  int Z = vars.query_size()-m-1;
  vars.add_vars(m*k);
    // Zij = Z + m*i + j for 1 <= i <= k and 1 <= j <= m
    // this variable indicates that l[i] will be compared to r[j]:
    // so l[i] >= r[j], or even l[i] > r[j] if Ci holds

  for (i = 1; i <= k; i++) {
    // C is a subset of the non-filtered arguments of left
    add_formula(new Or(
        new AntiVar(C+i),
        new AntiVar(master->arg_filtered(fname, i))
      ));
    // the function Z has only non-filtered arguments in its domain
    // and range
    for (j = 1; j <= m; j++) {
      add_formula(new Or(
          new AntiVar(Z+i*m+j),
          new AntiVar(master->arg_filtered(fname, i))
        ));
      add_formula(new Or(
          new AntiVar(Z+i*m+j),
          new AntiVar(master->arg_filtered(gname, j))
        ));
    }
  }

  // if we really do have multiset status (if not, we'll just choose
  // all new variables false), then C should be non-empty
  Or *atleastoneC = new Or(new AntiVar(index), new Var(master->lex(fname)));
  for (i = 1; i <= k; i++) atleastoneC->add_child(new Var(C+i));
  add_formula(atleastoneC);

  // the domain of Z should contain all non-filtered arguments of right
  for (j = 1; j <= m; j++) {
    Or *atleastoneZ = new Or(new AntiVar(index),
                             new Var(master->lex(fname)),
                             new Var(master->arg_filtered(gname, j)));
    for (i = 1; i <= k; i++)
      atleastoneZ->add_child(new Var(Z+i*m+j));
    add_formula(atleastoneZ);
  }

  // Z is injective, except perhaps on elements of C
  for (i = 1; i <= k; i++) {
    for (j = 1; j < m; j++) {
      for (int j2 = j+1; j2 <= m; j2++) {
        add_formula(new Or(
            new AntiVar(Z+i*m+j),
            new AntiVar(Z+i*m+j2),
            new Var(C+i)
          ));
      }
    }
  }

  // if Z(j) = i, then l[i] >= r[j], or even l[i] > r[j] if i in C
  for (i = 1; i <= k; i++) {
    for (j = 1; j <= m; j++) {
      add_formula(new Or(
          new AntiVar(Z+i*m+j),
          new AntiVar(C+i),
          new Var(var_for_constraint(lsplit[i], rsplit[j], ">"))
        ));
      add_formula(new Or(
          new AntiVar(Z+i*m+j),
          new Var(C+i),
          new Var(var_for_constraint(lsplit[i], rsplit[j], ">="))
        ));
    }
  }
}

void HorpoConstraintList :: handle_stat(PTerm left,
                                        PTerm right,
                                        int index,
                                        map<string,int> &arities) {
  vector<PTerm> lsplit = left->split();
  vector<PTerm> rsplit = right->split();
  int i,a,b;
  
  if (!lsplit[0]->query_constant() ||
      !rsplit[0]->query_constant()) {
    add_formula(new AntiVar(index));
    return;
  }
  PConstant f = dynamic_cast<PConstant>(lsplit[0]);
  PConstant g = dynamic_cast<PConstant>(rsplit[0]);
  string gname = g->query_name();
  string fname = f->query_name();
  fname = fname.substr(0,fname.length()-1);
    // f is marked; remove the *

  int n = lsplit.size()-1;
  int k = arities[fname];
  int m = rsplit.size()-1;

  // we must have that f = g
  f->rename(fname);
  add_formula(new Or(
      new AntiVar(index),
      new Var(master->precequal(f,g))
    ));
  f->rename(fname + "*");

  // thus far the shared functionality; lex and mul are both treated
  // differently from now on
  handle_lex(left, right, lsplit, rsplit, index, fname, gname, n, k, m);
  handle_mul(left, right, lsplit, rsplit, index, fname, gname, n, k, m);
}

void HorpoConstraintList :: handle_fabs(PTerm left,
                                        PTerm right,
                                        int index) {
  // we are guaranteed that left = f* l1 ... lk ... ln, and
  // additionally need that right = /\x:type.r, and
  // f* l1 ... ln x >= r
  if (!right->query_abstraction()) {
    add_formula(new AntiVar(index));
    return;
  }

  // shouldn't happen, but check just in case
  if (!right->query_type()->query_composed()) {
    cout << "ERROR: abstraction with a non-functional type!" << endl;
    add_formula(new AntiVar(index));
    return;
  }

  // due to type changing functions, we might get into this situation
  // when the types of left and right are not actually comparable;
  // however, there are constraints saying the types are mapped to the
  // same thing, so we can safely retype!
  if (!left->query_type()->query_composed()) {
    PTerm retyped = retype(left, right->query_type()->copy());
    handle_fabs(retyped, right, index);
    destroy_retyped(retyped);
    return;
  }

  // just add a variable, and a new constraint!
  Abstraction *rr = dynamic_cast<Abstraction*>(right);
  PVariable x = rr->query_abstraction_variable();
  PTerm r = right->get_child(0);
  PVariable X = new Variable(x->query_type()->copy(), x->query_index());
  PTerm new_l = new Application(left, X);
  add_formula(new Or(
      new AntiVar(index),
      new Var(var_for_constraint(new_l, r, ">="))
    ));
  new_l->replace_child(0, NULL);
  delete new_l;
}

void HorpoConstraintList :: handle_copy(PTerm left,
                                        PTerm right,
                                        int index) {
  vector<PTerm> lsplit = left->split();
  vector<PTerm> rsplit = right->split();
  int i,a,b;
  
  if (!lsplit[0]->query_constant() ||
      !rsplit[0]->query_constant()) {
    add_formula(new AntiVar(index));
    return;
  }
  PConstant f = dynamic_cast<PConstant>(lsplit[0]);
  PConstant g = dynamic_cast<PConstant>(rsplit[0]);
  string gname = g->query_name();
  string fname = f->query_name();
  fname = fname.substr(0,fname.length()-1);
    // f is marked; remove the *

  // f > g
  f->rename(fname);
  add_formula(new Or(
      new AntiVar(index),
      new Var(master->precstrict(f, g))
    ));
  f->rename(fname + "*");

  // otherwise we only need that f* l1 ... ln >= each ri
  for (i = 1; i < rsplit.size(); i++) {
    PTerm lretyped = retype(left, rsplit[i]->query_type()->copy());
    add_formula(new Or(
        new AntiVar(index),
        new Var(master->arg_filtered(gname, i)),
        new Var(var_for_constraint(lretyped, rsplit[i], ">="))
      ));
    destroy_retyped(lretyped);
  }
}

void HorpoConstraintList :: handle_select(PTerm left,
                                          PTerm right,
                                          int index,
                                          PTerm resterm,
                                          int resnum) {
  int i;
  if (resterm == NULL) {
    resterm = left;
    resnum = 4;
  }

  vector<PTerm> lsplit = left->split();
  if (!lsplit[0]->query_constant() || lsplit.size() == 1) {
    add_formula(new AntiVar(index));
    return;
  }
  string headname = dynamic_cast<PConstant>(lsplit[0])->query_name();
  headname = headname.substr(0,headname.length()-1);

  // add variables for: "selected child is at position i"
  int K = vars.query_size()-1;
  vars.add_vars(lsplit.size()-1);

  // if we select, we're selecting at some position
  Or *formula = new Or(new AntiVar(index));
  for (i = 1; i < lsplit.size(); i++) {
    formula->add_child(new Var(K+i));
  }
  add_formula(formula);

  // what does it mean to select at some position? Well, first of
  // all, that position isn't filtered away!
  for (i = 1; i < lsplit.size(); i++) {
    add_formula(new Or(
        new AntiVar(K+i),
        new AntiVar(master->arg_filtered(headname, i))
      ));
  }

  // second, si<...> >= r for some output type
  for (i = 1; i < lsplit.size(); i++) {
    formula = new Or(new AntiVar(K+i));
    if (resterm == left) {
      formula->add_child(new Var(
        var_for_constraint(lsplit[i], right, ">=stdr")));
    }
    else {
      formula->add_child(new Var(
        var_for_constraint(lsplit[i], right, ">=RST",
                           resterm, resnum)));
    }

    // for functional types, also add more advanced possibilities!
    PType type = lsplit[i]->query_type();
    if (!type->query_composed()) {
      add_formula(formula);
      continue;
    }
      // TODO: if we use type changing functions, this constraint
      // should be changed!
    PTerm term = lsplit[i];
    PTerm head = term->query_head();
    if (!term->query_abstraction() && !head->query_constant()) {
      add_formula(formula);
      continue;
    }
    term = term->copy();

    int k = 0;
    while (type->query_composed()) {
      if (term->query_abstraction()) {
        Abstraction *abs = dynamic_cast<Abstraction*>(term);
        PVariable x = abs->query_abstraction_variable();
        PTerm child = abs->replace_child(0, NULL);
        Substitution subst;
        PTerm retyped = retype(left, type->query_child(0));
        subst[x] = retyped->copy();
        term = child->apply_substitution(subst);
        destroy_retyped(retyped);
        delete abs;
        head = term->query_head();
      }
      else {
        if (!head->query_constant()) break;
        PConstant f = dynamic_cast<PConstant>(head);
        string fname = f->query_name();
        if (fname != "" && fname[fname.length()-1] == '*')
          f->rename(fname.substr(0,fname.length()-1));

        PTerm retyped = retype(left, type->query_child(0));
        term = new Application(term, retyped);
        k++;
      }
      formula->add_child(new Var(var_for_constraint(term, right,
        ">=RST", resterm, resnum-1)));
      type = term->query_type();
    }

    for (int j = 0; j < k; j++) {
      destroy_retyped(term->replace_child(1, NULL));
      PTerm leftside = term->replace_child(0, NULL);
      delete term;
      term = leftside;
    }
    delete term;

    add_formula(formula);
  }

}

int HorpoConstraintList :: measure(PTerm term) {
  if (term->query_variable()) return 1;
  if (term->query_constant()) {
    string name = dynamic_cast<PConstant>(term)->query_name();
    if (name != "" && name[name.length()-1] == '*') return 2;
    else return 1;
  }

  int ret = 0;
  for (int i = 0; i < term->number_children(); i++)
    ret += measure(term->get_child(i));
  if (term->query_abstraction() || term->query_meta()) return ret+1;
  else return ret;
}

void HorpoConstraintList :: handle_restricted(PTerm left,
                                              PTerm right,
                                              int index,
                                              PTerm resterm,
                                              int resnum) {
  // we restrict only by measure - if the measure has dropped far
  // enough, we don't need to use the restriction!
  if (measure(left) < measure(resterm)) {
    add_formula(new Or(
        new AntiVar(index),
        new Var(var_for_constraint(left, right, ">=stdr"))
      ));
    return;
  }

  // but we do have a restriction - if we've done too many increasing
  // steps, cut off this evaluation path
  if (resnum <= 0 || (constraints.size() > 1000 && resnum == 1)) {
    add_formula(new AntiVar(index));
    return;
  }

  return handle_standard_right(left, right, index, resterm, resnum);
}

void HorpoConstraintList :: simplify(map<string,int> &arities) {
  // any constraint with index < handled is DONE
  // any constraint with index >= handled is TODO
  for (; handled < constraints.size(); handled++) {
    PTerm left = constraints[handled]->left;
    PTerm right = constraints[handled]->right;
    string relation = constraints[handled]->relation;
    int index = constraints[handled]->variable_index;

    constraints[handled]->justification_start = formulas.size();

    // only terms of equal type (modulo collapsing of base types)
    // can be compared (in the future, we will here add type
    // changing functions
    vector<Or*> type_equality = master->force_type_equality(
                    left->query_type(), right->query_type());
    bool pointless = false;
    for (int i = 0; i < type_equality.size(); i++) {
      if (type_equality[i]->query_number_children() == 0) pointless = true;
      type_equality[i]->add_child(new AntiVar(index));
      add_formula(type_equality[i]);
    }
    if (pointless) continue;

    // consider the various forms a constraint might have
    if (relation == ">")
      handle_greater(left, right, index);
    else if (relation == ">=")
      handle_basic_geq(left, right, index);
    else if (relation == ">=stdr")
      handle_standard_right(left, right, index, NULL, 0);
    else if (relation == ">=fun")
      handle_fun(left, right, index);
    else if (relation == ">=eta")
      handle_eta(left, right, index, arities);
    else if (relation == ">=stat")
      handle_stat(left, right, index, arities);
    else if (relation == ">=fabs")
      handle_fabs(left, right, index);
    else if (relation == ">=copy")
      handle_copy(left, right, index);
    else if (relation == ">=select")
      handle_select(left, right, index,
                    constraints[handled]->restriction_term,
                    constraints[handled]->restriction_num);
    else if (relation == ">=RST")
      handle_restricted(left, right, index,
                        constraints[handled]->restriction_term,
                        constraints[handled]->restriction_num);
  }
}

vector<PFormula> HorpoConstraintList :: justify_constraint(int index) {
  vector<PFormula> ret;
  for (int i = 0; i < constraints.size(); i++) {
    if (constraints[i]->variable_index == index) {
      int start = constraints[i]->justification_start;
      int end = formulas.size();
      if (i+1 < constraints.size())
        end = constraints[i+1]->justification_start;
      for (int j = start; j < end; j++) {
        ret.push_back(formulas[j]);
      }
      break;
    }
  }
  return ret;
}

vector<int> HorpoConstraintList :: constraint_variables() {
  vector<int> ret;
  for (int i = 0; i < constraints.size(); i++) {
    ret.push_back(constraints[i]->variable_index);
  }
  return ret;
}

