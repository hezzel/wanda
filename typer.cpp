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

#include "typer.h"
#include "typesubstitution.h"

PartTypedTerm :: ~PartTypedTerm() {
  for (int i = 0; i < children.size(); i++) {
    if (children[i] != NULL) delete children[i];
  }
  if (type != NULL) delete type;
}

string PartTypedTerm :: to_string(bool brackets) {
  // constants
  if (name == "constant" && type != NULL)
    return sdata + "_{" + type->to_string() + "}";
  if (name == "constant") return sdata;

  // variables
  if (name == "variable") {
    char varname[20];
    varname[0] = '%';
    int i = 1, k = idata;
    while (true) {
      varname[i] = 'a'+(k%26);
      if (k < 26) {varname[i+1] = '\0'; break;}
      k = k/26-1;
      i++;
    }
    if (type == NULL) return string(varname);
    else return string(varname) + "_{" + type->to_string() + "}";
  }

  // abstraction
  if (name == "abstraction") {
    if (children.size() != 2) return "ERR";
    string ret = "/\\" + children[0]->to_string() + "." +
                 children[1]->to_string();
    if (brackets) return "(" + ret + ")";
    else return ret;
  }

  // application
  if (name == "application") {
    string ret;
    if (children.size() != 2) return "ERR";
    if (children[0]->name == "application")
      ret = children[0]->to_string(false);
    else ret = children[0]->to_string(true);
    ret += " " + children[1]->to_string(true);
    if (brackets) ret = "(" + ret + ")";
    return ret;
  }

  // meta-application
  if (name == "meta") {
    string ret = children[0]->to_string() + "[";
    for (int i = 1; i < children.size(); i++) {
      if (i != 1) ret += ",";
      ret += children[i]->to_string();
    }
    return ret + "]";
  }

  return "ERR";
}

PType Typer :: substitute_recursively(PType type,
                                      TypeSubstitution &theta) {
  if (type == NULL) return NULL;

  if (type->query_typevar()) {
    TypeVariable *alpha = dynamic_cast<TypeVariable*>(type);
    if (theta[alpha] == NULL) return type->copy();
    else return substitute_recursively(theta[alpha], theta);
  }

  if (type->query_composed()) {
    return new ComposedType(
         substitute_recursively(type->query_child(0),theta),
         substitute_recursively(type->query_child(1),theta));
  }

  if (type->query_data()) {
    string constructor =
      dynamic_cast<DataType*>(type)->query_constructor();
    vector<PType> children;
    for (int i = 0; ; i++) {
      PType c = substitute_recursively(type->query_child(i), theta);
      if (c == NULL) break;
      children.push_back(c);
    }
    return new DataType(constructor, children);
  }

  // SOMETHING WEIRD
  return type->copy();
}

bool Typer :: contains(PType sigma, TypeVariable *alpha,
                       TypeSubstitution &t) {
  if (sigma->query_typevar()) {
    TypeVariable *ss = dynamic_cast<TypeVariable*>(sigma);
    if (t[ss] != NULL) return contains(t[ss], alpha, t); 
    if (ss->equals(alpha)) return true;
    return false;
  }

  for (int i = 0; sigma->query_child(i) != NULL; i++) {
    if (contains(sigma->query_child(i), alpha, t)) return true;
  }
  return false;
}

PType Typer :: unify_help(PType sigma, PType tau,
                          TypeSubstitution &theta) {
  string constructor;

  /* alpha theta unifies with tau if either theta(alpha) unifies
   * with tau, or alpha is not in the domain of theta (in which case
   * alpha is mapped to tau)
   */
  if (sigma->query_typevar()) {
    TypeVariable *alpha = dynamic_cast<TypeVariable*>(sigma);
    if (alpha->equals(tau)) return alpha->copy();
    if (theta[alpha] != NULL)
      return unify_help(theta[alpha], tau, theta);
    while (tau->query_typevar()) {
      // tau might still be equal to sigma, after substituting
      TypeVariable *beta = dynamic_cast<TypeVariable*>(tau);
      if (theta[beta] == NULL) break;
      tau = theta[beta];
      if (alpha->equals(tau)) return alpha->copy();
    }
    if (contains(tau, alpha, theta)) return NULL;
    theta[alpha] = tau->copy();
    return tau->copy();
  }

  /* sigma unifies with beta if either sigma unifies with
   * theta(beta), or beta is not in the domain of theta (in which
   * case beta is mapped to sigma)
   */
  if (tau->query_typevar()) {
    TypeVariable *beta = dynamic_cast<TypeVariable*>(tau);
    if (beta->equals(sigma)) return beta->copy();
    if (theta[beta] != NULL)
      return unify_help(sigma, theta[beta], theta);
    if (contains(sigma, beta, theta)) return NULL;
    theta[beta] = sigma->copy();
    return sigma->copy();
  }

  /* two non-variables only unify if they have the same form */
  if (sigma->query_composed() && !tau->query_composed()) return NULL;
  if (sigma->query_data()) {
    if (!tau->query_data()) return NULL;
    string c1, c2;
    c1 = dynamic_cast<DataType*>(sigma)->query_constructor();
    c2 = dynamic_cast<DataType*>(tau)->query_constructor();
    if (c1 != c2) return NULL;
    constructor = c1;
  }

  /* Recursive work: the unification of a(s1,...,sn) theta with
   * a(t1,...,tn) theta is found by first unifying s1 theta with
   * t1 theta, then on to sn theta with tn theta; during the
   * process, theta is extended with new finds, and the result must
   * still have theta applied on it
   */
  vector<PType> children;
  for (int i = 0; ; i++) {
    PType si = sigma->query_child(i);
    PType ti = tau->query_child(i);
    if (si == NULL && ti == NULL) break;
    PType ci = NULL;
    if (si != NULL && ti != NULL) {
      ci = unify_help(si,ti,theta);
    }
    if (ci == NULL) {
      for (int j = 0; j < i; j++) delete children[j];
      return NULL;
    }
    children.push_back(ci);
  }

  if (sigma->query_composed())
    return new ComposedType(children[0], children[1]);
  return new DataType(constructor, children);
}

string Typer :: unify_failure(PType sigma, PType tau,
                              TypeSubstitution &theta) {
  PType type1 = substitute_recursively(sigma, theta);
  PType type2 = substitute_recursively(tau, theta);
  string ret =  "could not unify " + type1->to_string() +
                " with " + type2->to_string() + ".";
  delete type1;
  delete type2;
  return ret;
}

PType Typer :: fresh_copy(PType sigma) {
  Varset ftv = sigma->vars();
  TypeSubstitution sub;
  set<int>::iterator it;
  for (it = ftv.begin(); it != ftv.end(); it++) {
    sub[*it] = new TypeVariable();
  }
  return sigma->copy()->substitute(sub);
}

PType Typer :: unify(PType sigma, PType tau) {
  TypeSubstitution theta;
  PType ret = unify_help(sigma, tau, theta);
  PType ret2 = substitute_recursively(ret, theta);
  delete ret;
  return ret2;
}

bool Typer :: check_consistency(PPartTypedTerm term, Alphabet &Sigma,
                                Environment &Gamma) {
  /* variables: must have the same type or NULL everywhere in the
   * term; type unifying is not sufficient
   */
  if (term->name == "variable") {
    if (term->type == NULL) {
      if (Gamma.contains(term->idata))
        term->type = Gamma.get_type(term->idata)->copy();
      return true;
    }
    // type is not NULL
    if (!Gamma.contains(term->idata)) {
      Gamma.add(term->idata, term->type->copy());
      return true;
    }
    // type denotation given, and already known
    if (!term->type->equals(Gamma.get_type(term->idata))) {
      last_warning = "Inconsistent typing: variable has been "
        "assigned both type " +
        Gamma.get_type(term->idata)->to_string() +
        " and type " + term->type->to_string() + ".";
      return false;
    }
    return true;
  }

  /* constants: all occurrences of a constant with type must be an
   * instance of that type
   */
  if (term->name == "constant") {
    PType expected = Sigma.query_type(term->sdata);
    if (expected == NULL) {
      last_warning = "Unknown constant: " + term->sdata + ".";
      return false;
    }
    if (term->type == NULL) {
      term->type = fresh_copy(expected);
      return true;
    }

    TypeSubstitution temp;
    if (!expected->instantiate(term->type, temp)) {
      last_warning = "Problem typing constant: " +
        term->type->to_string() + " is not an instantiation of " +
        expected->to_string() + ".";
      return false;
    }
    return true;
  }

  /* anything else: only check children */
  for (int i = 0; i < term->children.size(); i++)
    if (!check_consistency(term->children[i], Sigma, Gamma))
      return false;

  return true;
}

PType Typer :: type_variable(PPartTypedTerm var, Environment &Gamma) {
  // note: this function is only called if var->type is NULL
  if (Gamma.contains(var->idata))
    var->type = Gamma.get_type(var->idata)->copy();
  else {
    var->type = new TypeVariable();
    Gamma.add(var->idata, var->type->copy());
  }

  return var->type;
}

PType Typer :: type_abstraction(PPartTypedTerm todo, Environment
                                &Gamma, TypeSubstitution &theta) {
  
  if (todo->children.size() != 2) {
    last_warning = "Program error: trying to type an abstraction "
      "which does not have the expected number of children (2).";
    return NULL;
  }

  PType sigma = type_variable(todo->children[0], Gamma);
  if (sigma == NULL) return NULL;
  PType tau = type_help(todo->children[1], Gamma, theta);
  if (tau == NULL) return NULL;
  todo->type = new ComposedType(sigma->copy(), tau->copy());
  return todo->type;
}

PType Typer :: type_application(PPartTypedTerm todo, Environment
                                &Gamma, TypeSubstitution &theta) {
  if (todo->children.size() != 2) {
    last_warning = "Program error: trying to type an abstraction "
      "which does not have the expected number of children (2).";
    return NULL;
  }

  PType sigma = type_help(todo->children[0], Gamma, theta);
  if (sigma == NULL) return NULL;
  PType tau = type_help(todo->children[1], Gamma, theta);
  if (tau == NULL) return NULL;

  PType alpha = new TypeVariable();  
  PType expected = new ComposedType(tau->copy(), alpha->copy());
  PType unification = unify_help(sigma, expected, theta);
  if (unification == NULL) {
    last_warning = "Could not type application " + todo->to_string()+
      ": " + unify_failure(sigma, expected, theta);
    delete expected;
    return NULL;
  }
  
  delete expected;
  delete unification;
  todo->type = alpha;
  return todo->type;
}

PType Typer :: type_meta(PPartTypedTerm todo, Environment &Gamma,
                         TypeSubstitution &theta) {
  if (todo->children.size() == 0) {
    last_warning = "Program error: ill-defined meta-variable application.";
    return NULL;
  }

  PType Z = type_variable(todo->children[0], Gamma);
  if (Z == NULL) return NULL;
  if (todo->children.size() == 0) {
    todo->type = Z->copy();
    return todo->type;
  }
  
  for (int i = 1; i < todo->children.size(); i++) {
    if (type_help(todo->children[i], Gamma, theta) == NULL)
      return NULL;
  }

  PType outp = new TypeVariable();
  PType total = outp;
  for (int j = todo->children.size()-1; j > 0; j--) {
    total = new ComposedType(todo->children[j]->type, total);
  }

  PType unification = unify_help(Z, total, theta);
  if (unification == NULL) {
    last_warning = "Could not type meta-variable application " +
      todo->to_string() + ":" +
      unify_failure(Z, total, theta);
    delete total;
    return NULL;
  }
  
  todo->type = outp->copy();
  delete total;
  delete unification;   // all the information is in theta anyway
  return todo->type;
}

PType Typer :: type_help(PPartTypedTerm todo, Environment &Gamma,
                         TypeSubstitution &theta) {
  if (todo->type != NULL) return todo->type;
  
  if (todo->name == "variable")
    return type_variable(todo, Gamma);
  if (todo->name == "abstraction")
    return type_abstraction(todo, Gamma, theta);
  if (todo->name == "application")
    return type_application(todo, Gamma, theta);
  if (todo->name == "meta")
    return type_meta(todo, Gamma, theta);
  
  last_warning = "Program error: partly typed term of "
    "unexpected form (" + todo->name + ").";
  return NULL;
}

PTerm Typer :: parse_fully_typed_term(PPartTypedTerm term,
                                      TypeSubstitution &theta) {

  if (term->type == NULL) return NULL;

  // type is actually only important in this case
  if (term->children.size() == 0) {
    PType type = substitute_recursively(term->type, theta);
    if (term->name == "constant") return new Constant(term->sdata, type);
    if (term->name == "variable") return new Variable(type, term->idata);
    last_warning = "Program error: partly typed term has an unusual form.";
    return NULL;
  }

  // find out the children; if they can't be converted, abort
  vector<PTerm> cchildren;
  for (int i = 0; i < term->children.size(); i++) {
    PTerm c = parse_fully_typed_term(term->children[i], theta);
    if (c == NULL) {
      for (int j = 0; j < i; j++) delete cchildren[j];
      return NULL;
    }
    cchildren.push_back(c);
  }

  if (term->name == "application" && cchildren.size() == 2) {
    return new Application(cchildren[0], cchildren[1]);
  }
  if (term->name == "abstraction" && cchildren.size() == 2) {
    if (cchildren[0]->query_variable()) {
      return new Abstraction(dynamic_cast<PVariable>(cchildren[0]),
                             cchildren[1]);
    }
  }
  if (term->name == "meta") {
    if (cchildren[0]->query_variable()) {
      vector<PTerm> ccc;
      ccc.insert(ccc.end(), cchildren.begin()+1,cchildren.end());
      return new MetaApplication(dynamic_cast<PVariable>(cchildren[0]),
                                 ccc);
    }
  }

  // none of the standard patterns! Fail.
  for (int j = 0; j < cchildren.size(); j++) delete cchildren[j];
  return NULL;
}

PTerm Typer :: type_term(PPartTypedTerm term, Alphabet &Sigma,
                         PType expected_type) {

  Environment Gamma;
  if (!check_consistency(term, Sigma, Gamma)) return NULL;
  if (!check_consistency(term, Sigma, Gamma)) return NULL;
    // we do this twice, to make sure all variables which have a type
    // anywhere, are typed everywhere

  TypeSubstitution theta;
  PType result_type = type_help(term, Gamma, theta);
  if (result_type == NULL) return NULL;
  
  // respect expected_type
  if (expected_type != NULL) {
    PType unification = unify_help(result_type, expected_type, theta);
    if (unification == NULL) {
        last_warning = "Term has different type than expected: " +
        unify_failure(result_type, expected_type, theta);
      return NULL;
    }
    delete unification;   // theta has all the required information
  }
  
  // turn it into a term!  
  return parse_fully_typed_term(term, theta);
}

string Typer :: query_warning() {
  return last_warning;
}

