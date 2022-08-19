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

#include "polynomial.h"
#include <cstdio>
#include <iostream>

int Polynomial :: query_type() {
  return -1;
}

int Polynomial :: compare(PPol other) {
  return cmp(query_type(), other->query_type());
}

int Polynomial :: cmp(int a, int b) {
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

void Polynomial :: sort(vector<PPol> &vec) {
  for (int i = 0; i+1 < vec.size(); i++) {
    for (int j = i+1; j < vec.size(); j++) {
      if (vec[i]->compare(vec[j]) == 1) {
        PPol tmp = vec[j];
        vec[j] = vec[i];
        vec[i] = tmp;
      }
    }
  }
}

string Polynomial :: to_string(bool brackets) {
  return "ERR";
}

string Polynomial :: to_string(map<int,int> &freerename,
                               map<int,int> &boundrename,
                               bool brackets) {
  return to_string(brackets);
}

PPol Polynomial :: simplify() {
  return this;
}

PPol Polynomial :: replace_unknowns(map<int,PPol> &subst) {
  return this;
}

PPol Polynomial :: apply_substitution(PolynomialSubstitution &substitution) {
  return this;
}

PPol Polynomial :: copy() {
  return this;
}

bool Polynomial :: equals(PPol other) {
  return compare(other) == 0;
}

bool Polynomial :: query_similar(PPol other, int &a, int &b) {
  if (other->query_product()) return other->query_similar(this, b, a);
  if (equals(other)) { a = 1; b = 1; return true; }
  return false;
}

bool Polynomial :: query_integer() { return false; }
bool Polynomial :: query_unknown() { return false; }
bool Polynomial :: query_variable() { return false; }
bool Polynomial :: query_functional() { return false; }
bool Polynomial :: query_max() { return false; }
bool Polynomial :: query_sum() { return false; }
bool Polynomial :: query_product() { return false; }

int Polynomial :: number_children() { return 0; }
PPol Polynomial :: get_child(int index) { return NULL; }
PPol Polynomial :: replace_child(int index, PPol replace) { return NULL; }

void Polynomial :: vars(vector<Polvar*> initial) { };

Integer :: Integer(int val) : value(val) {}

int Integer :: query_type() { return 0; }

int Integer :: compare(PPol other) {
  if (!other->query_integer())
    return cmp(query_type(), other->query_type());
  return cmp(value, dynamic_cast<Integer*>(other)->query_value());
}

string Integer :: to_string(bool brackets) {
  char tmp[10];
  sprintf(tmp, "%d", value);
  return string(tmp);
}

PPol Integer :: copy() {
  return new Integer(value);
}

bool Integer :: query_integer() { return true; }

int Integer :: query_value() {
  return value;
}

void Integer :: set_value(int val) {
  value = val;
}

Unknown :: Unknown(int ind) :index(ind) {}

int Unknown :: query_type() { return 2; }

int Unknown :: compare(PPol other) {
  if (!other->query_unknown())
    return cmp(query_type(), other->query_type());
  return cmp(index, dynamic_cast<Unknown*>(other)->query_index());
}

string Unknown :: to_string(bool brackets) {
  char tmp[10];
  sprintf(tmp, "a%d", index);
  return string(tmp);
}

PPol Unknown :: copy() {
  return new Unknown(index);
}

PPol Unknown :: replace_unknowns(map<int,PPol> &substitution) {
  if (substitution.find(index) == substitution.end()) return this;
  PPol ret = substitution[index]->copy();
  delete this;
  return ret;
}

bool Unknown :: query_unknown() { return true; }

int Unknown :: query_index() {
  return index;
}

Polvar :: Polvar(int ind) :index(ind) {}

int Polvar :: query_type() { return 4; }

int Polvar :: compare(PPol other) {
  if (!other->query_variable())
    return cmp(query_type(), other->query_type());
  return cmp(index, dynamic_cast<Polvar*>(other)->query_index());
}

string Polvar :: to_string(bool brackets) {
  char tmp[10];
  sprintf(tmp, "x%d", index);
  return string(tmp);
}

string Polvar :: to_string(map<int,int> &freerename,
                           map<int,int> &boundrename,
                           bool brackets) {
  char tmp[10];
  
  if (boundrename.find(index) != boundrename.end()) {
    sprintf(tmp, "y%d", boundrename[index]);
    return string(tmp);
  }

  if (freerename.find(index) == freerename.end()) {
    int k = freerename.size();
    freerename[index] = k;
  }

  sprintf(tmp, "x%d", freerename[index]);
  return string(tmp);
}

PPol Polvar :: copy() {
  return new Polvar(index);
}

PPol Polvar :: apply_substitution(PolynomialSubstitution &subst) {
  if (subst.find(index) == subst.end()) return this;
  vector<PolynomialFunction*> empty;
  PPol ret = subst[index]->apply(empty);
  delete this;
  return ret;
}

bool Polvar :: query_variable() { return true; }

int Polvar :: query_index() {
  return index;
}

void Polvar :: vars(vector<Polvar*> initial) {
  initial.push_back(this);
};

Functional :: Functional(int index) : funindex(index) {}

Functional :: Functional(int index, PPol a) :funindex(index) {
  vector<int> vars;
  args.push_back(new PolynomialFunction(vars, a));
}

Functional :: Functional(int index, vector<PPol> a) :funindex(index) {
  for (int i = 0; i < a.size(); i++) {
    vector<int> vars;
    args.push_back(new PolynomialFunction(vars, a[i]));
  }
}

Functional :: Functional(int index, vector<PolynomialFunction*> a)
  :funindex(index), args(a) {
}

Functional :: ~Functional() {
  for (int i = 0; i < args.size(); i++) delete args[i];
}

int Functional :: query_type() { return 6; }

int Functional :: compare(PPol other) {
  if (!other->query_functional())
    return cmp(query_type(), other->query_type());
  Functional *ot = dynamic_cast<Functional*>(other);
  int ret = cmp(funindex, ot->funindex);
  if (ret != 0) return ret;
  ret = cmp(args.size(), ot->args.size());
  if (ret != 0) return ret;
  for (int i = 0; i < args.size(); i++) {
    ret = args[i]->get_polynomial()->compare(ot->args[i]->get_polynomial());
      // TODO: might miss equality by alpha-equality!
    if (ret != 0) return ret;
  }
  return 0;
}

string Functional :: to_string(bool brackets) {
  char tmp[10];
  sprintf(tmp, "F%d", funindex);
  string ret = tmp;
  ret += "(";
  for (int i = 0; i < args.size(); i++) {
    if (i != 0) ret += ",";
    ret += args[i]->to_string();
  }
  ret += ")";
  return ret;
}

string Functional :: to_string(map<int,int> &freerename,
                               map<int,int> &boundrename,
                               bool brackets) {

  string ret;
  char tmp[10];

  if (boundrename.find(funindex) != boundrename.end()) {
    sprintf(tmp, "G%d", boundrename[funindex]);
    ret = string(tmp);
  }
  else {
    if (freerename.find(funindex) == freerename.end()) {
      int k = freerename.size();
      freerename[funindex] = k;
    }
    sprintf(tmp, "F%d", freerename[funindex]);
    ret = string(tmp);
  }

  ret += "(";
  for (int i = 0; i < args.size(); i++) {
    if (i != 0) ret += ",";
    ret += args[i]->to_string(freerename, boundrename);
  }
  ret += ")";

  return ret;
}

PPol Functional :: copy() {
  vector<PolynomialFunction*> args2;
  for (int i = 0; i < args.size(); i++)
    args2.push_back(args[i]->copy());
  return new Functional(funindex, args2);
}

PPol Functional :: simplify() {
  for (int i = 0; i < args.size(); i++)
    args[i]->replace_polynomial(args[i]->get_polynomial()->simplify());
  return this;
}

PPol Functional :: replace_unknowns(map<int,PPol> &substitution) {
  for (int i = 0; i < args.size(); i++)
    args[i]->replace_polynomial(
      args[i]->get_polynomial()->replace_unknowns(substitution));
  return this;
}

PPol Functional :: apply_substitution(PolynomialSubstitution &subst) {
  int i;
  
  for (i = 0; i < args.size(); i++) {
    args[i]->replace_polynomial(
      args[i]->get_polynomial()->apply_substitution(subst));
  }

  if (subst.find(funindex) == subst.end()) return this;

  PPol ret = subst[funindex]->apply(args);
  delete this;
  return ret;
}

bool Functional :: query_functional() { return true; }

int Functional :: function_index() {
  return funindex;
}

int Functional :: number_children() {
  return args.size();
}

PPol Functional :: get_child(int index) {
  if (index < 0 || index >= args.size()) return NULL;
  return args[index]->get_polynomial();
}

PPol Functional :: replace_child(int index, PPol replace) {
  if (index < 0 || index >= args.size()) return NULL;
  PPol old_polynomial = args[index]->get_polynomial();
  args[index]->replace_polynomial(replace);
  return old_polynomial;
}

PolynomialFunction *Functional :: get_function_child(int index) {
  if (index < 0 || index >= args.size()) return NULL;
  return args[index];
}

void Functional :: add_argument(PolynomialFunction *arg) {
  args.push_back(arg);
}

void Functional :: weak_delete() {
  args.clear();
  delete this;
}

void Functional :: vars(vector<Polvar*> initial) {
  for (int i = 0; i < args.size(); i++)
    args[i]->vars(initial);
}

Max :: Max() {}

Max :: Max(PPol left, PPol right) {
  parts.push_back(left);
  parts.push_back(right);
}

Max :: Max(vector<PPol> p) :parts(p) {}

Max :: ~Max() {
  for (int i = 0; i < parts.size(); i++) delete parts[i];
}

int Max :: query_type() { return 8; }

int Max :: compare(PPol other) {
  if (!other->query_max())
    return cmp(query_type(), other->query_type());
  Max *ot = dynamic_cast<Max*>(other);
  for (int i = 0; i < parts.size(); i++) {
    if (i >= ot->parts.size()) return 1;
    int ret = parts[i]->compare(ot->parts[i]);
    if (ret != 0) return ret;
  }
  if (parts.size() < ot->parts.size()) return -1;
  else return 0;
}

string Max :: to_string(bool brackets) {
  string ret = "max(";
  for (int i = 0; i < parts.size(); i++) {
    if (i != 0) ret += ", ";
    ret += parts[i]->to_string();
  }
  ret += ")";
  return ret;
}

string Max :: to_string(map<int,int> &frename, map<int,int> &brename,
                        bool brackets) {
  string ret = "max(";
  for (int i = 0; i < parts.size(); i++) {
    if (i != 0) ret += ", ";
    ret += parts[i]->to_string(frename, brename, false);
  }
  ret += ")";
  return ret;
}

PPol Max :: copy() {
  vector<PPol> parts2;
  for (int i = 0; i < parts.size(); i++)
    parts2.push_back(parts[i]->copy());
  return new Max(parts2);
}

PPol Max :: simplify() {
  int i;

  for (i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->simplify();

  // "flatten" nested maxes
  for (i = 0; i < parts.size(); i++) {
    if (parts[i]->query_max()) {
      Max *pi = dynamic_cast<Max*>(parts[i]);
      for (int k = 0; k < pi->number_children(); k++)
        parts.push_back(pi->get_child(k));
      pi->flat_free();
      parts[i] = parts[parts.size()-1];
      parts.pop_back();
      i--;
      continue;
    }
  }

  // sort the children
  sort(parts);

  // remove 0, and multiple integers (all integers are at the start
  // of parts smallest first
  while (parts.size() > 1 && parts[0]->query_integer()) {
    if (dynamic_cast<Integer*>(parts[0])->query_value() != 0 &&
        !parts[1]->query_integer()) break;
    delete parts[0];
    for (i = 1; i < parts.size(); i++) parts[i-1] = parts[i];
    parts.pop_back();
  }

  // remove equal parts
  for (i = 0; i < parts.size()-1; i++) {
    if (parts[i]->equals(parts[i+1])) {
      delete parts[i+1];
      for (int j = i+2; j < parts.size(); j++) parts[j-1] = parts[j];
      parts.pop_back();
    }
  }

  // finishing off
  if (parts.size() == 0) {
    delete this;
    return new Integer(0);
  }

  if (parts.size() == 1) {
    PPol ret = parts[0];
    flat_free();
    return ret;
  }

  return this;
}

PPol Max :: replace_unknowns(map<int,PPol> &substitution) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->replace_unknowns(substitution);
  return this;
}

PPol Max :: apply_substitution(PolynomialSubstitution &subst) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->apply_substitution(subst);
  return this;
}

bool Max :: query_max() { return true; }

int Max :: number_children() { return parts.size(); }

PPol Max :: get_child(int index) {
  if (index < 0 || index >= parts.size()) return NULL;
  return parts[index];
}

void Max :: add_child(PPol arg) {
  parts.push_back(arg);
}

PPol Max :: replace_child(int pos, PPol replacement) {
  PPol ret = parts[pos];
  parts[pos] = replacement;
  return ret;
}

void Max :: flat_free() {
  parts.clear();
  delete this;
}

void Max :: vars(vector<Polvar*> initial) {
  for (int i = 0; i < parts.size(); i++)
    parts[i]->vars(initial);
}

Sum :: Sum() {}

Sum :: Sum(PPol left, PPol right) {
  parts.push_back(left);
  parts.push_back(right);
}

Sum :: Sum(PPol part) {
  parts.push_back(part);
}

Sum :: Sum(vector<PPol> p) :parts(p) {}

Sum :: ~Sum() {
  for (int i = 0; i < parts.size(); i++) delete parts[i];
}

int Sum :: query_type() { return 10; }

int Sum :: compare(PPol other) {
  if (!other->query_sum())
    return cmp(query_type(), other->query_type());
  Sum *ot = dynamic_cast<Sum*>(other);
  for (int i = 0; i < parts.size(); i++) {
    if (i >= ot->parts.size()) return 1;
    int ret = parts[i]->compare(ot->parts[i]);
    if (ret != 0) return ret;
  }
  if (parts.size() < ot->parts.size()) return -1;
  else return 0;
}

string Sum :: to_string(bool brackets) {
  string ret;
  for (int i = 0; i < parts.size(); i++) {
    if (i != 0) ret += " + ";
    ret += parts[i]->to_string();
  }
  if (brackets) ret = "(" + ret + ")";
  else if (parts.size() <= 1) ret = "[" + ret + "]";
  return ret;
}

string Sum :: to_string(map<int,int> &frename, map<int,int> &brename,
                        bool brackets) {
  string ret;
  for (int i = 0; i < parts.size(); i++) {
    if (i != 0) ret += " + ";
    ret += parts[i]->to_string(frename, brename, false);
  }
  if (brackets) ret = "(" + ret + ")";
  else if (parts.size() <= 1) ret = "[" + ret + "]";
  return ret;
}

PPol Sum :: copy() {
  vector<PPol> parts2;
  for (int i = 0; i < parts.size(); i++)
    parts2.push_back(parts[i]->copy());
  return new Sum(parts2);
}

PPol Sum :: simplify() {
  int i;

  for (i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->simplify();

  // "flatten" nested sums
  for (i = 0; i < parts.size(); i++) {
    if (parts[i]->query_sum()) {
      Sum *pi = dynamic_cast<Sum*>(parts[i]);
      for (int k = 0; k < pi->number_children(); k++)
        parts.push_back(pi->get_child(k));
      pi->flat_free();
      parts[i] = parts[parts.size()-1];
      parts.pop_back();
      i--;
      continue;
    }
  }

  // sort the parts
  sort(parts);

  // merge integers, remove 0
  if (parts.size() > 0 && parts[0]->query_integer()) {
    Integer *p0 = dynamic_cast<Integer*>(parts[0]);
    int value = p0->query_value();
    for (i = 1; i < parts.size() && parts[i]->query_integer(); i++) {
      value += dynamic_cast<Integer*>(parts[i])->query_value();
      delete parts[i];
    }
    int start = 1;
    if (value == 0) {
      delete parts[0];
      start = 0;
    }
    else if (value != p0->query_value()) {
      delete parts[0];
      parts[0] = new Integer(value);
    }
    int n = i;
    for (; i < parts.size(); i++, start++) {
      parts[start] = parts[i];
    }
    parts.resize(start);
  }

  // merge similar products, so i*a + j*a with i, j integers
  bool changed = true, anychanges = false;
  while (changed) {
    changed = false;
    for (i = 0; i < parts.size(); i++) {
      for (int j = i+1; j < parts.size(); j++) {
        int a, b;
        if (parts[i]->query_similar(parts[j], a, b)) {
          int c = a+b;
          if (a == 1) parts[i] = new Product(new Integer(c), parts[i]);
          else {
            Product *p = dynamic_cast<Product*>(parts[i]);
            dynamic_cast<Integer*>(p->get_child(0))->set_value(c);
          }
          delete parts[j];
          parts[j] = parts[parts.size()-1];
          parts.pop_back();
          j--;
          changed = true;
        }
      }
    }
    anychanges |= changed;
  }

  // changes might have caused the order to be messed up
  if (anychanges) sort(parts);

  if (parts.size() == 0) {
    delete this;
    return new Integer(0);
  }

  if (parts.size() == 1) {
    PPol ret = parts[0];
    flat_free();
    return ret;
  }

  return this;
}

PPol Sum :: replace_unknowns(map<int,PPol> &substitution) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->replace_unknowns(substitution);
  return this;
}

PPol Sum :: apply_substitution(PolynomialSubstitution &subst) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->apply_substitution(subst);
  return this;
}

bool Sum :: query_sum() { return true; }

int Sum :: number_children() { return parts.size(); }

PPol Sum :: get_child(int index) {
  if (index < 0 || index >= parts.size()) return NULL;
  return parts[index];
}

void Sum :: add_child(PPol arg) {
  parts.push_back(arg);
}

PPol Sum :: replace_child(int pos, PPol replacement) {
  PPol ret = parts[pos];
  parts[pos] = replacement;
  return ret;
}

void Sum :: flat_free() {
  parts.clear();
  delete this;
}

void Sum :: vars(vector<Polvar*> initial) {
  for (int i = 0; i < parts.size(); i++)
    parts[i]->vars(initial);
}

Product :: Product() {}

Product :: Product(PPol left, PPol right) {
  parts.push_back(left);
  parts.push_back(right);
}

Product :: Product(vector<PPol> p) :parts(p) {}

Product :: ~Product() {
  for (int i = 0; i < parts.size(); i++) delete parts[i];
}

int Product :: query_type() {
  int ret = -1;
  for (int i = 0; i < parts.size(); i++) {
    int k = parts[i]->query_type();
    if (parts[i]->query_product()) k--;
    if (k >= ret) ret = k+1;
  }
  return ret;
}

int Product :: compare(PPol other) {
  int ret = cmp(query_type(), other->query_type());
  if (ret != 0) return ret;

  Product *ot = dynamic_cast<Product*>(other);
  for (int i = 0; i < parts.size(); i++) {
    if (i >= ot->parts.size()) return 1;
    ret = parts[i]->compare(ot->parts[i]);
    if (ret != 0) return ret;
  }
  if (parts.size() < ot->parts.size()) return -1;
  else return 0;
}

bool Product :: query_similar(PPol other, int &a, int &b) {
  // this product is assumed to be simplified!
  if (parts.size() < 2) return false;

  // if the other is not a product, this must be i*other
  if (!other->query_product()) {
    if (parts.size() != 2 || !parts[0]->query_integer()) return false;
    if (!parts[1]->equals(other)) return false;
    a = dynamic_cast<Integer*>(parts[0])->query_value();
    b = 1;
    return true;
  }

  Product *ot = dynamic_cast<Product*>(other);
  if (!(ot->parts.size() >= 2)) return false;  // not simplified

  // if neither starts with an integer, they must be equal
  if (!parts[0]->query_integer() &&
      !ot->parts[0]->query_integer()) {
    if (equals(other)) { a = 1; b = 1; return true; }
    return false;
  }

  // if only the other one starts with an integer, let him solve it
  if (!parts[0]->query_integer()) return ot->query_similar(this, b, a);

  // check whether all the parts are equal
  int i = 1, j = 0;
  if (ot->parts[0]->query_integer()) j++;
  if (parts.size() - i != ot->parts.size() - j) return false;
  for ( ; i < parts.size(); i++, j++)
    if (!parts[i]->equals(ot->parts[j])) return false;

  // figure out the values of a and b
  a = dynamic_cast<Integer*>(parts[0])->query_value();
  if (ot->parts[0]->query_integer())
    b = dynamic_cast<Integer*>(ot->parts[0])->query_value();
  else b = 1;

  return true;
}

string Product :: to_string(bool brackets) {
  string ret;
  for (int i = 0; i < parts.size(); i++) {
    if (parts[i]->query_integer() && i != 0) ret += "*";
    ret += parts[i]->to_string(true);
  }
  if (parts.size() <= 1) ret = "{" + ret + "}";
  return ret;
}

string Product :: to_string(map<int,int> &fr, map<int,int> &br,
                            bool brackets) {
  string ret;
  for (int i = 0; i < parts.size(); i++) {
    if (parts[i]->query_integer() && i != 0) ret += "*";
    ret += parts[i]->to_string(fr, br, true);
  }
  if (parts.size() <= 1) ret = "{" + ret + "}";
  return ret;
}

PPol Product :: copy() {
  vector<PPol> parts2;
  for (int i = 0; i < parts.size(); i++)
    parts2.push_back(parts[i]->copy());
  return new Product(parts2);
}

void Product :: flatten_nested() {
  for (int i = 0; i < parts.size(); i++) {
    if (parts[i]->query_product()) {
      Product *pi = dynamic_cast<Product*>(parts[i]);
      for (int k = 0; k < pi->number_children(); k++)
        parts.push_back(pi->get_child(k));
      pi->flat_free();
      parts[i] = parts[parts.size()-1];
      parts.pop_back();
      i--;
      continue;
    }
  }
}

// deal with children of the form a + b (helping function for simplify)
PPol Product :: sum_children() {
  vector<Sum*> sumparts;
  int i, j, k;

  // split into parts and sumparts
  for (i = 0, j = 0; i < parts.size(); i++) {
    parts[j] = parts[i];
    if (parts[i]->query_sum() && parts[i]->number_children() > 0)
      sumparts.push_back(dynamic_cast<Sum*>(parts[i]));
    else j++;
  }
  parts.resize(j);

  // there are no such children
  if (sumparts.size() == 0) return this;

  // calculate all parts of eg a(b + c)(d + e + f)(g + h) and save
  // them in p
  vector<PPol> p;
  for (j = 0; j < sumparts[0]->number_children(); j++)
    p.push_back(sumparts[0]->get_child(j));
  sumparts[0]->flat_free();

  for (i = 1; i < sumparts.size(); i++) {
    int N = p.size();
    for (j = 0; j < sumparts[i]->number_children(); j++) {
      PPol child = sumparts[i]->get_child(j);
      for (int k = 0; k < N; k++) {
        if (k > 0) child = child->copy();
        if (j+1 == sumparts[i]->number_children()) p[k] = new Product(p[k], child);
        else p.push_back(new Product(p[k]->copy(), child));
      }
    }
    sumparts[i]->flat_free();
  }

  if (parts.size() > 0) {
    for (i = 0; i < p.size(); i++)
      p[i] = new Product(p[i], i == 0 ? this : copy());
  }
  else flat_free();

  Sum *ret = new Sum();
  for (i = 0; i < p.size(); i++) ret->add_child(p[i]);
  return ret;
}

PPol Product :: simplify() {
  int i;

  for (i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->simplify();

  // "flatten" nested products
  flatten_nested();

  // sort the children
  sort(parts);

  // merge integers, deal with 1
  if (parts.size() > 0 && parts[0]->query_integer()) {
    Integer *p0 = dynamic_cast<Integer*>(parts[0]);
    int value = p0->query_value();
    for (i = 1; i < parts.size() && parts[i]->query_integer(); i++) {
      value *= dynamic_cast<Integer*>(parts[i])->query_value();
      delete parts[i];
    }
    int start = 1;
    if (value == 1) {
      delete parts[0];
      start = 0;
    }
    else if (value != p0->query_value()) {
      p0->set_value(value);
    }
    if (i != start) {
      for (; i < parts.size(); i++, start++) {
        parts[start] = parts[i];
      }
      parts.resize(start);
    }
  }

  // deal with 0
  if (parts.size() > 0 && parts[0]->query_integer() &&
      dynamic_cast<Integer*>(parts[0])->query_value() == 0) {
    delete this;
    return new Integer(0);
  }

  // deal with sums
  PPol tmp = sum_children();
  if (tmp != this) return tmp->simplify();

  // deal with max
  /*
  if (parts.size() > 1 && parts[parts.size()-1]->query_max()) {
    Max *a = dynamic_cast<Max*>(parts[parts.size()-1]);
    Product *b = new Product();
    for (i = 0; i < parts.size()-1; i++) b->add_child(parts[i]);
    parts.clear();

    Max *replace = new Max();
    for (i = 0; i < a->number_children(); i++) {
      replace->add_child(new Product(a->get_child(i), b->copy()));
    }
    a->flat_free();
    delete b;
    flat_free();
    return replace->simplify();
  }
  */

  if (parts.size() == 0) {
    delete this;
    return new Integer(1);
  }

  if (parts.size() == 1) {
    PPol ret = parts[0];
    flat_free();
    return ret;
  }

  return this;
}

PPol Product :: replace_unknowns(map<int,PPol> &substitution) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->replace_unknowns(substitution);
  return this;
}

PPol Product :: apply_substitution(PolynomialSubstitution &subst) {
  for (int i = 0; i < parts.size(); i++)
    parts[i] = parts[i]->apply_substitution(subst);
  return this;
}

bool Product :: query_product() {
  return true;
}

int Product :: number_children() { return parts.size(); }

PPol Product :: get_child(int index) {
  if (index < 0 || index >= parts.size()) return NULL;
  return parts[index];
}

void Product :: add_child(PPol arg) {
  parts.push_back(arg);
}

PPol Product :: replace_child(int pos, PPol replacement) {
  PPol ret = parts[pos];
  parts[pos] = replacement;
  return ret;
}

void Product :: flat_free() {
  parts.clear();
  delete this;
}

void Product :: vars(vector<Polvar*> initial) {
  for (int i = 0; i < parts.size(); i++)
    parts[i]->vars(initial);
}

PolynomialFunction :: PolynomialFunction(vector<int> vars,
                                         vector<PType> types,
                                         PPol main) {
  variables = vars;
  variable_types = types;
  p = main;
  calculate_type();
}

PolynomialFunction :: PolynomialFunction(vector<int> vars,
                                         PPol main) {
  variables = vars;
  for (int i = 0; i < vars.size(); i++)
    variable_types.push_back(new DataType("N"));
  p = main;
  calculate_type();
}

PolynomialFunction :: ~PolynomialFunction() {
  delete mytype;
}

PolynomialFunction *PolynomialFunction :: copy() {
  vector<PType> types;
  for (int i = 0; i < variable_types.size(); i++)
    types.push_back(variable_types[i]->copy());
  return new PolynomialFunction(variables, types, p->copy());
}

void PolynomialFunction :: calculate_type() {
  mytype = new DataType("N");
  for (int i = variable_types.size()-1; i >= 0; i--)
    mytype = new ComposedType(variable_types[i]->copy(), mytype);
}

string PolynomialFunction :: to_string() {
  if (variables.size() == 0) return p->to_string();
  string ret = "\\";
  for (int i = 0; i < variables.size(); i++) {
    char tmp[10];
    sprintf(tmp, "%c%d",
      variable_types[i]->query_data() ? 'x' : 'F',
      variables[i]);
    ret += string(tmp);
  }
  return ret + "." + p->to_string();
}

string PolynomialFunction :: to_string(map<int,int> &freerename,
                                       map<int,int> &boundrename) {
  if (variables.size() == 0)
    return p->to_string(freerename, boundrename);
  
  string ret = "\\";
  int i;

  for (i = 0; i < variables.size(); i++) {
    int k = boundrename.size();
    boundrename[variables[i]] = k;
    char tmp[10];
    sprintf(tmp, "%c%d",
      variable_types[i]->query_data() ? 'y' : 'G', k);
    ret += string(tmp);
  }

  ret += "." + p->to_string(freerename, boundrename);

  for (i = 0; i < variables.size(); i++) {
    boundrename.erase(variables[i]);
  }

  return ret;
}

PType PolynomialFunction :: get_function_type() {
  return mytype;
}

PPol PolynomialFunction :: apply(vector<PolynomialFunction*> args) {
  PolynomialSubstitution substitution;
  for (int i = 0; i < variables.size(); i++)
    substitution[variables[i]] = args[i];
  return p->copy()->apply_substitution(substitution);
}

void PolynomialFunction :: replace_unknowns(map<int,PPol> &subst) {
  p = p->replace_unknowns(subst)->simplify();
}

PPol PolynomialFunction :: get_polynomial() {
  return p;
}

void PolynomialFunction :: replace_polynomial(PPol new_p) {
  p = new_p;
}

int PolynomialFunction :: num_variables() {
  return variables.size();
}

int PolynomialFunction :: variable_index(int num) {
  return variables[num];
}

PType PolynomialFunction :: variable_type(int num) {
  return variable_types[num];
}

void PolynomialFunction :: vars(vector<Polvar*> initial) {
  int start = initial.size();
  p->vars(initial);

  // remove bound variables!
  for (int j = start; j < initial.size(); j++) {
    for (int k = 0; k < variables.size(); k++) {
      if (initial[j]->query_index() == variables[k]) {
        initial[j] = initial[initial.size()-1];
        initial.pop_back();
        j--;
        break;
      }
    }
  }
}

int last_polvar_index = 0;
int unused_polvar_index() {
  return last_polvar_index++;
}

