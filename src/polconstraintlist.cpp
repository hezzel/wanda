/**************************************************************************
   Copyright 2013 Cynthia Kop

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

#include "outputmodule.h"
#include "polconstraintlist.h"
#include "polymodule.h"
#include <cstdio>
#include <iostream>

PolConstraintList :: PolConstraintList(PolyModule *_master) :master(_master) {}

PolConstraintList :: ~PolConstraintList() {
  reset();
}

string PolConstraintList :: print_poly_constraint(PPol left, PPol right) {
  map<int,int> freerename, boundrename;
  string ret = left->to_string(freerename, boundrename) + " >= ";
  return ret + right->to_string(freerename, boundrename);
}

string PolConstraintList :: print_poly_constraint(int index) {
  return print_poly_constraint(constraints[index]->left,
                               constraints[index]->right);
}

void PolConstraintList :: reset() {
  int i;
  for (i = 0; i < formulas.size(); i++) delete formulas[i];
  for (i = 0; i < constraints.size(); i++) delete constraints[i];
  formulas.clear();
  constraints.clear();
}

void PolConstraintList :: add_formula(PFormula formula) {
  formulas.push_back(formula);
}

int PolConstraintList :: add_geq(PPol left, PPol right) {
  int num = vars.query_size();
  PolyConstraint *pc = new PolyConstraint(left, right, num);
  string printed = print_poly_constraint(left, right);
  vars.add_vars(1);
  char strid[10];
  sprintf(strid, "%d", constraints.size());
  vars.set_description(num, "polconstraint " + string(strid));
  //vars.set_description(num, printed);
  constraint_lookup[printed] = constraints.size();
  constraints.push_back(pc);
  return num;
}

int PolConstraintList :: index_of_constraint(PPol left, PPol right) {
  string printed = print_poly_constraint(left, right);
  if (constraint_lookup.find(printed) == constraint_lookup.end()) {
    add_geq(left, right);
    return constraints.size()-1;
  }
  delete left;
  delete right;
  return constraint_lookup[printed];
}

int PolConstraintList :: var_for_constraint(PPol left, PPol right) {
  int constraint_index = index_of_constraint(left, right);
  return constraints[constraint_index]->variable_index;
}

void PolConstraintList :: simplify() {
  for (int handled = 0; handled < (int)constraints.size(); handled++) {
    PPol left = constraints[handled]->left;
    PPol right = constraints[handled]->right;
    int vid = constraints[handled]->variable_index;

    if (trivial_checks(left, right, vid) ||
        remove_duplicates(left, right, vid) ||
        split_max(left, right, vid) ||
        split_parts(left, right, vid)) {
      constraints[handled]->active = false;
    }
  }
}

bool PolConstraintList :: trivial_checks(PPol left, PPol right, int vindex) {
  // l >= 0 always holds
  if (right->query_integer() &&
      dynamic_cast<Integer*>(right)->query_value() == 0) {
    add_formula(new Var(vindex));
    return true;
  }

  // if both sides are integers, we know how they compare!
  if (left->query_integer() && right->query_integer()) {
    int a = dynamic_cast<Integer*>(left)->query_value();
    int b = dynamic_cast<Integer*>(right)->query_value();
    if (a >= b) add_formula(new Var(vindex));
    else add_formula(new AntiVar(vindex));
    return true;
  }

  // we can do a lot if left is 0
  if (left->query_integer() &&
      dynamic_cast<Integer*>(left)->query_value() == 0) {
    
    // 0 >= a + b iff 0 >= a and 0 >= b
    if (right->query_sum()) {
      Sum *r = dynamic_cast<Sum*>(right);
      for (int i = 0; i < r->number_children(); i++) {
        int nc = var_for_constraint(new Integer(0), r->get_child(i)->copy());
        add_formula(new Or(new AntiVar(vindex), new Var(nc)));
      }
      return true;
    }

    // 0 >= a * b iff 0 >= a or 0 >= b
    if (right->query_product()) {
      Product *r = dynamic_cast<Product*>(right);
      Or *newform = new Or(new AntiVar(vindex));
      for (int i = 0; i < r->number_children(); i++) {
        int nc = var_for_constraint(new Integer(0), r->get_child(i)->copy());
        newform->add_child(new Var(nc));
      }
      add_formula(newform);
      return true;
    }

    // we do not in general have 0 >= x or F(x)
    if (right->query_variable() || right->query_functional()) {
      add_formula(new AntiVar(vindex));
      return true;
    }
  }

  // also if right is 1
  if (right->query_integer() &&
      dynamic_cast<Integer*>(right)->query_value() == 1) {
    
    // a + b >= 1 iff a >= 1 or b >= 1
    if (left->query_sum()) {
      Sum *l = dynamic_cast<Sum*>(left);
      Or *newform = new Or(new AntiVar(vindex));
      for (int i = 0; i < l->number_children(); i++) {
        int nc = var_for_constraint(l->get_child(i)->copy(), new Integer(1));
        newform->add_child(new Var(nc));
      }
      add_formula(newform);
      return true;
    }

    // a * b >= 1 iff a >= 1 and b >= 1
    if (left->query_product()) {
      Product *l = dynamic_cast<Product*>(left);
      for (int i = 0; i < l->number_children(); i++) {
        int nc = var_for_constraint(l->get_child(i)->copy(), new Integer(1));
        add_formula(new Or(new AntiVar(vindex), new Var(nc)));
      }
      return true;
    }

    // x >= 1 and F(x) >= 1 do not in general hold
    if (left->query_variable() || left->query_functional()) {
      add_formula(new AntiVar(vindex));
      return true;
    }
  }

  return false;
}

bool PolConstraintList :: remove_duplicates(PPol left, PPol right, int vindex) {
  int a, b;

  // two non-sums
  if (!left->query_sum() && !right->query_sum()) {
    if (!left->query_similar(right, a, b)) return false;
    if (a >= b) {
      add_formula(new Var(vindex));
    }
    else {
      PPol nul = new Integer(0);
      PPol newright = right->copy();
      set_coefficient(newright, b-a);
      int newvar = var_for_constraint(nul, newright->simplify());
      add_formula(new Or(new AntiVar(vindex), new Var(newvar)));
    }
    return true;
  }

  // only the left is a sum
  if (left->query_sum() && !right->query_sum()) {
    Sum *l = dynamic_cast<Sum*>(left);
    for (int i = 0; i < l->number_children(); i++) {
      if (l->get_child(i)->query_similar(right, a, b)) {
        if (a >= b) {
          add_formula(new Var(vindex));
          return true;
        }
        else {
          PPol l = left->copy();
          PPol r = right->copy();
          set_coefficient(l, 0, i);
          set_coefficient(r, b-a);
          l = l->simplify();
          r = r->simplify();
          add_formula(new Or(new AntiVar(vindex),
                             new Var(var_for_constraint(l,r))));
          return true;
        }
      }
    }
    return false;
  }

  // only the right is a sum
  if (right->query_sum() && !left->query_sum()) {
    if (left->query_integer() &&
        dynamic_cast<Integer*>(left)->query_value() == 0) return false;
    Sum *r = dynamic_cast<Sum*>(right);
    for (int i = 0; i < r->number_children(); i++) {
      if (left->query_similar(r->get_child(i), a, b)) {
        if (a > b) {
          PPol l = left->copy();
          PPol r = right->copy();
          set_coefficient(l, a-b);
          set_coefficient(r, 0, i);
          l = l->simplify();
          r = r->simplify();
          add_formula(new Or(new AntiVar(vindex),
                             new Var(var_for_constraint(l,r))));
          return true;
        }
        else {
          PPol l = new Integer(0);
          PPol r = right->copy();
          set_coefficient(r, b-a, i);
          add_formula(new Or(new AntiVar(vindex),
                      new Var(var_for_constraint(l, r->simplify()))));
          return true;
        }
      }
    }
    return false;
  }

  // both are sums!
  Sum *l = dynamic_cast<Sum*>(left);
  Sum *r = dynamic_cast<Sum*>(right);
  PPol lnew = NULL, rnew = NULL;
  for (int i = 0; i < l->number_children(); i++) {
    for (int j = 0; j < r->number_children(); j++) {
      if (l->get_child(i)->query_similar(r->get_child(j), a, b)) {
        if (lnew == NULL) {
          lnew = left->copy();
          rnew = right->copy();
        }
        int ind1 = (a >= b ? a-b : 0);
        int ind2 = (b >= a ? b-a : 0);
        set_coefficient(lnew, ind1, i);
        set_coefficient(rnew, ind2, j);
      }
    }
  }
  if (lnew != NULL) {
    lnew = lnew->simplify();
    rnew = rnew->simplify();
    add_formula(new Or(new AntiVar(vindex),
                       new Var(var_for_constraint(lnew, rnew))));
    return true;
  }
  return false;
}

void PolConstraintList :: set_coefficient(PPol &pol, int coef, int index) {
  // deal with sums
  if (index != -1 && pol->query_sum()) {
    Sum *p = dynamic_cast<Sum*>(pol);
    PPol c = p->get_child(index);
    set_coefficient(c, coef);
    p->replace_child(index, c);
    return;
  }

  // deal with products i*a
  if (pol->query_product()) {
    Product *cp = dynamic_cast<Product*>(pol);
    if (cp->number_children() >= 1 &&
        cp->get_child(0)->query_integer()) {
      Integer *ipol = dynamic_cast<Integer*>(cp->get_child(0));
      ipol->set_value(coef);
      return;
    }
  }

  // deal with the remainder
  if (coef == 0) { delete pol; pol = new Integer(0); }
  else if (coef != 1) pol = new Product(new Integer(coef), pol);
}

bool PolConstraintList :: split_max(PPol left, PPol right,
                                    int vindex) {
  PPol greater;
  vector<PPol> smaller;

  bool leftcontains = contains_max(left);
  bool rightcontains = contains_max(right);

  if (!leftcontains && !rightcontains) return false;

  PPol newleft = left->copy();
  PPol newright = right->copy();

  // first case: left contains a max - we just split it into an or
  // (this is not optimal, but it's a rare case; only really happens
  // for argument functions)
  if (leftcontains) {
    PPol left_split = split_max(newleft, greater, smaller);
    int aindex = var_for_constraint(newleft->simplify(), newright->copy());
    int bindex = var_for_constraint(left_split->simplify(), newright);
    add_formula(new Or(new AntiVar(vindex), new Var(aindex), new Var(bindex)));
    return true;
  }

  // second (more important) case: right contains a max
  if (rightcontains) {
    PPol right_split = split_max(newright, greater, smaller);
    if (right_split == NULL) return false;
    account_for_greater(greater, smaller, right_split);
    account_for_smaller(greater, smaller, newright);
    newright = newright->simplify();
    right_split = right_split->simplify();
    int cindex = var_for_constraint(newleft->copy(), right_split);
    add_formula(new Or(new AntiVar(vindex), new Var(cindex)));
  }

  add_formula(new Or(new AntiVar(vindex),
                     new Var(var_for_constraint(newleft, newright))));
  return true;
}

bool PolConstraintList :: contains_max(PPol pol) {
  if (pol->query_max()) return true;
  if (pol->query_sum() || pol->query_product())
    for (int i = 0; i < pol->number_children(); i++)
      if (contains_max(pol->get_child(i))) return true;
  return false;
}

PPol PolConstraintList :: split_max(PPol pol, PPol &greater,
                                    vector<PPol> &smaller) {
  // TODO: optimise this by taking an innermost max, rather than outermost
  if (pol->query_max()) {
    Max *m = dynamic_cast<Max*>(pol);
    if (m->number_children() < 2) return NULL;
    greater = m->get_child(0);
    for (int i = 1; i < m->number_children(); i++)
      smaller.push_back(m->get_child(i));
    return m->replace_child(0, new Integer(0));
  }

  if (pol->query_product()) {
    Product *prod = dynamic_cast<Product*>(pol);
    for (int i = 0; i < prod->number_children(); i++) {
      PPol p = split_max(prod->get_child(i), greater, smaller);
      if (p == NULL) continue;
      // this child contained a max, and took p out of it;
      // multiply with the rest of this product and return
      for (int j = 0; j < prod->number_children(); j++) {
        if (i != j) p = new Product(prod->get_child(j)->copy(), p);
      }
      return p;
    }
  }

  if (pol->query_sum()) {
    Sum *sum = dynamic_cast<Sum*>(pol);
    for (int i = 0; i < sum->number_children(); i++) {
      PPol p = split_max(sum->get_child(i), greater, smaller);
      if (p == NULL) continue;
      // this child contained a max, and took p out of it;
      // add to the rest of this sum and return
      for (int j = 0; j < sum->number_children(); j++) {
        if (i != j) p = new Sum(sum->get_child(j)->copy(), p);
      }
      return p;
    }
  }
  // note: don't do this for maxs inside a functional term

  return NULL;
}

void PolConstraintList :: account_for_greater(PPol &greater, vector<PPol>
                                               &smaller, PPol &pol) {
  int i, j, k;
  for (i = 0; i < pol->number_children(); i++) {
    PPol c = pol->get_child(i);
    account_for_greater(greater, smaller, c);
  }

  if (!pol->query_max()) return;

  /* Note the following:
   * both [greater,smaller1,...,smallern], and the children of pol,
   * are ordered by rating, smaller ratings first.
   * So if greater < child i, it will also be < all next children.
   * Moreover, it child i = greater, and child j = smaller k, then
   * it must hold that i < j.
   */

  for (i = 0; i < pol->number_children(); i++) {
    int cmp = pol->get_child(i)->compare(greater);
    if (cmp == -1) return;
    if (cmp == 1) continue;
    // child i = greater! See whether any of smaller are represented!

    k = 0;
    for (j = i+1; j < pol->number_children(); j++) {
      PPol child = pol->get_child(j);
      for (; k < smaller.size(); k++) {
        cmp = child->compare(smaller[k]);
        if (cmp == -1) break;
        if (cmp == 1) continue;
        // child j = smaller[k]!
        delete dynamic_cast<Max*>(pol)->replace_child(j, new Integer(0));
        break;
      }
    }
  }
}

void PolConstraintList :: account_for_smaller(PPol &greater, vector<PPol>
                                              &smaller, PPol &pol) {
  int i, j, k;
  for (i = 0; i < pol->number_children(); i++) {
    PPol c = pol->get_child(i);
    account_for_smaller(greater, smaller, c);
  }

  if (!pol->query_max()) return;

  /* Note the following:
   * both [greater,smaller1,...,smallern], and the children of pol,
   * are ordered by rating, smaller ratings first.
   * So if greater < child i, it will also be < all next children.
   * Moreover, it child i = greater, and child j = smaller k, then
   * it must hold that i < j.
   */

  for (i = 0; i < pol->number_children(); i++) {
    int cmp = pol->get_child(i)->compare(greater);
    if (cmp == -1) return;
    if (cmp == 1) continue;
    // child i = greater! See whether all of smaller are represented!

    bool all_present = true;
    k = 0;
    for (j = i+1; j < pol->number_children(); j++) {
      PPol child = pol->get_child(j);
      bool ok = false;
      for (; k < smaller.size() && !ok; k++) {
        cmp = child->compare(smaller[k]);
        if (cmp == -1) break;
        if (cmp == 0) ok = true;
      }
      if (!ok) all_present = false;
    }

    if (!all_present) return;
    // they're all here! Remove greater.
    delete dynamic_cast<Max*>(pol)->replace_child(i, new Integer(0));
  }
}

// responsible for absolute positiveness
bool PolConstraintList :: split_parts(PPol left, PPol right, int vindex) {
  vector<int> combination = find_combination(left);
  if (combination.size() == 0) combination = find_combination(right);
  if (combination.size() == 0) return false;

  PPol newleft1 = left->copy();
  PPol newright1 = right->copy();

  while (combination.size() != 0) {
    // there is at least one part of left/right with variables in it;
    // split it off into newleft2 / newright2
    PPol newleft2 = remove_parts(newleft1, combination);
    PPol newright2 = remove_parts(newright1, combination);

    // newleft1 / newright1 contain the remainder, which might still
    // contain (different) variables; if they don't, we just require
    // that still newleft1 >= newright1
    newleft1 = newleft1->simplify();
    newright1 = newright1->simplify();

    remove_variables(newleft2, newright2, vindex);

    combination = find_combination(newleft1);
    if (combination.size() == 0) combination = find_combination(newright1);
  }

  add_formula(new Or(new AntiVar(vindex),
                     new Var(var_for_constraint(newleft1, newright1))));

  return true;
}

vector<int> PolConstraintList :: find_combination(PPol pol) {
  vector<int> ret;

  if (pol->query_integer() || pol->query_unknown()) return ret;
  
  if (pol->query_variable()) {
    ret.push_back(dynamic_cast<Polvar*>(pol)->query_index());
    return ret;
  }
  
  if (pol->query_functional()) {
    ret.push_back(dynamic_cast<Functional*>(pol)->function_index());
    return ret;
  }
  
  if (pol->query_sum()) {
    Sum* psum = dynamic_cast<Sum*>(pol);
    PPol last = psum->get_child(psum->number_children()-1);
    if (last == NULL) return ret;
    return find_combination(last);
  }
  
  if (!pol->query_product()) return ret;
  Product *p = dynamic_cast<Product*>(pol);
  
  for (int i = 0; i < p->number_children(); i++) {
    if (p->get_child(i)->query_variable()) {
      int j = dynamic_cast<Polvar*>(p->get_child(i))->query_index();
      ret.push_back(j);
    }
    else if (p->get_child(i)->query_functional()) {
      int j = dynamic_cast<Functional*>(p->get_child(i))->function_index();
      ret.push_back(j);
    }
  }
  return ret;
}

PPol PolConstraintList :: remove_parts(PPol &pol, vector<int> &combination) {
  if (!pol->query_sum()) {
    vector<int> comb = find_combination(pol);
    if (comb.size() != combination.size()) return new Integer(0);
    for (int i = 0; i < combination.size(); i++) {
      if (comb[i] != combination[i]) return new Integer(0);
    }
    PPol ret = pol;
    pol = new Integer(0);
    return ret;
  }

  Sum *sum = dynamic_cast<Sum*>(pol);
  Sum *ret = new Sum();
  for (int i = 0; i < sum->number_children(); i++) {
    vector<int> comb = find_combination(sum->get_child(i));
    bool ok = comb.size() == combination.size();
    for (int j = 0; j < combination.size() && ok; j++) {
      if (comb[j] != combination[j]) ok = false;
    }
    if (ok) {
      ret->add_child(sum->get_child(i));
      sum->replace_child(i, new Integer(0));
    }
  }
  return ret->simplify();
}

typedef vector<PolynomialFunction*> Polfunvec;
typedef vector<PPol> Polvec;

void PolConstraintList :: remove_variables(PPol left, PPol right, int vindex) {
  bool has_functional = false;
  vector<Polfunvec> funcpartsleft, funcpartsright;
  vector<Polvec> partsleft, partsright;
  int i, j, k;

  // make sure we're dealing with sums here!
  if (!left->query_sum()) {
    Sum *l = new Sum();
    if (left->query_integer() &&
        dynamic_cast<Integer*>(left)->query_value() == 0) delete left;
    else l->add_child(left);
    left = l;
  }
  if (!right->query_sum()) {
    Sum *r = new Sum();
    if (right->query_integer() &&
        dynamic_cast<Integer*>(right)->query_value() == 0) delete right;
    else r->add_child(right);
    right = r;
  }

  for (int side = 0; side < 2; side++) {
    // turn the side into a Sum*
    Sum *pol;
    if (side == 0) pol = dynamic_cast<Sum*>(left);
    else pol = dynamic_cast<Sum*>(right);

    // deal with each of the children
    for (i = 0; i < pol->number_children(); i++) {
      PPol c = pol->get_child(i);

      // deal with non-products
      if (c->query_variable()) { delete c; c = new Integer(1); }
      if (c->query_functional()) {
        has_functional = true;
        Polfunvec vec;
        Functional *d = dynamic_cast<Functional*>(c);
        for (j = 0; j < d->number_children(); j++)
          vec.push_back(d->get_function_child(j));
        if (side == 0) funcpartsleft.push_back(vec);
        else funcpartsright.push_back(vec);
        d->weak_delete();
        c = new Integer(1);
      }

      if (c->query_product()) {
        Product *prod = dynamic_cast<Product*>(c);
        Polfunvec vec;
        for (j = 0; j < prod->number_children(); j++) {
          if (prod->get_child(j)->query_variable())
            delete prod->replace_child(j, new Integer(1));
          if (prod->get_child(j)->query_functional()) {
            Functional *d =
              dynamic_cast<Functional*>(prod->get_child(j));
            for (k = 0; k < d->number_children(); k++)
              vec.push_back(d->get_function_child(k));
            prod->replace_child(j, new Integer(1));
            d->weak_delete();
          }
        }
        if (vec.size() != 0) {
          has_functional = true;
          if (side == 0) funcpartsleft.push_back(vec);
          else funcpartsright.push_back(vec);
        }

      }

      // make sure changes to c propagate back to pol
      pol->replace_child(i, c);
    }
  }

  if (!has_functional) {
    int cnum = var_for_constraint(left->simplify(), right->simplify());
    add_formula(new Or(new AntiVar(vindex), new Var(cnum)));
    return;
  }

  // functionals everywhere! => make some basic checks

  // check 1: if one of the sides is 0, we know the outcome regardless
  // of the values of functionals
  Sum *l = dynamic_cast<Sum*>(left);
  Sum *r = dynamic_cast<Sum*>(right);
  if (r->number_children() == 0 || l->number_children() == 0) {
    for (i = 0; i < funcpartsleft.size(); i++)
      for (j = 0; j < funcpartsleft[i].size(); j++)
        delete funcpartsleft[i][j];
    for (i = 0; i < funcpartsright.size(); i++)
      for (j = 0; j < funcpartsright[i].size(); j++)
        delete funcpartsright[i][j];

    if (r->number_children() != 0) {
      add_formula(new Or(new AntiVar(vindex),
           new Var(var_for_constraint(left->simplify(), right->simplify()))));
    }
    else {
      delete l;
      delete r;
    }
    return;
  }

  // they're actual sums; do some error checking
  int n = -1;
  if (funcpartsleft.size() != 0) n = funcpartsleft[0].size();
  if (funcpartsright.size() != 0) n = funcpartsright[0].size();
  if (n <= 0) {
    return;
  }
  bool ok = true;
  for (i = 0; i < funcpartsleft.size() && ok; i++)
    ok &= (funcpartsleft[i].size() == n);
  for (i = 0; i < funcpartsright.size() && ok; i++)
    ok &= (funcpartsright[i].size() == n);
  if (!ok) {
    cout << "ERROR!  Inconsistent functionals!\n" << endl;
    return;
  }

  /**
   * the parts on left- and right-hand side contain functionals, but
   * in each part, there are in total n arguments, and they have the
   * same functional form; turn them into polynomials (with the same
   * variables replacing the bound variables everywhere) and save this
   * into partsleft and partsright!
   */
  for (i = 0; i < funcpartsleft.size(); i++) {
    Polvec lpart;
    for (j = 0; j < n; j++) lpart.push_back(NULL);
    partsleft.push_back(lpart);
  }
  for (i = 0; i < funcpartsright.size(); i++) {
    Polvec rpart;
    for (j = 0; j < n; j++) rpart.push_back(NULL);
    partsright.push_back(rpart);
  }

  for (j = 0; j < n; j++) {
    // make a consistent set of variables (which we can reuse for all
    // function parts, both on the left- and right-hand side)
    Polfunvec vars;
    PolynomialFunction *leftfunc = funcpartsleft[0][j];
    for (k = 0; k < leftfunc->num_variables(); k++) {
      vars.push_back(master->make_variable(
                     leftfunc->variable_type(k),
                     leftfunc->variable_index(k)));
    }

    // calculate partsleft and partsright
    for (i = 0; i < funcpartsleft.size(); i++)
      partsleft[i][j] = funcpartsleft[i][j]->apply(vars)->simplify();
    for (i = 0; i < funcpartsright.size(); i++)
      partsright[i][j] = funcpartsright[i][j]->apply(vars)->simplify();

    // free memory from funcparts and vars
    for (k = 0; k < vars.size(); k++) delete vars[k];
    for (i = 0; i < funcpartsleft.size(); i++)
      delete funcpartsleft[i][j];
    for (i = 0; i < funcpartsright.size(); i++)
      delete funcpartsright[i][j];
  }

  /**
   * special case 1: a constraint a * F(p1,...,pn) >= b * F(q1,...,qn)
   * valid if a >= b, and either b = 0 or each pi >= qi
   */
  if (l->number_children() == 1 && r->number_children() == 1) {
    PPol b = r->get_child(0)->copy()->simplify();
    // a >= b
    add_formula(new Or(new AntiVar(vindex),
        new Var(var_for_constraint(l->simplify(), r->simplify()))));
    // p >= q
    int u = var_for_constraint(new Integer(0), b);
    for (j = 0; j < n; j++) {
      add_formula(new Or(new AntiVar(vindex), new Var(u),
          new Var(var_for_constraint(partsleft[0][j], partsright[0][j]))));
    }
    return;
  }

  /**
   * special case 2: a constraint a1 * F(p1) + ... + an * F(pn) >= bF(q)
   * holds if there are bits c1, ..., cn such that
   * a1*c1 + ... + an*cn >= b AND
   * for each i: either ci = 0 or p1 >= q
   */
  if (r->number_children() == 1) {
    // create the ci
    vector<PPol> bits;
    for (i = 0; i < l->number_children(); i++)
      bits.push_back(master->new_unknown(1));
    // for all i, either ci = 0 or pi >= q
    for (i = 0; i < l->number_children(); i++) {
      int u = var_for_constraint(new Integer(0), bits[i]->copy());
      for (j = 0; j < n; j++) {
        PPol pji = partsleft[i][j];
        PPol q = partsright[0][j]->copy();
        add_formula(new Or(new AntiVar(vindex), new Var(u),
                           new Var(var_for_constraint(pji, q))));
      }
    }
    // delete remaining partsright
    for (j = 0; j < n; j++) delete partsright[0][j];
    // a1*c1 + ... + an*cn >= b
    Sum *csum = new Sum();
    for (i = 0; i < l->number_children(); i++) {
      csum->add_child(new Product(l->get_child(i)->copy(), bits[i]));
    }
    add_formula(new Or(new AntiVar(vindex),
      new Var(var_for_constraint(csum->simplify(), r->simplify()))));
    return;
  }

  /**
   * finally, a1 * F(p1) + ... + an * F(pn) >= b1 * F(q1) + ... +
   * bm * F(qm) if each ai >= ai1 + ... + aim AND a1j + ... + anj >=
   * bj AND always either aij = 0 or pi >= qj
   */
  // create all aij
  vector<Polvec> a;
  for (i = 0; i < l->number_children(); i++) {
    Polvec ai;
    for (j = 0; j < r->number_children(); j++) {
      Unknown *aij = master->new_unknown(MAX_SPECIAL_UNKNOWN);
      ai.push_back(aij);
    }
    a.push_back(ai);
  }
  // add the requirements that always ai >= ai1 + ... + aim
  for (i = 0; i < l->number_children(); i++) {
    Sum *aisum = new Sum();
    for (j = 0; j < r->number_children(); j++) {
      aisum->add_child(a[i][j]->copy());
    }
    PPol lh = l->replace_child(i, new Integer(0))->simplify();
    int cindex = var_for_constraint(lh->simplify(), aisum->simplify());
    add_formula(new Or(new AntiVar(vindex), new Var(cindex)));
  }
  // add the requirements that always a1j + ... + anj >= bj
  for (j = 0; j < r->number_children(); j++) {
    Sum *ajsum = new Sum();
    for (i = 0; i < l->number_children(); i++) {
      ajsum->add_child(a[i][j]->copy());
    }
    PPol rh = r->replace_child(j, new Integer(0))->simplify();
    add_formula(new Or(new AntiVar(vindex),
                       new Var(var_for_constraint(ajsum->simplify(), rh))));
  }
  // also require for each i, j that either aij = 0, or pi >= qi
  for (i = 0; i < l->number_children(); i++) {
    for (j = 0; j < r->number_children(); j++) {
      int num = var_for_constraint(new Integer(0), a[i][j]);
      for (k = 0; k < n; k++) {
        PPol ll = partsleft[i][k]->copy();
        PPol rr = partsright[j][k]->copy();
        add_formula(new Or(new AntiVar(vindex), new Var(num),
                      new Var(var_for_constraint(ll, rr))));
      }
    }
  }

  // clean up
  for (i = 0; i < partsleft.size(); i++)
    for (k = 0; k < n; k++) delete partsleft[i][k];
  for (i = 0; i < partsright.size(); i++)
    for (k = 0; k < n; k++) delete partsright[i][k];
  delete left;
  delete right;
}

/*
void unit_propagate(And *formula) {
  bool changed = true;
  set<int> nulls;
  while (changed) {
    changed = false;
    for (int i = 0; i < formula->query_number_children(); i++) {
      PFormula child = formula->query_child(i)->simplify();

      if (child->query_variable() || child->query_antivariable()) {
        changed = true;
        dynamic_cast<Atom*>(child)->force();
      }
      
      if (child->query_special("integer arithmetic")) {
        IntegerArithmeticConstraint* iac =
          dynamic_cast<IntegerArithmeticConstraint*>(child);
        PPol l = iac->query_left();
        PPol r = iac->query_right();
        if (l->query_integer() && r->query_unknown() &&
            dynamic_cast<Integer*>(r)->query_value() == 0) {
          int index = dynamic_cast<Unknown*>(r)->query_index();
          cout << "found 0 element: " << index << endl;
        }
      }
      
      formula->replace_child(i, child);
    }
  }
}
*/

And *PolConstraintList :: generate_complete_formula() {
  And *ret = new And();
  int i;

  for (i = 0; i < formulas.size(); i++)
    ret->add_child(formulas[i]->copy());

  for (i = 0; i < constraints.size(); i++)
    if (constraints[i]->active) {
      ret->add_child(new Or(
        new AntiVar(constraints[i]->variable_index),
        new IntegerArithmeticConstraint(
          constraints[i]->left->copy(),
          constraints[i]->right->copy()
      )));
    }

  return ret;
}

string PolConstraintList :: print() {
  string ret = "";
  int i;

  for (i = 0; i < constraints.size(); i++) {
    char tmp[10];
    sprintf(tmp, "%d", i);
    ret += string(tmp) + "] " + print_poly_constraint(i);
    if (constraints[i]->active) ret += " (active)";
    ret += "\n";
  }
  ret += "\n";

  for (i = 0; i < formulas.size(); i++)
    ret += formulas[i]->to_string() + "\n";

  return ret;
}

void PolConstraintList :: debug_print() {
  wout.start_table();
  for (int i = 0; i < constraints.size(); i++) {
    char tmp[10];
    vector<string> columns;
    string left, right, line;
    map<int,int> freename, boundname;

    sprintf(tmp, "%d", i);
    columns.push_back(string(tmp) + "] ");
    left = wout.print_polynomial(constraints[i]->left, freename, boundname);
    right = wout.print_polynomial(constraints[i]->right, freename, boundname);
    line = left + " " + wout.polgeq_symbol() + " " + right;
    if (constraints[i]->active) line += " (active)";
    columns.push_back(line);
    wout.table_entry(columns);
  }
  wout.end_table();
  wout.start_table();
  for (int i = 0; i < formulas.size(); i++) {
    vector<string> columns;
    columns.push_back(formulas[i]->to_string());
    wout.table_entry(columns);
  }
  wout.end_table();
}


IntegerArithmeticConstraint :: IntegerArithmeticConstraint(PPol l, PPol r)
  :left(l), right(r) {
}

IntegerArithmeticConstraint :: ~IntegerArithmeticConstraint() {
  delete left;
  delete right;
}

PFormula IntegerArithmeticConstraint :: simplify_main() {
  return this;
}

PFormula IntegerArithmeticConstraint :: copy() {
  return new IntegerArithmeticConstraint(left->copy(), right->copy());
}

string IntegerArithmeticConstraint :: to_string(bool brackets) {
  map<int,int> freerename, boundrename;
  string ret = left->to_string(freerename, boundrename) + " >= ";
  return ret + right->to_string(freerename, boundrename);
}

bool IntegerArithmeticConstraint :: query_special(string description) {
  return description == "integer arithmetic";
}

PPol IntegerArithmeticConstraint :: query_left() {
  return left;
}

PPol IntegerArithmeticConstraint :: query_right() {
  return right;
}

PPol IntegerArithmeticConstraint :: replace_left(PPol l) {
  PPol ret = left;
  left = l;
  return ret;
}

PPol IntegerArithmeticConstraint :: replace_right(PPol r) {
  PPol ret = right;
  right = r;
  return ret;
}

