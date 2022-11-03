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

#include "term.h"
#include "environment.h"
#include "substitution.h"

/* ========== TERM IMPLEMENTATION ========== */

Term :: Term() : type(NULL) {}
Term :: Term(PType _type) : type(_type) {}

Term :: ~Term() {
  if (type != NULL) delete type;
}

PType Term :: query_type() {
  return type;
}

bool Term :: query_constant() {
  return false;
}

bool Term :: query_variable() {
  return false;
}

bool Term :: query_abstraction() {
  return false;
}

bool Term :: query_application() {
  return false;
}

bool Term :: query_meta() {
  return false;
}

bool Term :: query_special(string description) {
  return false;
}

void Term :: apply_type_substitution(TypeSubstitution &s) {
  type = type->substitute(s);
}

PTerm Term :: apply_substitution(Substitution &subst) {
  return this;
}

bool Term :: instantiate(PTerm term, TypeSubstitution &theta,
                         Substitution &gamma) {
  Renaming bound;
  return instantiate(term, theta, gamma, bound);
}

PTerm Term :: subterm(string position) {
  if (position.length() == 0) return this;
  else return NULL;
}

PTerm Term :: replace_subterm(PTerm subterm, string position) {
  return NULL;      // should not be called in the inherit!
}

int Term :: number_children() {
  return 0;
}

PTerm Term :: get_child(int index) {
  return NULL;
}

PTerm Term :: replace_child(int index, PTerm newchild) {
  return NULL;
}

Varset Term :: free_var(bool metavars) {
  return Varset();
}

Varset Term :: free_typevar() {
  return type->vars();
}

PTerm Term :: query_head() {
  return this;
}

vector<string> Term :: query_positions(string startwith) {
  vector<string> ret;
  ret.push_back(startwith);
  return ret;
}

vector<PTerm> Term :: split() {
  vector<PTerm> ret;
  ret.push_back(this);
  return ret;
}

PType Term :: lookup_type(int v) {
  return NULL;
}

bool Term :: query_pattern() {
  return true;
}

void Term :: adjust_arities(map<string,int> &arities) {
  for (int i = 0; i < number_children(); i++) {
    get_child(i)->adjust_arities(arities);
  }
}

string Term :: to_string(bool annotated, bool addtype) {
  Environment env;
  return to_string(env, annotated, addtype);
}

string Term :: to_string(Environment &env, bool annotated,
                         bool addtype) {
  TypeNaming tenv;
  int binders = 0, freevars = 0;
  string ret = to_string_recursive(env, tenv, binders, freevars,
                                   false, annotated);
  if (addtype) return ret + " : " + type->to_string(tenv);
  else return ret;
}

string Term :: to_string(Environment &env, TypeNaming &tenv) {
  int binders = 0, freevars = 0;
  return to_string_recursive(env, tenv, binders, freevars,
                             false, true);
}

PTerm Term :: copy() {
  Renaming boundrename;
  return copy_recursive(boundrename);
}

bool Term :: equals(PTerm other) {
  Renaming boundrename;
  return equals_recursive(other, boundrename);
}


PTerm Term :: copy_recursive(Renaming &boundrename) {
  return this;    // should probably not be called here
}

bool Term :: equals_recursive(PTerm other, Renaming &boundrename) {
  return this == other;   // should probably not be called here
}

string Term :: to_string_recursive(Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  return "[ERR]";   // should not be called here!
}

bool Term :: instantiate(PTerm term, TypeSubstitution &theta,
                         Substitution &gamma, Renaming &bound) {
  return false;   // should not be called here!
}

string Term :: pretty_name(int index, bool bound) {
  string bnames[] = { "x","y","z","u","v","w","p","q","r","s",
                      "t","a","b" };
  string fnames[] = { "X","Y","Z","U","V","W","F","G","H","I",
                      "J","P","Q","R","S","T" };
  
  string *names = bound ? bnames : fnames;
  int numnames = bound ? 13 : 16;
  
  string ret = "%" + names[index % numnames];
  index /= numnames;
  while (index != 0) {
    ret += string(1, 'a' + (index % 26));
    index /= 26;
  }
  return ret;
}

/* ========== CONSTANT IMPLEMENTATION ========== */

Constant :: Constant(string _name, PType _type) :Term(_type) {
  name = _name;
  // determine arity
  possargs = 0;
  for (PType tmp = _type; tmp->query_composed();
                          tmp = tmp->query_child(1)) possargs++;

  typestring = _type->to_string();
}

Constant :: Constant(PConstant f) :Term(f->type->copy()) {
  name = f->name;
  possargs = f->possargs;
  typestring = f->typestring;
}

bool Constant :: query_constant() {
  return true;
}

int Constant :: query_max_arity() {
  return possargs;
}

string Constant :: query_name() {
  return name;
}

void Constant :: rename(string newname) {
  name = newname;
}

void Constant :: adjust_arities(map<string,int> &arities) {
  arities[name] = 0;
}

void Constant :: apply_type_substitution(TypeSubstitution &s) {
  type = type->substitute(s);
  typestring = type->to_string();
}

PTerm Constant :: copy_recursive(Renaming &boundrename) {
  return new Constant(this);
}

bool Constant :: equals_recursive(PTerm other, Renaming &rename) {
  if (!other->query_constant()) return false;
  PConstant c = dynamic_cast<Constant*>(other);
  return c->name == name && c->typestring == typestring;
}

string Constant :: to_string_recursive(
                           Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  if (annotated) return name + "_{" + type->to_string(tenv) + "}";
  else return name;
}

bool Constant :: instantiate(PTerm term, TypeSubstitution &theta,
                             Substitution &gamma, Renaming &bound) {
  if (!term->query_constant()) return false;
  PConstant f = dynamic_cast<PConstant>(term);
  if (f->name != name) return false;
  return type->instantiate(f->type, theta);
}

/* ========== VARIABLE IMPLEMENTATION ========== */

/**
 * To be able to create fresh variables we keep track of the next
 * index which has not yet been used.
 */
static long nextvarindex = 0;

Variable :: Variable(PType _type, long _index) :Term(_type) {
  if (_index == FRESHVAR) {
    index = nextvarindex;
    nextvarindex++;
  }
  else {
    index = _index;
    if (index >= nextvarindex) nextvarindex = index+1;
  }
  typestring = _type->to_string();
}

Variable :: Variable(PVariable var) :Term(var->type->copy()) {
  index = var->index;
  typestring = var->typestring;
}

bool Variable :: query_variable() {
  return true;
}

long Variable :: query_index() {
  return index;
}

Varset Variable :: free_var(bool metavars) {
  if (metavars) return Varset();
  else return Varset(index);
}

PType Variable :: lookup_type(int v) {
  if (v == index) return type;
  return NULL;
}

void Variable :: apply_type_substitution(TypeSubstitution &s) {
  type = type->substitute(s);
  typestring = type->to_string();
}

PTerm Variable :: apply_substitution(Substitution &subst) {
  if (subst.contains(index)) {
    PTerm ret = subst[index]->copy();
    delete this;
    return ret;
  }
  else return this;
}

PTerm Variable :: copy_recursive(Renaming &boundrename) {
  PVariable v = new Variable(this);
  Renaming::iterator it = boundrename.find(index);
  if (it != boundrename.end()) v->index = it->second;
  return v;
}

bool Variable :: equals_recursive(PTerm other, Renaming &rename) {
  if (!other->query_variable()) return false;
  int id = dynamic_cast<PVariable>(other)->query_index();
  Renaming::iterator it = rename.find(index);
  if (it == rename.end()) return index == id;
  else return it->second == id;
}

string Variable :: to_string_recursive(
                           Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  string ret = env.get_name(index);
  if (ret == "") {
    while (ret == "" || env.lookup(ret) != -1) {
      ret = pretty_name(unknownfree, false);
      unknownfree++;
    }
    env.add(index, ret);
  }
  if (annotated) return ret + "_{" + type->to_string(tenv) + "}";
  else return ret;
}

bool Variable :: instantiate(PTerm term, TypeSubstitution &theta,
                             Substitution &gamma, Renaming &bound) {
  if (bound.find(index) != bound.end()) {
    if (!term->query_variable()) return false;
    PVariable x = dynamic_cast<PVariable>(term);
    if (x->index != bound[index]) return false;
    return type->instantiate(x->type, theta);
  }
  
  if (gamma.contains(index)) {
    return gamma[index]->equals(term);
  }
  
  if (!type->instantiate(term->query_type(), theta)) return false;
  
  // can't instantiate something which contains bound variables
  Varset FV = term->free_var();
  for (Renaming::iterator it = bound.begin(); it != bound.end(); it++) {
    if (FV.contains(it->second)) return false;
  }
  
  gamma[index] = term->copy();
  return true;
}

/* ========== APPLICATION IMPLEMENTATION ========== */

Application :: Application(PTerm _left, PTerm _right)
    :Term(_left->query_type()->query_child(1)->copy()),
     left(_left), right(_right) {
}

Application :: ~Application() {
  if (left != NULL) delete left;
  if (right != NULL) delete right;
}

bool Application :: query_application() {
  return true;
}

void Application :: apply_type_substitution(TypeSubstitution &s) {
  left->apply_type_substitution(s);
  right->apply_type_substitution(s);
  type = type->substitute(s);
}

PTerm Application :: apply_substitution(Substitution &subst) {
  left = left->apply_substitution(subst);
  right = right->apply_substitution(subst);
  return this;
}

PTerm Application :: subterm(string position) {
  if (position.length() == 0) return this;
  if (position[0] == '1') return left->subterm(position.substr(1));
  if (position[0] == '2') return right->subterm(position.substr(1));
  return NULL;
}

PTerm Application :: replace_subterm(PTerm subterm, string position) {
  if (position.length() == 0) return NULL;
  if (position[0] != '1' && position[0] != '2') return NULL;
  if (position.length() == 1) {
    PTerm ret = (position[0] == '1') ? left : right;
    if (position[0] == '1') left = subterm;
    else right = subterm;
    return ret;
  }
  if (position[0] == '1')
    return left->replace_subterm(subterm, position.substr(1));
  else
    return right->replace_subterm(subterm, position.substr(1));
}

int Application :: number_children() {
  return 2;
}

PTerm Application :: get_child(int index) {
  if (index == 0) return left;
  if (index == 1) return right;
  return NULL;
}

PTerm Application :: replace_child(int index, PTerm newchild) {
  PTerm ret = get_child(index);
  if (index == 0) left = newchild;
  else if (index == 1) right = newchild;
  else return NULL;
  return ret;
}

Varset Application :: free_var(bool metavars) {
  Varset ret1 = left->free_var(metavars);
  Varset ret2 = right->free_var(metavars);
  if (ret1.size() > ret2.size()) {
    ret1.add(ret2);
    return ret1;
  }
  else {
    ret2.add(ret1);
    return ret2;
  }
}

Varset Application :: free_typevar() {
  Varset ret1 = left->free_typevar();
  Varset ret2 = right->free_typevar();
  if (ret1.size() > ret2.size()) {
    ret1.add(ret2);
    return ret1;
  }
  else {
    ret2.add(ret1);
    return ret2;
  }
}

PTerm Application :: query_head() {
  return left->query_head();
}

vector<string> Application :: query_positions(string startwith) {
  vector<string> l = left->query_positions(startwith + "1");
  vector<string> r = right->query_positions(startwith + "2");
  l.insert(l.end(), r.begin(), r.end());
  l.push_back(startwith);
  return l;
}

vector<PTerm> Application :: split() {
  vector<PTerm> ret = left->split();
  ret.push_back(right);
  return ret;
}

PType Application :: lookup_type(int v) {
  PType ret = left->lookup_type(v);
  if (ret == NULL) return right->lookup_type(v);
  return ret;
}

bool Application :: query_pattern() {
  return !left->query_meta() && left->query_pattern() &&
         right->query_pattern();
}

void Application :: adjust_arities(map<string,int> &arities) {
  vector<PTerm> parts = split();
  if (parts[0]->query_constant()) {
    string name = dynamic_cast<PConstant>(parts[0])->query_name();
    int num = parts.size()-1;
    if (arities.find(name) == arities.end() || arities[name] > num)
      arities[name] = num;
  }
  for (int i = 1; i < parts.size(); i++)
    parts[i]->adjust_arities(arities);
}

PTerm Application :: copy_recursive(Renaming &boundrename) {
  return new Application(left->copy_recursive(boundrename),
                         right->copy_recursive(boundrename));
}

bool Application :: equals_recursive(PTerm other, Renaming &rename) {
  if (!other->query_application()) return false;
  Application *ot = dynamic_cast<Application*>(other);
  return left->equals_recursive(ot->left, rename) &&
         right->equals_recursive(ot->right, rename);
}

string Application :: to_string_recursive(
                           Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  string txt = left->to_string_recursive(env, tenv, bindersencountered,
               unknownfree, !left->query_application(), annotated);
  txt += " " + right->to_string_recursive(env, tenv, bindersencountered,
               unknownfree, true, annotated);
  if (brackets) return "(" + txt + ")";
  else return txt; 

}

bool Application :: instantiate(PTerm term, TypeSubstitution &theta,
                             Substitution &gamma, Renaming &bound) {
  if (!term->query_application()) return false;
  return left->instantiate(term->subterm("1"), theta, gamma, bound) &&
         right->instantiate(term->subterm("2"), theta, gamma, bound);
}

/* ========== ABSTRACTION IMPLEMENTATION ========== */

Abstraction :: Abstraction(PVariable _var, PTerm _term)
    :Term(new ComposedType(_var->query_type()->copy(),
                           _term->query_type()->copy())),
     var(_var), term(_term) {
}

Abstraction :: ~Abstraction() {
  delete var;
  if (term != NULL) delete term;
}

bool Abstraction :: query_abstraction() {
  return true;
}

PVariable Abstraction :: query_abstraction_variable() {
  return var;
}

void Abstraction :: apply_type_substitution(TypeSubstitution &s) {
  var->apply_type_substitution(s);
  term->apply_type_substitution(s);
  type = type->substitute(s);
}

PTerm Abstraction :: apply_substitution(Substitution &subst) {
  if (!subst.contains(var)) term = term->apply_substitution(subst);
    // Else error! But solve the best way we can by not substituting
  return this;
}

PTerm Abstraction :: subterm(string position) {
  if (position.length() == 0) return this;
  if (position[0] == '1') return term->subterm(position.substr(1));
  return NULL;
}

PTerm Abstraction :: replace_subterm(PTerm subterm, string position) {
  if (position.length() == 0 || position[0] != '1') return NULL;
  if (position.length() == 1) {
    PTerm ret = term;
    term = subterm;
    return ret;
  }
  return term->replace_subterm(subterm, position.substr(1));
}

int Abstraction :: number_children() {
  return 1;
}

PTerm Abstraction :: get_child(int index) {
  if (index == 0) return term;
  if (index == -1) return var;
  return NULL;
}

PTerm Abstraction :: replace_child(int index, PTerm newchild) {
  if (index == 0) { PTerm ret = term; term = newchild; return ret; }
  else return NULL;
}

Varset Abstraction :: free_var(bool metavars) {
  Varset ret = term->free_var(metavars);
  if (!metavars) ret.remove(var);
  return ret;
}

Varset Abstraction :: free_typevar() {
  Varset ret1 = var->free_typevar();
  Varset ret2 = term->free_typevar();
  ret2.add(ret1);
  return ret2;
}

vector<string> Abstraction :: query_positions(string startwith) {
  vector<string> ret = term->query_positions(startwith + "1");
  ret.push_back(startwith);
  return ret;
}

PType Abstraction :: lookup_type(int v) {
  return term->lookup_type(v);
}

bool Abstraction :: query_pattern() {
  return term->query_pattern();
}

PTerm Abstraction :: copy_recursive(Renaming &boundrename) {
  int x = var->query_index(), y = nextvarindex;
  boundrename[x] = y;
  nextvarindex++;
  PTerm sub = term->copy_recursive(boundrename);
  PVariable v = new Variable(var->query_type()->copy(), y);
  boundrename.erase(x);
  return new Abstraction(v, sub);
}

bool Abstraction :: equals_recursive(PTerm other,
                                     Renaming &boundrename) {
  if (!other->query_abstraction()) return false;
  Abstraction *ot = dynamic_cast<Abstraction*>(other);
  if (ot->var->typestring != var->typestring) return false;
  boundrename[var->query_index()] = ot->var->query_index();
  bool ret = term->equals_recursive(ot->term, boundrename);
  boundrename.erase(var->query_index());
  return ret;
}

string Abstraction :: to_string_recursive(
                           Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  string sub, middle, dot, varname, typestr;
  
  varname = pretty_name(bindersencountered, true);
  env.add(var, varname);
  bindersencountered++;
  sub = term->to_string_recursive(env, tenv, bindersencountered,
                                  unknownfree, false, annotated);
  env.remove(var);
  
  dot = ". ";
  if (term->query_abstraction()) {
    sub = sub.substr(2);
    dot = ", ";
  }

  if (annotated) typestr = "_{"+var->query_type()->to_string()+"}";
  else typestr = ":" + var->query_type()->to_string();
  middle = "/\\" + varname + typestr + dot + sub;
                  
  if (brackets) return "(" + middle + ")";
  else return middle;
}

bool Abstraction :: instantiate(PTerm with, TypeSubstitution &theta,
                             Substitution &gamma, Renaming &bound) {
  if (!with->query_abstraction()) return false;
  Abstraction *abs = dynamic_cast<Abstraction*>(with);
  if (bound.find(var->query_index()) != bound.end()) return false;
    // shouldn't happen!
  if (!var->query_type()->instantiate(abs->var->query_type(), theta))
    return false;
  
  bound[var->query_index()] = abs->var->query_index();
  bool ret = term->instantiate(abs->term, theta, gamma, bound);
  bound.erase(var->query_index());
  return ret;
}
/* ========== META-APPLICATION IMPLEMENTATION ========== */

MetaApplication :: MetaApplication(PVariable _metavar,
                                   vector<PTerm> &_children) {
  metavar = _metavar;
  children = _children;
  determine_type();
}

MetaApplication :: MetaApplication(PVariable _metavar) {
  metavar = _metavar;
  determine_type();
}

MetaApplication :: ~MetaApplication() {
  delete metavar;
  for (int i = 0; i < children.size(); i++)
    if (children[i] != NULL) delete children[i];
}

void MetaApplication :: determine_type() {
  PType t = metavar->query_type();
  if (children.size() == 0) typestring = t->to_string();
  typestring = "(";
  for (int i = 0; i < children.size(); i++) {
    if (i != 0) typestring += " * ";
    typestring += t->query_child(0)->to_string();
    t = t->query_child(1);
  }
  type = t->copy();
  typestring += ") --> " + t->to_string();
}

bool MetaApplication :: query_meta() {
  return true;
}

PVariable MetaApplication :: get_metavar() {
  return metavar;
}

void MetaApplication :: apply_type_substitution(TypeSubstitution &s) {
  metavar->apply_type_substitution(s);
  delete type;
  determine_type();
  for (int i = 0; i < children.size(); i++)
    children[i]->apply_type_substitution(s);
}

PTerm MetaApplication :: apply_substitution(Substitution &subst) {
  int i;

  for (i = 0; i < children.size(); i++)
    children[i] = children[i]->apply_substitution(subst);
  if (!subst.contains(metavar->query_index())) return this;

  PTerm ret = subst[metavar->query_index()]->copy();
  bool emptysubstitution = true;
  Substitution ss;
  for (i = 0; i < children.size(); i++) {
    if (ret->query_abstraction()) {
      PVariable v =
        dynamic_cast<Abstraction*>(ret)->query_abstraction_variable();
      ss[v] = children[i];
      emptysubstitution = false;
      PTerm r = ret->replace_subterm(NULL, "1");
      delete ret;
      ret = r;
    }
    else {
      ret = new Application(ret, children[i]);
    }
  }
  if (!emptysubstitution) ret = ret->apply_substitution(ss);
  children.clear();   // they're used in gamma or directly in ret
  delete this;
  return ret;
}

PTerm MetaApplication :: subterm(string position) {
  if (position.length() == 0) return this;
  int k = position[0] - '0';
  if (k < 0 || k >= children.size()) return NULL;
  return children[k]->subterm(position.substr(1));
}

PTerm MetaApplication :: replace_subterm(PTerm subterm, string position) {
  if (position.length() == 0) return NULL;
  int k = position[0] - '0';
  if (k < 0 || k >= children.size()) return NULL;
  if (position.length() == 1) {
    PTerm ret = children[k];
    children[k] = subterm;
    return ret;
  }
  return children[k]->replace_subterm(subterm, position.substr(1));
}

Varset MetaApplication :: free_var(bool metavars) {
  Varset ret = (metavars ? Varset(metavar) : Varset());
  for (int i = 0; i < children.size(); i++) {
    Varset FV = children[i]->free_var(metavars);
    ret.add(FV);
  }
  return ret;
}

Varset MetaApplication :: free_typevar() {
  Varset ret = metavar->free_typevar();
  for (int i = 0; i < children.size(); i++) {
    Varset FTV = children[i]->free_typevar();
    ret.add(FTV);
  }
  return ret;
}

vector<string> MetaApplication :: query_positions(string startwith) {
  vector<string> ret;
  for (int i = 0; i < children.size(); i++) {
    vector<string> tmp =
      children[i]->query_positions(startwith+string(1,'0'+i));
    ret.insert(ret.end(), tmp.begin(), tmp.end());
    if (i+'0' == 'z') break;
  }
  ret.push_back(startwith);
  return ret;
}

PTerm MetaApplication :: copy_recursive(Renaming &boundrename) {
  PVariable mv = new Variable(metavar);
  vector<PTerm> ch;
  for (int i = 0; i < children.size(); i++)
    ch.push_back(children[i]->copy_recursive(boundrename));
  return new MetaApplication(mv, ch);
}

bool MetaApplication :: equals_recursive(PTerm other,
                                         Renaming &boundrename) {
  if (!other->query_meta()) return false;
  MetaApplication *ot = dynamic_cast<MetaApplication*>(other);
  if (ot->metavar->query_index() != metavar->query_index()) return false;
  if (ot->typestring != typestring) return false;
  if (ot->children.size() != children.size()) return false;
  for (int i = 0; i < children.size(); i++) {
    if (!children[i]->equals_recursive(ot->children[i], boundrename))
      return false;
  }
  return true;
}

string MetaApplication :: to_string_recursive(
                           Environment &env, TypeNaming &tenv,
                           int &bindersencountered, int &unknownfree,
                           bool brackets, bool annotated) {
  int index = metavar->query_index();
  string ret = env.get_name(index);
  if (ret == "") {
    while (ret == "" || env.lookup(ret) != -1) {
      ret = pretty_name(unknownfree, false);
      unknownfree++;
    }
    env.add(index, ret);
  }
  if (annotated) {
    ret += "_{(";
    PType p = metavar->query_type();
    for (int k = 0; k < children.size(); k++) {
      if (k != 0) ret += " * ";
      ret += p->query_child(0)->to_string(tenv);
      p = p->query_child(1);
    }
    ret += ") --> " + p->to_string(tenv) + "}";
  }
  if (children.size() != 0) {
    ret += "[";
    for (int i = 0; i < children.size(); i++) {
      if (i != 0) ret += ", ";
      ret += children[i]->to_string_recursive(env, tenv,
        bindersencountered, unknownfree, false, annotated);
    }
    ret += "]";
  }
  return ret;
}

PType MetaApplication :: lookup_type(int v) {
  if (metavar->query_index() == v) return metavar->query_type();
  for (int i = 0; i < children.size(); i++) {
    PType ret = children[i]->lookup_type(v);
    if (ret != NULL) return ret;
  }
  return NULL;
}

bool MetaApplication :: query_pattern() {
  // are all children different variables?
  Varset X;
  for (int i = 0; i < children.size(); i++) {
    if (!children[i]->query_variable()) return false;
    X.add(dynamic_cast<PVariable>(children[i]));
  }
  return X.size() == children.size();
}

int MetaApplication :: number_children() {
  return children.size();
}

PTerm MetaApplication :: get_child(int index) {
  if (index == -1) return metavar;
  if (0 <= index && index < children.size()) return children[index];
  return NULL;
}

PTerm MetaApplication :: replace_child(int index, PTerm newchild) {
  if (index < 0 || index >= children.size()) return NULL;
  PTerm ret = children[index];
  children[index] = newchild;
  return ret;
}

bool MetaApplication :: instantiate(PTerm term,
                                    TypeSubstitution &theta,
                                    Substitution &gamma,
                                    Renaming &bound) {
  // are all children different bound variables?
  Varset X;
  for (int i = 0; i < children.size(); i++) {
    if (!children[i]->query_variable()) return false;
    PVariable y = dynamic_cast<PVariable>(children[i]);
    if (bound.find(y->query_index()) == bound.end()) return false;
    X.add(y);
  }
  if (X.size() != children.size()) return false;
  
  // okay, this *is* a pattern, but does term contain only the
  // variables it's allowed to?
  Varset FV = term->free_var();
  for (Renaming::iterator it = bound.begin(); it != bound.end(); it++) {
    if (FV.contains(it->second) && !X.contains(it->first))
      return false;
  }
  
  // check whether the output type is correct
  // (argument types have already been checked separately)
  if (!type->instantiate(term->query_type(), theta)) return false;
  
  // looks all good - determine what gamma[index] should be
  PTerm rep = term->copy();
  for (int i = children.size()-1; i >= 0; i--) {
    PType ctype = children[i]->query_type()->copy()->substitute(theta);
    PVariable newvar = new Variable(ctype);
    Substitution sub;
    int k = bound[dynamic_cast<PVariable>(children[i])->query_index()];
    sub[k] = newvar->copy();
    rep = rep->apply_substitution(sub);
    rep = new Abstraction(newvar, rep);
  }
  
  // in a non-left-linear system, gamma should still have a unique
  // value  
  if (gamma.contains(metavar)) {
    bool ok = gamma[metavar]->equals(rep);
    delete rep;
    return ok;
  }
  gamma[metavar] = rep;
  return true;
}

