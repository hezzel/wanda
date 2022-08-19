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

#include "horpo.h"
#include "horpojustifier.h"
#include "outputmodule.h"
#include "sat.h"
#include <iostream>
#include <cstdio>

Horpo :: Horpo() : constraints(this) {}

vector<int> Horpo :: orient(OrderingProblem *prob) {
  vector<int> ret;

  problem = prob;

  wout.start_method("horpo");

  // initialising
  alphabet = problem->arities;
  save_constraints();
  create_basic_variables();
  save_argfun_constraints();
  save_precedence_constraints();
  force_minimality();

  // solving
  constraints.simplify(alphabet);
  wout.debug_print("The horpo constraints are simplified to:\n" +
    constraints.print() + "\n");

  PFormula formula = constraints.generate_complete_formula();
  formula = formula->simplify();

  SatSolver sat;
  if (!sat.solve(formula)) {
    delete formula;
    wout.abort_method("horpo");
    return ret;
  }
  check_irrelevant_constraints(formula);

  // all strictly oriented constraints can be removed!
  ret = problem->strictly_oriented();
  HorpoJustifier justifier;
  justifier.run(this, prob, alphabet);

  // cleaning up
  alphabet.clear();
  constraints.reset();
  vars.reset_valuation();
  wout.succeed_method("horpo");

  return ret;
}

bool Horpo :: has_monomorphic_applications(PTerm term) {
  if (term->query_application() && !term->query_head()->query_application()) {
    Varset TV = term->query_head()->query_type()->vars();
    if (!TV.empty()) return false;
  }
  for (int i = 0; i < term->number_children(); i++) {
    if (!has_monomorphic_applications(term->get_child(i))) return false;
  }
  return true;
}

void Horpo :: make_application_free(PTerm term) {
  for (int i = 0; i < term->number_children(); i++)
    make_application_free(term->get_child(i));

  if (term->query_application()) {
    // we split only to see whether the head is a constant
    vector<PTerm> parts = term->split();
    int Nargs = parts.size()-1;
    if (parts[0]->query_constant() &&
        alphabet[dynamic_cast<PConstant>(parts[0])->query_name()] >= Nargs)
          return;

    // this is a real application - do something about it!
    PType application_type = term->get_child(0)->query_type()->copy();
    string application_name;
    if (monomorphic) {
      PType collapsed = application_type->collapse();
      application_name = "@_{" + collapsed->to_string() + "}";
      delete collapsed;
      alphabet[application_name] = 2;
    }
    else application_name= "@";
    application_type = new ComposedType(application_type, application_type->copy());
    PConstant app = new Constant(application_name, application_type);
    PTerm newsub = new Application(app, term->get_child(0));
    term->replace_subterm(newsub, "1");
  }
}

void Horpo :: save_constraints() {
  vector<OrderRequirement*> reqs;
  int i;

  // start by saving all basic constraints of the ordering problem
  vector<PFormula> baseconstraints = problem->query_constraints();
  for (i = 0; i < baseconstraints.size(); i++)
    constraints.add_formula(baseconstraints[i]->copy());

  // then check whether all applications are monomorphic (it depends
  // on this whether we add a single "application" to the alphabet, or
  // different symbols for all applications which occur)
  monomorphic = true;
  for (i = 0; i < reqs.size() && monomorphic; i++) {
    if (!has_monomorphic_applications(reqs[i]->left)) monomorphic = false;
    else if (!has_monomorphic_applications(reqs[i]->right)) monomorphic = false;
  }
  if (!monomorphic) alphabet["@"] = 2;

  // save all the ordering constraints in HORPO-form
  reqs = problem->orientables();
  for (i = 0; i < reqs.size(); i++) {
    PTerm left = reqs[i]->left->copy();
    PTerm right = reqs[i]->right->copy();
    make_application_free(left);
    make_application_free(right);
    if (reqs[i]->definite_requirement()) {
      int num1 = constraints.add_geq(left, right);
      int num2 = constraints.add_greater(left->copy(), right->copy());
      geq_requirements[reqs[i]] = num1;
      greater_requirements[reqs[i]] = num2;
      constraints.add_formula(new Or(reqs[i]->orient_greater(),
                                     new Var(num1)));
      constraints.add_formula(new Or(reqs[i]->orient_geq(),
                                     new Var(num2)));
    }
    else {
      int num = constraints.add_geq(left, right);
      geq_requirements[reqs[i]] = num;
      constraints.add_formula(
        new Or(reqs[i]->orient_at_all()->negate(), new Var(num)));
    }
  }
}

void Horpo :: create_basic_variables() {
  /* variables for the argument filtering */
  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    string f = it->first;
    int ar = it->second;

    // SymbolFiltered[f]
    var_symbol_filtered[f] = vars.query_size();
    vars.add_vars(1);
    vars.set_description(symbol_filtered(f),
      "SymbolFiltered[" + f + "]");

    // Permutation[f,i,j]
    var_permutation[f] = vars.query_size();
    vars.add_vars(ar*ar);
    for (int i = 1; i <= ar; i++)
      for (int j = 1; j <= ar; j++)
        vars.set_description(permutation(f,i,j),
          "Permutation[" + f + "," + str(i) + "," + str(j) + "]");

    // ArgLengthMin[f,len]
    var_arg_length_min[f] = vars.query_size();
    vars.add_vars(ar);
    for (int i = 1; i <= ar; i++)
      vars.set_description(arg_length_min(f,i),
        "ArgLengthMin[" + f + "," + str(i) + "]");

    // Minimal[f]
    var_minimal[f] = vars.query_size();
    vars.add_vars(1);
    vars.set_description(minimal(f), "Minimal[" + f + "]");
  }

  /* variables for the precedence */
  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    string f = it->first;
    int n = vars.query_size();
    vars.add_vars(alphabet.size() * 3);
    for (map<string,int>::iterator it2 = alphabet.begin();
         it2 != alphabet.end(); it2++) {
      string g = it2->first;

      var_precedence[f + "^" + g] = n;
      vars.set_description(n, "Prec[" + f + "," + g + "]");
      vars.set_description(n+1, "PrecGr[" + f + "," + g + "]");
      vars.set_description(n+2, "PrecEq[" + f + "," + g + "]");
      n += 3;
    }
  }

  /* variables for the status */
  int n = vars.query_size();
  vars.add_vars(alphabet.size());
  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    string f = it->first;
    var_lex[f] = n;
    vars.set_description(n, "Lex[" + f + "]");
    n++;
  }
}

void Horpo :: save_argfun_constraints() {
  int i, j;

  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    string f = it->first;
    int ar = it->second;
  
    // if f is symbol filtered, then there is at most one argument
    // which is not filtered away
    for (i = 1; i < ar; i++) for (j = i+1; j <= ar; j++) {
      constraints.add_formula(new Or(
          new AntiVar(symbol_filtered(f)),
          new Var(arg_filtered(f, i)),
          new Var(arg_filtered(f, j))
        ));
    }

    // if f is symbol filtered, then there is at least one argument
    // which is not filtered away
    Or *atleastone = new Or(new AntiVar(symbol_filtered(f)));
    for (i = 1; i <= ar; i++) {
      atleastone->add_child(new AntiVar(arg_filtered(f, i)));
    }
    constraints.add_formula(atleastone);

    // if f is symbol filtered, then the argument which is not
    // filtered away must have the same output type as f
    if (f[0] != '@') {
      vector<PType> types;
      PType ftype = problem->symbol_type(f);
      if (ftype == NULL && ar > 0) {    // we don't need to know the
                                        // type of constants!
        cout << "ERROR: type of " << f
             << " does not occur in alphabet!" << endl;
      }
      for (i = 1; i <= ar; i++) {
        types.push_back(ftype->query_child(0));
        ftype = ftype->query_child(1);
      }
      for (i = 1; i <= ar; i++) {
        PType argtype = types[i-1];
        vector<Or*> type_constraints = force_type_equality(ftype,argtype);
        for (j = 0; j < type_constraints.size(); j++) {
          type_constraints[j]->add_child(new AntiVar(symbol_filtered(f)));
          type_constraints[j]->add_child(new Var(arg_filtered(f,i)));
          constraints.add_formula(type_constraints[j]);
        }
      }
    }

    // if f is symbol filtered, we don't use the permutation
    for (i = 1; i <= ar; i++) for (j = 1; j <= ar; j++) {
      constraints.add_formula(new Or(
          new AntiVar(symbol_filtered(f)),
          new AntiVar(permutation(f, i, j))
        ));
    }

    // if f is symbol filtered, it is not mapped to bot
    constraints.add_formula(new Or(
        new AntiVar(symbol_filtered(f)),
        new AntiVar(minimal(f))
      ));

    // if a symbol is mapped to bot, then all its arguments are
    // removed
    for (i = 1; i <= ar; i++) {
      constraints.add_formula(new Or(
          new AntiVar(minimal(f)),
          new Var(arg_filtered(f, i))
        ));
    }

    // if an argument is filtered away, then it does not occur in the
    // domain of the permutation
    for (i = 1; i <= ar; i++) for (j = 1; j <= ar; j++) {
      constraints.add_formula(new Or(
          new Var(symbol_filtered(f)),
          new AntiVar(arg_filtered(f, i)),
          new AntiVar(permutation(f, i, j))
        ));
    }

    // if an argument is not filtered away, then it does occur in the
    // domain of the permutation
    for (i = 1; i <= ar; i++) {
      atleastone = new Or(
        new Var(symbol_filtered(f)),
        new Var(arg_filtered(f, i)));
      for (j = 1; j <= ar; j++) {
        atleastone->add_child(new Var(permutation(f,i,j)));
      }
      constraints.add_formula(atleastone);
    }

    // if a number j is not in the range of the permutation, then
    // the total number of arguments which are not filtered away is
    // lower than j
    for (i = 1; i <= ar; i++) for (j = 1; j <= ar; j++) {
      constraints.add_formula(new Or(
          new AntiVar(permutation(f,i,j)),
          new Var(arg_length_min(f,j))
        ));
    }

    // if the argument length is >= j, then some argument is mapped
    // to position j
    for (j = 1; j <= ar; j++) {
      atleastone = new Or(new AntiVar(arg_length_min(f,j)));
      for (i = 1; i <= ar; i++) {
        atleastone->add_child(new Var(permutation(f,i,j)));
      }
      constraints.add_formula(atleastone);
    }

    // if the argument length is >= j, it is also >= j-1
    for (j = 1; j < ar; j++) {
      constraints.add_formula(new Or(
          new Var(arg_length_min(f,j)),
          new AntiVar(arg_length_min(f,j+1))
        ));
    }

    // the permutation is injective
    for (i = 1; i < ar; i++) for (int i2 = i+1; i2 <= ar; i2++) {
      for (j = 1; j <= ar; j++) {
        constraints.add_formula(new Or(
            new AntiVar(permutation(f,i,j)),
            new AntiVar(permutation(f,i2,j))
          ));
      }
    }

    // the permutation is actually a function
    for (i = 1; i <= ar; i++) {
      for (j = 1; j < ar; j++) for (int j2 = j+1; j2 <= ar; j2++) {
        constraints.add_formula(new Or(
            new AntiVar(permutation(f,i,j)),
            new AntiVar(permutation(f,i,j2))
          ));
      }
    }
  }
}

void Horpo :: save_precedence_constraints() {
  vector<string> symbols;
  int i, j, k;
  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    symbols.push_back(it->first);
  }

  // reflexivity
  for (i = 0; i < symbols.size(); i++) {
    constraints.add_formula(new Var(prec(symbols[i],symbols[i])));
  }
  
  // transitivity
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    for (j = 0; j < symbols.size(); j++) {
      string g = symbols[j];
      for (k = 0; k < symbols.size(); k++) {
        string h = symbols[k];
        constraints.add_formula(new Or(
            new AntiVar(prec(f,g)),
            new AntiVar(prec(g,h)),
            new Var(prec(f,h))
          ));
      }
    }
  }

  // completeness: not necessary, but very convenient for printing
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    for (j = 0; j < symbols.size(); j++) {
      string g = symbols[j];
      constraints.add_formula(new Or(
          new Var(prec(f,g)),
          new Var(prec(g,f))
        ));
    }
  }

  // PrecGr is well-defined
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    for (j = 0; j < symbols.size(); j++) {
      string g = symbols[j];

      // if f > g, then f >= g and not g >= f
      constraints.add_formula(new Or(
          new AntiVar(precstrict(f, g)),
          new Var(prec(f,g))
        ));
      constraints.add_formula(new Or(
          new AntiVar(precstrict(f, g)),
          new AntiVar(prec(g,f))
        ));

      // if f >= g and not g >= f, then f > g
      constraints.add_formula(new Or(
          new AntiVar(prec(f,g)),
          new Var(prec(g,f)),
          new Var(precstrict(f,g))
        ));
    }
  }

  // PrecEq is well-defined
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    for (j = 0; j < symbols.size(); j++) {
      string g = symbols[j];

      // if f = g, then f >= g and g >= f
      constraints.add_formula(new Or(
          new AntiVar(precequal(f, g)),
          new Var(prec(f,g))
        ));
      constraints.add_formula(new Or(
          new AntiVar(precequal(f, g)),
          new Var(prec(g,f))
        ));

      // if f >= g and g >= f, then f = g
      constraints.add_formula(new Or(
          new AntiVar(prec(f,g)),
          new AntiVar(prec(g,f)),
          new Var(precequal(f,g))
        ));
    }
  }

  // the status function must respect the equality induced by the
  // precedence
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    for (j = 0; j < symbols.size(); j++) {
      string g = symbols[j];
      if (i == j) continue;
      constraints.add_formula(new Or(
          new AntiVar(precequal(f,g)),
          new AntiVar(lex(f)),
          new Var(lex(g))
        ));
      constraints.add_formula(new Or(
          new AntiVar(precequal(f,g)),
          new Var(lex(f)),
          new AntiVar(lex(g))
        ));
    }
  }

  /** TODO:
   * When implementing type changing functions, we must here put
   * some additional constraints, that @_sigma = @_tau if sigma and
   * tau define the same type.
   * Probably use something like
   * vector<Or*> different_types(sigma, tau)
   * and add NOT PrecEq[@_sigma,@_tau] to all elements of that list
   */

  // only the dynamic dependency pair situation has further
  // constraints
  if (!problem->unfilterable_substeps_permitted()) return;

  // in the monomorphic case, we must have that @_A >= @_B if B
  // has at most equal length to A
  // NOTE: when using type changing functions, we cannot just use
  // "length" here, but must instead use a more advanced formula
  if (monomorphic) {
    for (i = 0; i < symbols.size(); i++) {
      string f = symbols[i];
      for (j = 0; j < symbols.size(); j++) {
        string g = symbols[j];
        if (f == g) continue;
        if (f[0] == '@' && g[0] == '@') {
          if (f.length() >= g.length())
            constraints.add_formula(new Var(prec(f,g)));
          else
            constraints.add_formula(new AntiVar(prec(f,g)));
        }
      }
    }
  }

  // in the non-monomorphic case, all @s are assigned the same
  // symbol in the alphabet; however, we must still have that
  // @_A > @_B if B is a strict subtype of A.  Mostly, the prec
  // function will take care of that, but it means we cannot have
  // f = @, since that would imply that both f = @_A and f = @_B
  else if (alphabet.find("@") != alphabet.end()) {
    for (i = 0; i < symbols.size(); i++) {
      string f = symbols[i];
      if (f == "@") continue;
      constraints.add_formula(new AntiVar(precequal(f,"2")));
    }
  }
}

void Horpo :: force_minimality() {
  vector<OrderRequirement*> reqs = problem->orientables();
  set<string> symbs;
  int i;

  // add stuff that occurs in the right-hand side
  for (i = 0; i < reqs.size(); i++) {
    vector<PTerm> subs;
    subs.push_back(reqs[i]->right);
    for (int j = 0; j < subs.size(); j++) {
      PTerm term = subs[j];
      for (int k = 0; k < term->number_children(); k++)
        subs.push_back(term->get_child(k));
      if (term->query_constant()) {
        string name = dynamic_cast<PConstant>(term)->query_name();
        if (alphabet[name] == 0) symbs.insert(name);
      }
    }
  }

  // remove stuff that occurs in the left-hand side
  for (i = 0; i < reqs.size(); i++) {
    vector<PTerm> subs;
    subs.push_back(reqs[i]->left);
    for (int j = 0; j < subs.size(); j++) {
      PTerm term = subs[j];
      for (int k = 0; k < term->number_children(); k++)
        subs.push_back(term->get_child(k));
      if (term->query_constant()) {
        string name = dynamic_cast<PConstant>(term)->query_name();
        if (alphabet[name] == 0) symbs.erase(name);
      }
    }
  }

  // set everything that occurs in right but not in left to minimal
  for (set<string>::iterator it = symbs.begin();
       it != symbs.end(); it++) {
    constraints.add_formula(new Var(minimal(*it)));
  }
}

int Horpo :: arg_filtered(string symbol, int index) {
  if (symbol[0] == '@') return vars.false_var();
  int ret = problem->filtered_variable(symbol, index);
  if (ret < 0) return vars.false_var();
  else return ret;
}

int Horpo :: symbol_filtered(string symbol) {
  return var_symbol_filtered[symbol];
}

int Horpo :: permutation(string symbol, int id1, int id2) {
  return var_permutation[symbol] + (id1-1) * alphabet[symbol] + (id2-1);
}

int Horpo :: arg_length_min(string symbol, int len) {
  return var_arg_length_min[symbol] + len-1;
}

int Horpo :: minimal(string symbol) {
  return var_minimal[symbol];
}

int Horpo :: prec(string f, string g) {
  return var_precedence[f + "^" + g];
}

int Horpo :: precstrict(string f, string g) {
  return prec(f, g) + 1;
}

int Horpo :: precequal(string f, string g) {
  return prec(f, g) + 2;
}

int Horpo :: precstrict(PConstant f, PConstant g) {
  string fname = f->query_name();
  string gname = g->query_name();
  if (monomorphic || fname != "@" || gname != "@" ||
      !problem->unfilterable_substeps_permitted())
    return precstrict(fname, gname);
  
  // NOTE: will have to be changed when type changing functions are
  // implemented
  string ftype, gtype;
  if (f->query_type()->to_string().length() <=
      g->query_type()->to_string().length()) return vars.false_var();

  if (same_typevars(f->query_type(), g->query_type(), false))
    return vars.true_var();
  else return vars.false_var();
}

int Horpo :: precequal(PConstant f, PConstant g) {
  string fname = f->query_name();
  string gname = g->query_name();
  if (monomorphic || fname != "@" || gname != "@" ||
      !problem->unfilterable_substeps_permitted())
    return precequal(fname, gname);
  
  // NOTE: will have to be changed when type changing functions are
  // implemented
  string ftype, gtype;
  if (f->query_type()->to_string().length() !=
      g->query_type()->to_string().length()) return vars.false_var();

  if (same_typevars(f->query_type(), g->query_type(), true))
    return vars.true_var();
  else return vars.false_var();
}

bool Horpo :: same_typevars(PType a, PType b, bool exactly) {
  map<int,int> typevars;
  vector<PType> subs;
  int total = 0;

  // find out what type variables occur on either side;
  // those on the right must occur at least as often as on the left
  subs.push_back(a);
  for (int i = 0; i < subs.size(); i++) {
    if (subs[i]->query_typevar()) {
      int id = dynamic_cast<TypeVariable*>(subs[i])->query_index();
      if (typevars.find(id) == typevars.end())
        typevars[id] = 0;
      else typevars[id]++;
      total++;
    }
    for (int k = 0; subs[i]->query_child(k) != NULL; k++)
      subs.push_back(subs[i]->query_child(k));
  }

  subs.push_back(b);
  for (int j = 0; j < subs.size(); j++) {
    if (subs[j]->query_typevar()) {
      int id = dynamic_cast<TypeVariable*>(subs[j])->query_index();
      if (typevars.find(id) == typevars.end() || typevars[id] == 0)
        return false;
      typevars[id]--;
      total--;
    }
    for (int k = 0; subs[j]->query_child(k) != NULL; k++)
      subs.push_back(subs[j]->query_child(k));
  }

  if (exactly) return total == 0;
  else return true;
}

int Horpo :: lex(string f) {
  return var_lex[f];
}

vector<Or*> Horpo :: force_type_equality(PType type1, PType type2) {
  if (type1->query_data() && type2->query_data()) {
    vector<Or*> ret;
    return ret;
  }

  if (type1->query_composed() && type2->query_composed()) {
    vector<Or*> ret = force_type_equality(type1->query_child(0),
        type2->query_child(0));
    vector<Or*> ret2 = force_type_equality(type1->query_child(1),
        type2->query_child(1));
    ret.insert(ret.end(), ret2.begin(), ret2.end());
    return ret;
  }

  vector<Or*> ret;
  if (!type1->equals(type2)) ret.push_back(new Or());
  return ret;
}

vector<Or*> Horpo :: query_justification(int variable) {
  vector<PFormula> base = constraints.justify_constraint(variable);
  vector<Or*> ret;

  for (int i = 0; i < base.size(); i++) {
    if (!base[i]->query_disjunction() &&
        !base[i]->query_variable() &&
        !base[i]->query_antivariable()) {
      cout << "Error! Cannot deal with formula "
           << base[i]->to_string() << endl;
    }
    // if it's an atom, it has to be true!
    if (!base[i]->query_disjunction()) {
      ret.push_back(new Or(base[i]));
      continue;
    }
    Or *baseform = dynamic_cast<Or*>(base[i]);
    Or *newform = new Or();
    ret.push_back(newform);
    for (int j = 0; j < baseform->query_number_children(); j++) {
      if (!baseform->query_child(j)->query_variable() &&
          !baseform->query_child(j)->query_antivariable())
        cout << "Error! Cannot deal with formula "
             << base[i]->to_string() << endl;
      Atom *child = dynamic_cast<Atom*>(baseform->query_child(j));
      int index = child->query_index();
      if (index == variable) continue;
      if (child->query_variable() && vars.query_value(index) == FALSE)
        continue;
      if (child->query_antivariable() && vars.query_value(index) == TRUE)
        continue;

      newform->add_child(child->copy());
    }
  }
  return ret;
}

void Horpo :: get_constraint(int variable, PTerm &left, PTerm &right,
                             string &relation, string &rule) {
  HorpoConstraint *constraint =
    constraints.constraint_by_index(variable);
  rule = "";

  if (constraint == NULL) {
    left = NULL;
    right = NULL;
    relation = "";
  }
  else {
    left = constraint->left;
    right = constraint->right;
    if (constraint->relation == ">")  relation = ">";
    else {
      relation = ">=";
      if (constraint->relation == ">=fun") rule = "(Fun)";
      if (constraint->relation == ">=eta")
        rule = "(Eta)" + wout.cite("Kop13:2");
      if (constraint->relation == ">=stat") rule = "(Stat)";
      if (constraint->relation == ">=fabs") rule = "(F-Abs)";
      if (constraint->relation == ">=copy") rule = "(Copy)";
      if (constraint->relation == ">=select") rule = "(Select)";
    }
  }
}

int Horpo :: get_constraint_index(OrderRequirement *req) {
  int ret;
  PFormula form;
  if (req->definite_requirement()) {
    form = req->orient_greater()->simplify();
    if (form->query_top()) ret = greater_requirements[req];
    else ret = geq_requirements[req];
  }
  else {
    form = req->orient_at_all()->simplify();
    if (form->query_bottom()) ret = -1;
    else ret = geq_requirements[req];
  }
  delete form;
  return ret;
}

void Horpo :: check_irrelevant_constraints(PFormula formula) {
  vector<int> relevant_vars = constraints.constraint_variables();
  int i;

  // say that we are actually unsure about all false constraints
  for (i = 0; i < relevant_vars.size(); i++) {
    int var = relevant_vars[i];
    if (vars.query_value(var) == FALSE)
      vars.force_value(var, UNKNOWN);
  }

  formula = formula->simplify();

  bool changed = true;
  for (int tries = 1; tries < 10 && changed; ) {
    changed = false;

    // force everything that has to have a certain value
    if (formula->query_conjunction()) {
      And *form = dynamic_cast<And*>(formula);
      for (i = 0; i < form->query_number_children(); i++) {
        PFormula child = form->query_child(i);
        if (child->query_variable() || child->query_antivariable()) {
          dynamic_cast<Atom*>(child)->force();
          changed = true;
        }
      }
    }
    if (formula->query_variable() || formula->query_antivariable()) {
      dynamic_cast<Atom*>(formula)->force();
      changed = true;
    }
    if (changed) {
      formula = formula->simplify();
      continue;
    }

    // the next part is more intensive, so we can do this less often
    tries++;

    // find out which variables occur negatively
    set<int> negative_occurrences;
    vector<PFormula> subs;
    subs.push_back(formula);
    for (i = 0; i < subs.size(); i++) {
      if (subs[i]->query_conjunction() ||
          subs[i]->query_disjunction()) {
        AndOr *andor = dynamic_cast<AndOr*>(subs[i]);
        for (int j = 0; j < andor->query_number_children(); j++)
          subs.push_back(andor->query_child(j));
      }
      if (subs[i]->query_antivariable()) {
        int index = dynamic_cast<AntiVar*>(subs[i])->query_index();
        negative_occurrences.insert(index);
      }
    }
    subs.clear();

    // set all variables to true which do not occur negatively
    for (i = 0; i < (int)relevant_vars.size(); i++) {
      int var = relevant_vars[i];
      if (vars.query_value(var) != UNKNOWN) {
        relevant_vars[i] = relevant_vars[relevant_vars.size()-1];
        relevant_vars.pop_back();
        i--;
        continue;
      }
      if (negative_occurrences.find(var) == negative_occurrences.end()) {
        vars.force_value(var, TRUE);
        relevant_vars[i] = relevant_vars[relevant_vars.size()-1];
        relevant_vars.pop_back();
        i--;
        changed = true;
      }
    }

    if (changed) formula = formula->simplify();
  }

  // set everything we haven't yet figured out to false
  for (i = 0; i < relevant_vars.size(); i++) {
    int var = relevant_vars[i];
    if (vars.query_value(var) == UNKNOWN)
      vars.force_value(var, FALSE);
  }

  formula = formula->simplify();
  if (!formula->query_top()) {
    cout << "ERROR! Formula generated by HORPO does not reduce to "
         << "truth!" << endl;
  }
  
  delete formula;
}

string Horpo :: str(int num) {
  return wout.str(num);
}

