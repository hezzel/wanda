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

#include "smt.h"
#include "bitblaster.h"
#include <iostream>

Smt :: Smt(vector<int> &mins, vector<int> &maxs) {
  minima.insert(minima.end(), mins.begin(), mins.end());
  maxima.insert(maxima.end(), maxs.begin(), maxs.end());
}

bool Smt :: solve(PFormula formula, vector<int> &values) {
  check_minmax(formula);

  bool changed = true;
  while (changed && formula->query_conjunction()) {
    And *aa = dynamic_cast<And*>(formula);
    changed = false;
    changed |= unit_propagate(aa);
    changed |= obvious_propagate(aa);
    if (changed) formula = simplify_formula(aa->simplify());
    if (check_single_sides(formula)) {
      changed = true;
      formula = simplify_formula(formula->simplify());
    }
  }

  save_squares(formula);
  save_unknown_products(formula);
  save_known_products(formula);
  save_sums(formula);
  
  formula = formula->simplify()->conjunctive_form();

  // bit blast!
  BitBlaster blaster(minima, maxima);
  blaster.set_squares(squares);
  blaster.set_unknown_products(unknown_products);
  blaster.set_known_products(known_products);
  blaster.set_unknown_sums(unknown_sums);
  blaster.set_known_sums(known_sums);
  if (!blaster.solve(formula, values)) return false;

  // test answers
  test_solution(values, blaster.overflow_bound());

  return true;
}

/* ========== updating and simplifying the formula ========== */

void Smt :: global_replace(PFormula f, map<int,PPol> &substitution) {
  if (f->query_conjunction() || f->query_disjunction()) {
    AndOr *formula = dynamic_cast<AndOr*>(f);
    for (int i = 0; i < formula->query_number_children(); i++) {
      global_replace(formula->query_child(i), substitution);
    }
  }

  else if (f->query_special("integer arithmetic")) {
    IntegerArithmeticConstraint *p =
      dynamic_cast<IntegerArithmeticConstraint*>(f);
    PPol l = p->query_left()->replace_unknowns(substitution)->simplify();
    PPol r = p->query_right()->replace_unknowns(substitution)->simplify();
    p->replace_left(l);
    p->replace_right(r);
  }
}

void Smt :: check_minmax(PFormula formula) {
  map<int,PPol> substitution;
  for (int i = 0; i < minima.size(); i++) {
    if (minima[i] == maxima[i])
      substitution[i] = new Integer(maxima[i]);
  }
  if (substitution.size() != 0) global_replace(formula, substitution);
}

PFormula Smt :: simplify_formula(PFormula f) {
  if (f->query_conjunction() || f->query_disjunction()) {
    AndOr *formula = dynamic_cast<AndOr*>(f);
    bool simplify_needed = false;
    for (int i = 0; i < formula->query_number_children(); i++) {
      PFormula child = formula->query_child(i);
      PFormula newchild = simplify_formula(child);
      if (newchild != child) {
        formula->replace_child(i, newchild);
        simplify_needed = true;
      }
    }
    if (simplify_needed) return formula->top_simplify();
    else return formula;
  }

  else if (f->query_special("integer arithmetic")) {
    IntegerArithmeticConstraint *p =
      dynamic_cast<IntegerArithmeticConstraint*>(f);
    remove_duplicates(p);
    return simplify_arithmetic(p);
  }

  return f;
}

void Smt :: remove_duplicates(IntegerArithmeticConstraint *p) {
  PPol left = p->query_left();
  PPol right = p->query_right();
  bool bother;
  int a, b;

  // check whether we're going to bother
  if (!left->query_sum() && !right->query_sum())
    bother = left->query_similar(right, a, b);
  else if (!left->query_sum()) {
    bother = false;
    for (int i = 0; i < left->number_children() && !bother; i++)
      bother |= left->get_child(i)->query_similar(right, a, b);
  }
  else if (!right->query_sum()) {
    bother = false;
    for (int i = 0; i < right->number_children() && !bother; i++)
      bother |= right->get_child(i)->query_similar(left, a, b);
  }
  else bother = true;

  if (!bother) return;

  Sum *l, *r;

  if (!left->query_sum()) l = new Sum(left);
  else l = dynamic_cast<Sum*>(left);
  if (!right->query_sum()) r = new Sum(right);
  else r = dynamic_cast<Sum*>(right);
  PPol lnew = NULL, rnew = NULL;

  for (int i = 0; i < l->number_children(); i++) {
    for (int j = 0; j < r->number_children(); j++) {
      if (l->get_child(i)->query_similar(r->get_child(j), a, b)) {
        if (lnew == NULL) {
          lnew = dynamic_cast<Sum*>(l->copy());
          rnew = dynamic_cast<Sum*>(r->copy());
        }
        int ind1 = (a >= b ? a-b : 0); 
        int ind2 = (b >= a ? b-a : 0);
        PPol c1 = lnew->get_child(i);
        if (c1->query_product() && c1->get_child(0)->query_integer()) {
          Integer* ipol = dynamic_cast<Integer*>(c1->get_child(0));
          ipol->set_value(ind1);
        }
        else lnew->replace_child(i, new Product(new Integer(ind1), c1));
        PPol c2 = rnew->get_child(j);
        if (c2->query_product() && c2->get_child(0)->query_integer()) {
          Integer* ipol = dynamic_cast<Integer*>(c2->get_child(0));
          ipol->set_value(ind2);
        }
        else rnew->replace_child(j, new Product(new Integer(ind2), c2));
      }   
    }   
  }

  if (lnew == NULL) {
    if (l != left) l->flat_free();
    if (r != right) r->flat_free();
  }

  else {
    lnew = lnew->simplify();
    rnew = rnew->simplify();
    p->replace_left(lnew);
    p->replace_right(rnew);
  }
}

PFormula Smt :: simplify_arithmetic(IntegerArithmeticConstraint *p) {
  PPol l = p->query_left();
  PPol r = p->query_right();

  // right-hand side an integer
  if (r->query_integer()) {
    int rr = dynamic_cast<Integer*>(r)->query_value();
    if (rr == 0) { delete p; return new Top(); }
    if (rr == 1 && l->query_sum()) {
      Or *result = new Or();
      for (int i = 0; i < l->number_children(); i++) {
        result->add_child(new IntegerArithmeticConstraint(
          l->get_child(i)->copy(), r->copy()));
      }
      delete p;
      return simplify_formula(result->simplify());
    }
    if (rr == 1 && l->query_product()) {
      And *result = new And();
      for (int i = 0; i < l->number_children(); i++) {
        result->add_child(new IntegerArithmeticConstraint(
          l->get_child(i)->copy(), r->copy()));
      }
      delete p;
      return simplify_formula(result->simplify());
    }
    if (l->query_unknown()) {
      int index = dynamic_cast<Unknown*>(l)->query_index();
      if (minima[index] >= rr) { delete p; return new Top; }
      if (maxima[index] < rr) { delete p; return new Bottom; }
    }
    if (l->query_integer()) {
      int ll = dynamic_cast<Integer*>(l)->query_value();
      delete p;
      if (ll >= rr) return new Top();
      else return new Bottom();
    }
  }

  // left-hand side an integer
  if (l->query_integer()) {
    int ll = dynamic_cast<Integer*>(l)->query_value();
    if (ll == 0 && r->query_product()) {
      Or *result = new Or();
      for (int i = 0; i < r->number_children(); i++) {
        result->add_child(new IntegerArithmeticConstraint(
          l->copy(), r->get_child(i)->copy()));
      }
      delete p;
      return simplify_formula(result->simplify());
    }
    if (ll == 0 && r->query_sum()) {
      And *result = new And();
      for (int i = 0; i < r->number_children(); i++) {
        result->add_child(new IntegerArithmeticConstraint(
          l->copy(), r->get_child(i)->copy()));
      }
      delete p;
      return simplify_formula(result->simplify());
    }
    if (r->query_unknown()) {
      int index = dynamic_cast<Unknown*>(r)->query_index();
      if (maxima[index] <= ll) { delete p; return new Top; }
      if (minima[index] > ll) { delete p; return new Bottom; }
    }
  }

  return p;
}

/* ========== obvious decisions ========== */

bool Smt :: unit_propagate(And *formula) {
  bool changed = false;
  for (int i = 0; i < formula->query_number_children(); i++) {
    PFormula child = formula->query_child(i)->simplify();
    if (child->query_variable() || child->query_antivariable()) {
      changed = true;
      dynamic_cast<Atom*>(child)->force();
      delete child;
      child = new Top();
    }
    if (child != formula->query_child(i))
      formula->replace_child(i, child);
  }
  return changed;
}

bool Smt :: obvious_propagate(And *formula) {
  map<int,PPol> substitution;
  bool changedstuff = false;

  for (int i = 0; i < formula->query_number_children(); i++) {
    PFormula current = formula->query_child(i);
    if (!current->query_special("integer arithmetic")) continue;
    IntegerArithmeticConstraint* cur =
      dynamic_cast<IntegerArithmeticConstraint*>(current);
    PPol left = cur->query_left();
    PPol right = cur->query_right();
    bool special = false;
    int id;
    
    if (left->query_integer() && right->query_unknown()) {
      int ll = dynamic_cast<Integer*>(left)->query_value();
      id = dynamic_cast<Unknown*>(right)->query_index();
      if (maxima[id] > ll) maxima[id] = ll;
      special = true;
    }

    if (left->query_unknown() && right->query_integer()) {
      int rr = dynamic_cast<Integer*>(right)->query_value();
      id = dynamic_cast<Unknown*>(left)->query_index();
      if (minima[id] < rr) minima[id] = rr;
      special = true;
    }

    if (special) {
      if (maxima[id] >= minima[id])
        delete formula->replace_child(i, new Top());
      else delete formula->replace_child(i, new Bottom());
      if (maxima[id] == minima[id]) {
        substitution[id] = new Integer(maxima[id]);
      }
      changedstuff = true;
    }
  }

  if (substitution.size() != 0) global_replace(formula, substitution);

  return changedstuff;
}

/* ========== less obvious decisions ========== */

bool Smt :: check_single_sides(PFormula formula) {
  set<int> lefts, rights;
  vector<PFormula> forms;
  vector<PPol> pols_left, pols_right;
  int i, j;

  forms.push_back(formula);
  for (i = 0; i < forms.size(); i++) {
    if (forms[i]->query_conjunction() || forms[i]->query_disjunction()) {
      AndOr *cur = dynamic_cast<AndOr*>(forms[i]);
      for (j = 0; j < cur->query_number_children(); j++) {
        forms.push_back(cur->query_child(j));
      }
    }
    if (forms[i]->query_special("integer arithmetic")) {
      IntegerArithmeticConstraint* cur =
        dynamic_cast<IntegerArithmeticConstraint*>(forms[i]);
      pols_left.push_back(cur->query_left());
      pols_right.push_back(cur->query_right());
    }
  }

  for (i = 0; i < pols_left.size(); i++) {
    for (j = 0; j < pols_left[i]->number_children(); j++) {
      pols_left.push_back(pols_left[i]->get_child(j));
    }
    if (pols_left[i]->query_unknown()) {
      Unknown *a = dynamic_cast<Unknown*>(pols_left[i]);
      lefts.insert(a->query_index());
    }
  }

  for (i = 0; i < pols_right.size(); i++) {
    for (j = 0; j < pols_right[i]->number_children(); j++) {
      pols_right.push_back(pols_right[i]->get_child(j));
    }
    if (pols_right[i]->query_unknown()) {
      Unknown *a = dynamic_cast<Unknown*>(pols_right[i]);
      rights.insert(a->query_index());
    }
  }

  set<int> l;
  map<int,PPol> substitution;

  for (set<int>::iterator it = lefts.begin();
       it != lefts.end(); it++) {
    if (rights.find(*it) == rights.end()) l.insert(*it);
    else rights.erase(*it);
  }

  for (set<int>::iterator it = l.begin();
       it != l.end(); it++) {
    int k = *it;
    if (k > maxima.size()) cout << "Error!" << endl;
    minima[k] = maxima[k];
    substitution[k] = new Integer(maxima[k]);
  }

  for (set<int>::iterator it = rights.begin();
       it != rights.end(); it++) {
    int k = *it;
    if (k > minima.size()) cout << "Error!" << endl;
    maxima[k] = minima[k];
    substitution[k] = new Integer(minima[k]);
  }

  if (substitution.size() != 0) {
    global_replace(formula, substitution);
    return true;
  }

  return false;
}

/* ========== fresh equalities a := P(b,c) ========== */

void Smt :: get_arithmetic(PFormula formula,
                           vector<IntegerArithmeticConstraint*> &v) {
  vector<PFormula> forms;
  forms.push_back(formula);
  for (int i = 0; i < forms.size(); i++) {
    if (forms[i]->query_conjunction() ||
        forms[i]->query_disjunction()) {
      AndOr *f = dynamic_cast<AndOr*>(forms[i]);
      for (int j = 0; j < f->query_number_children(); j++)
        forms.push_back(f->query_child(j));
    }
    else if (forms[i]->query_special("integer arithmetic")) {
      IntegerArithmeticConstraint *p =
        dynamic_cast<IntegerArithmeticConstraint*>(forms[i]);
      v.push_back(p);
    }
  }
}

void Smt :: save_squares(PFormula formula) {
  map<int,int> used_squares; // maps x to the index of x^2
  vector<IntegerArithmeticConstraint*> v;

  get_arithmetic(formula, v);
  
  for (int i = 0; i < v.size(); i++) {
    PPol left = handle_squares(v[i]->query_left(), used_squares);
    PPol right = handle_squares(v[i]->query_right(), used_squares);
    v[i]->replace_left(left);
    v[i]->replace_right(right);
  }
}

PPol Smt :: handle_squares(PPol p, map<int,int> &used_squares) {
  int i;
  bool anything_changed = false;

  for (i = 0; i < p->number_children(); i++)
    p->replace_child(i, handle_squares(p->get_child(i), used_squares));

  if (!p->query_product()) return p;

  for (i = 0; i < p->number_children()-1; i++) {
    if (!p->get_child(i)->query_unknown()) continue;
    if (!p->get_child(i+1)->query_unknown()) continue;
    int id1 = dynamic_cast<Unknown*>(p->get_child(i))->query_index();
    int id2 = dynamic_cast<Unknown*>(p->get_child(i+1))->query_index();
    if (id1 != id2) continue;
    anything_changed = true;
    if (used_squares.find(id1) == used_squares.end()) {
      used_squares[id1] = minima.size();
      squares[minima.size()] = id1;
      minima.push_back(minima[id1]*minima[id1]);
      maxima.push_back(maxima[id1]*maxima[id1]);
    }
    delete p->replace_child(i, new Integer(1));
    delete p->replace_child(i+1, new Unknown(used_squares[id1]));
  }

  if (!anything_changed) return p;
  else return handle_squares(p->simplify(), used_squares);
}

void Smt :: guarantee_existence(map<int,IntMap> &used, int id1,
                                int id2, int min, int max) {
  if (used.find(id1) == used.end()) {
    IntMap im;
    im[id2] = minima.size();
    used[id1] = im;
    minima.push_back(min);
    maxima.push_back(max);
  }
  else if (used[id1].find(id2) == used[id1].end()) {
    used[id1][id2] = minima.size();
    minima.push_back(min);
    maxima.push_back(max);
  }
}

void Smt :: save_unknown_products(PFormula formula) {
  map<int,map<int,int> > used_products; // maps x, y to the index of xy
  vector<IntegerArithmeticConstraint*> v;
  get_arithmetic(formula, v);
  for (int i = 0; i < v.size(); i++) {
    PPol left = handle_unknown_products(v[i]->query_left(), used_products);
    PPol right = handle_unknown_products(v[i]->query_right(), used_products);
    v[i]->replace_left(left);
    v[i]->replace_right(right);
  }
}

PPol Smt :: handle_unknown_products(PPol p, map<int,IntMap> &used) {
  int i;

  for (i = 0; i < p->number_children(); i++)
    p->replace_child(i, handle_unknown_products(p->get_child(i), used));

  if (!p->query_product()) return p;
  if (p->number_children() == 2 && p->get_child(0)->query_integer()) return p;

  int multiplier = 1;
  int eventual = -1;

  for (i = 0; i < p->number_children(); i++) {
    PPol child = p->get_child(i);
    if (child->query_integer()) {
      int val = dynamic_cast<Integer*>(child)->query_value();
      multiplier *= val;
      continue;
    }
    if (!child->query_unknown()) {
      cout << "Error: strange product in handle_unknown_products" << endl;
      cout << "p = " << p->to_string() << endl;
      return p;
    }
    int id = dynamic_cast<Unknown*>(child)->query_index();
    if (eventual == -1) { eventual = id; continue; }
    int id1, id2;
    if (id < eventual) { id1 = id; id2 = eventual; }
    else { id1 = eventual; id2 = id; }
    int min1 = minima[id1], min2 = minima[id2],
        max1 = maxima[id1], max2 = maxima[id2];
    min1 = min1 > 100 ? 100 : min1;
    min2 = min2 > 100 ? 100 : min2;
    max1 = max1 > 100 ? 100 : max1;
    max2 = max2 > 100 ? 100 : max2;
    guarantee_existence(used, id1, id2, min1 * min2, max1 * max2);
    eventual = used[id1][id2];
    unknown_products[eventual] = pair<int,int>(id1, id2);
  }

  delete p;
  if (multiplier == 1) return new Unknown(eventual);
  else if (multiplier == 0) return new Integer(0);
  else {
    Integer *itg = new Integer(multiplier);
    Unknown *evt = new Unknown(eventual);
    return new Product(itg, evt);
  }
}

void Smt :: save_known_products(PFormula formula) {
  map<int,map<int,int> > used_products; // maps x, y to the index of xy
  vector<IntegerArithmeticConstraint*> v;
  get_arithmetic(formula, v);
  for (int i = 0; i < v.size(); i++) {
    PPol left = handle_known_products(v[i]->query_left(), used_products);
    PPol right = handle_known_products(v[i]->query_right(), used_products);
    v[i]->replace_left(left);
    v[i]->replace_right(right);
  }
}

PPol Smt :: handle_known_products(PPol p, map<int,IntMap> &used) {
  int i;

  for (i = 0; i < p->number_children(); i++)
    p->replace_child(i, handle_known_products(p->get_child(i), used));

  if (!p->query_product()) return p;
  if (p->number_children() != 2 ||
      !p->get_child(0)->query_integer() ||
      !p->get_child(1)->query_unknown()) {
    cout << "Error: strange product in handle_known_products" << endl;
    return p;
  }

  int multiplier = dynamic_cast<Integer*>(p->get_child(0))->query_value();
  int unknown = dynamic_cast<Unknown*>(p->get_child(1))->query_index();
  guarantee_existence(used, multiplier, unknown, multiplier * minima[unknown],
                      multiplier * maxima[unknown]);
  int output = used[multiplier][unknown];
  known_products[output] = pair<int,int>(multiplier, unknown);

  delete p;
  return new Unknown(output);
}

void Smt :: save_sums(PFormula formula) {
  map<int,IntMap> used_known, used_unknown;
  vector<IntegerArithmeticConstraint*> v;
  get_arithmetic(formula, v);
  for (int i = 0; i < v.size(); i++) {
    PPol left = handle_sums(v[i]->query_left(), used_known, used_unknown);
    PPol right = handle_sums(v[i]->query_right(), used_known, used_unknown);
    v[i]->replace_left(left);
    v[i]->replace_right(right);
  }
}

PPol Smt :: handle_sums(PPol p, map<int,IntMap> &used_known,
                        map<int,IntMap> &used_unknown) {
  int i;

  for (i = 0; i < p->number_children(); i++)
    p->replace_child(i, handle_sums(p->get_child(i), used_known,
                                    used_unknown));

  if (!p->query_sum()) return p;

  int base = 0;
  int current = -1;

  for (i = 0; i < p->number_children(); i++) {
    PPol child = p->get_child(i);
    if (child->query_integer()) {
      int val = dynamic_cast<Integer*>(child)->query_value();
      base += val;
      continue;
    }
    if (!child->query_unknown()) {
      cout << "Error: strange sum in handle_unknown_products" << endl;
      cout << "p = " << p->to_string() << endl;
      return p;
    }
    int id = dynamic_cast<Unknown*>(child)->query_index();
    if (current == -1) { current = id; continue; }
    int id1, id2;
    if (id < current) { id1 = id; id2 = current; }
    else { id1 = current; id2 = id; }
    guarantee_existence(used_unknown, id1, id2, minima[id1] + minima[id2],
                        maxima[id1] + maxima[id2]);
    current = used_unknown[id1][id2];
    unknown_sums[current] = pair<int,int>(id1, id2);
  }

  delete p;
  if (base == 0) return new Unknown(current);
  else {
    guarantee_existence(used_known, base, current, base + minima[current],
                        base + maxima[current]);
    int newid = used_known[base][current];
    known_sums[newid] = pair<int,int>(base, current);
    return new Unknown(newid);
  }
}

void Smt :: test_equal(int a, int b, int overflow, string message) {
  if (a != b && (a < overflow || b < overflow)) {
    cout << "Equality problem: " << a << " != " << b;
    cout << "; " << message << endl;
  }
}

void Smt :: test_solution(vector<int> &values, int overflow) {
  for (map<int,int>::iterator it = squares.begin(); it != squares.end(); it++)
    test_equal(values[it->first], values[it->second] * values[it->second],
               overflow, "squares");
  for (map<int,pair<int,int> >::iterator it = unknown_products.begin();
       it != unknown_products.end(); it++)
    test_equal(values[it->first], values[it->second.first] *
               values[it->second.second], overflow, "unknown products");
  for (map<int,pair<int,int> >::iterator it = known_products.begin();
       it != known_products.end(); it++)
    test_equal(values[it->first], it->second.first * values[it->second.second],
               overflow, "known products");
  for (map<int,pair<int,int> >::iterator it = unknown_sums.begin();
       it != unknown_sums.end(); it++)
    test_equal(values[it->first], values[it->second.first] +
               values[it->second.second], overflow, "unknown sums");
  for (map<int,pair<int,int> >::iterator it = known_sums.begin();
       it != known_sums.end(); it++)
    test_equal(values[it->first], it->second.first + values[it->second.second],
               overflow, "known sums");
}

/*
bool BitBlaster :: replace_products(PFormula &formula) {
  DoubleIntmap dim;
  int best, best1, best2;
  int i, j, k;

  // step 1: get all products
  vector<PFormula> formulas;
  vector<PPol> polynomials, products;
  formulas.push_back(formula);
  for (i = 0; i < formulas.size(); i++) {
    PFormula form = formulas[i];
    if (form->query_conjunction() || form->query_disjunction()) {
      AndOr *f = dynamic_cast<AndOr*>(form);
      for (j = 0; j < f->query_number_children(); j++) {
        formulas.push_back(f->query_child(j));
      }
    }
    if (form->query_special("integer arithmetic")) {
      IntegerArithmeticConstraint *ar =
        dynamic_cast<IntegerArithmeticConstraint*>(form);
      polynomials.push_back(ar->query_left());
      polynomials.push_back(ar->query_right());
    }
  }
  for (i = 0; i < polynomials.size(); i++) {
    for (j = 0; j < polynomials[i]->number_children(); j++)
      polynomials.push_back(polynomials[i]->get_child(j));
    if (polynomials[i]->query_product())
      products.push_back(polynomials[i]);
  }

  // step 2: fill in all parts of products, and find out the best
  // combination while we're at it
  best = 0;
  for (i = 0; i < products.size(); i++) {
    PPol prod = products[i];
    for (j = 0; j < prod->number_children()-1; j++) {
      PPol child = prod->get_child(j);
      if (!child->query_unknown()) continue;
      int id1 = dynamic_cast<Unknown*>(child)->query_index();
      for (k = j + 1; k < prod->number_children(); k++) {
        PPol other = prod->get_child(k);
        if (!other->query_unknown()) continue;
        int id2 = dynamic_cast<Unknown*>(other)->query_index();
        dim[id1][id2] = dim[id1][id2] + 1;
        if (dim[id1][id2] > best) {
          best = dim[id1][id2];
          best1 = id1;
          best2 = id2;
        }
      }
    }
  }

  // step 3: if there are products around more than once, create a
  // suitable replacement
  if (best <= 1) return false;
  Representation productrep;
  create_product(unknowns[best1], unknowns[best2], productrep,
                 equality_info);
  int id = unknowns.size();
  unknowns.push_back(productrep);

  // step 4: replace all occurrences of the pair by the new unknown
  replace_products(formula, best1, best2, id);

  return true;
}

void BitBlaster :: replace_products(PFormula formula, int id1,
                                          int id2, int newid) {
  if (formula->query_special("integer arithmetic")) {
    IntegerArithmeticConstraint *ar =
      dynamic_cast<IntegerArithmeticConstraint*>(formula);
    PPol l = replace_products(ar->query_left(), id1, id2, newid);
    PPol r = replace_products(ar->query_right(), id1, id2, newid);
    ar->replace_left(l);
    ar->replace_right(r);
  }

  else if (formula->query_conjunction() || formula->query_disjunction()) {
    AndOr *f = dynamic_cast<AndOr*>(formula);
    for (int i = 0; i < f->query_number_children(); i++) {
      replace_products(f->query_child(i), id1, id2, newid);
    }
  }
}

PPol BitBlaster :: replace_products(PPol p, int id1, int id2, int newid) {
  int i, j;
  for (i = 0; i < p->number_children(); i++) {
    PPol child = p->get_child(i);
    p->replace_child(i, replace_products(child, id1, id2, newid));
  }

  if (p->query_product()) {
    for (i = 0; i+1 < p->number_children(); i++) {
      PPol child = p->get_child(i);
      if (!child->query_unknown()) continue;
      if (dynamic_cast<Unknown*>(child)->query_index() != id1) continue;
      for (j = i+1; j < p->number_children(); j++) {
        child = p->get_child(j);
        if (!child->query_unknown()) continue;
        if (dynamic_cast<Unknown*>(child)->query_index() != id2) continue;
        // we have a match!
        delete p->replace_child(i, new Integer(1));
        delete p->replace_child(j, new Unknown(newid));
        return p->simplify();
      }
    }
  }

  return p;
}
*/
