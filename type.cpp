/**************************************************************************
   Copyright 2012 Cynthia Kop

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

#include "type.h"
#include "typesubstitution.h"

/* As this is virtual and all the child classes are required to
 * overwrite it, this should never be called if the classes are used
 * correctly.
 * It retuns NULL rather than this so the error will occur at the
 * point where it is used, rather than at a completely different
 * place in the program where the original type is freed and the copy
 * used.
 */
PType Type :: copy() {
  return NULL;
}

/* Should also never be called. */
string Type :: to_string(bool brackets) {
  return "ERR";
}

string Type :: to_string(TypeNaming &naming, bool brackets) {
  return "ERR";
}

bool Type :: equals(PType other) {
  return to_string() == other->to_string();
}

bool Type :: query_data() {
  return false;
}

bool Type :: query_composed() {
  return false;
}

bool Type :: query_typevar() {
  return false;
}

PType Type :: query_child(int index) {
  return NULL;
}

PType Type :: collapse() {
  return this;
}

PType Type :: substitute(TypeSubstitution &theta) {
  return this;
}

bool Type :: instantiate(PType tau, TypeSubstitution &theta) {
  return false;   // shouldn't be called here!
}

Varset Type :: vars() {
  return Varset();
}

DataType :: DataType(string _name) :constructor(_name) {}

DataType :: DataType(string _name, vector<PType> _children)
  :constructor(_name), children(_children) {}

DataType :: ~DataType() {
  for (int i = 0; i < children.size(); i++) delete children[i];
}

PType DataType :: copy() {
  vector<PType> newchildren;
  for (int i = 0; i < children.size(); i++)
    newchildren.push_back(children[i]->copy());
  return new DataType(constructor, newchildren);
}

string DataType :: to_string(TypeNaming &naming, bool brackets) {
  string ret = constructor;
  if (children.size() == 0) return ret;
  ret += "(";
  for (int i = 0; i < children.size(); i++) {
    ret += children[i]->to_string(naming, false);
    if (i != children.size()-1) ret += ",";
    else ret += ")";
  }
  return ret;
}

string DataType :: to_string(bool brackets) {
  string ret = constructor;
  if (children.size() == 0) return ret;
  ret += "(";
  for (int i = 0; i < children.size(); i++) {
    ret += children[i]->to_string(false);
    if (i != children.size()-1) ret += ",";
    else ret += ")";
  }
  return ret;
}

bool DataType :: equals(PType type) {
  if (!type->query_data()) return false;
  DataType* data = dynamic_cast<DataType*>(type);
  if (data->constructor != constructor) return false;
  if (data->children.size() != children.size()) return false;
  for (int i = 0; i < children.size(); i++)
    if (!children[i]->equals(data->children[i])) return false;
  return true;
}

bool DataType :: query_data() {
  return true;
}

PType DataType :: query_child(int index) {
  if (index < 0 || index >= children.size()) return NULL;
  return children[index];
}

PType DataType :: collapse() {
  return new DataType("o");
}

PType DataType :: substitute(TypeSubstitution &theta) {
  for (int i = 0; i < children.size(); i++)
    children[i] = children[i]->substitute(theta);
  return this;
}

bool DataType :: instantiate(PType tau, TypeSubstitution &theta) {
  if (!tau->query_data()) return false;
  DataType *other = dynamic_cast<DataType*>(tau);
  if (other->query_constructor() != constructor) return false;
  for (int i = 0; i < children.size(); i++) {
    PType ci = other->query_child(i);
    if (ci == NULL) return false;
    if (!children[i]->instantiate(ci, theta)) return false;
  }
  if (other->query_child(children.size())) return false;

  return true;
}

Varset DataType :: vars() {
  if (children.size() == 0) return Varset();
  if (children.size() == 1) return children[0]->vars();
  Varset ret = children[0]->vars();
  for (int i = 1; i < children.size(); i++) {
    Varset c = children[i]->vars();
    ret.add(c);
  }
  return ret;
}

string DataType :: query_constructor() {
  return constructor;
}

ComposedType :: ComposedType(PType _left, PType _right)
  : left(_left), right(_right) {}

ComposedType :: ~ComposedType() {
  delete left;
  delete right;
}

PType ComposedType :: copy() {
  return new ComposedType(left->copy(), right->copy());
}

string ComposedType :: to_string(TypeNaming &naming, bool brackets) {
  string l = left->to_string(naming, true);
  string r = right->to_string(naming, false);
  string middle = l + " -> " + r;
  if (brackets) return "(" + middle + ")";
  else return middle;
}

string ComposedType :: to_string(bool brackets) {
  string middle = left->to_string(true) + " -> " +
                  right->to_string(false);
  if (brackets) return "(" + middle + ")";
  else return middle;
}

bool ComposedType :: equals(PType other) {
  if (!other->query_composed()) return false;
  return left->equals(other->query_child(0)) &&
         right->equals(other->query_child(1));
}

bool ComposedType :: query_composed() {
  return true;
}

PType ComposedType :: query_child(int index) {
  if (index == 0) return left;
  if (index == 1) return right;
  return NULL;
}

PType ComposedType :: collapse() {
  return new ComposedType(left->collapse(), right->collapse());
}

PType ComposedType :: substitute(TypeSubstitution &theta) {
  left = left->substitute(theta);
  right = right->substitute(theta);
  return this;
}

bool ComposedType :: instantiate(PType tau, TypeSubstitution &theta) {
  if (!tau->query_composed()) return false;
  if (tau->query_child(0) == NULL || tau->query_child(1) == NULL ||
      tau->query_child(2) != NULL) return false;
  if (!left->instantiate(tau->query_child(0), theta)) return false;
  if (!right->instantiate(tau->query_child(1), theta)) return false;

  return true;
}

Varset ComposedType :: vars() {
  Varset ret1 = left->vars();
  Varset ret2 = right->vars();
  if (ret1.size() > ret2.size()) {
    ret1.add(ret2);
    return ret1;
  }
  else {
    ret2.add(ret1);
    return ret2;
  }
}

/**
 * To be able to create fresh variables we keep track of the next
 * index which has not yet been used.
 */
static long nextvarindex = 0;

TypeVariable :: TypeVariable(int _index) {
  if (_index == FRESHTYPEVAR) {
    index = nextvarindex;
    nextvarindex++;
  }
  else {
    index = _index;
    if (index > nextvarindex) nextvarindex = index+1;
  }
}

TypeVariable :: TypeVariable(TypeVariable* alpha) {
  index = alpha->query_index();
}

PType TypeVariable :: copy() {
  return new TypeVariable(index);
}

string TypeVariable :: to_string(TypeNaming &naming, bool brackets) {
  if (naming.find(index) != naming.end()) return naming[index];
  int k = naming.size();
  char name[20];
  name[0] = '$';
  int i = 1;
  while (true) {
    name[i] = 'a'+(k%26);
    if (k < 26) {name[i+1] = '\0'; break;}
    k = k/26-1;
    i++;
  }
  naming[index] = string(name);
  return naming[index];
}

string TypeVariable :: to_string(bool brackets) {
  int k = index;
  char name[20];
  name[0] = '$';
  int i = 1;
  while (true) {
    name[i] = 'a'+(k%26);
    if (k < 26) {name[i+1] = '\0'; break;}
    k = k/26-1;
    i++;
  }
  return string(name);
}

bool TypeVariable :: equals(PType type) {
  if (!type->query_typevar()) return false;
  return dynamic_cast<TypeVariable*>(type)->index == index;
}

bool TypeVariable :: query_typevar() {
  return true;
}

PType TypeVariable :: substitute(TypeSubstitution &theta) {
  PType ret = theta[index];
  if (ret != NULL) {
    ret = ret->copy();
    delete this;
    return ret;
  }
  return this;
}

bool TypeVariable :: instantiate(PType tau, TypeSubstitution &theta) {
  if (theta[index] == NULL) {
    theta[index] = tau->copy();
    return true;
  }
  if (theta[index]->equals(tau)) return true;
  return false;
}

Varset TypeVariable :: vars() {
  return Varset(index);
}

int TypeVariable :: query_index() {
  return index;
}

