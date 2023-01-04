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

#include "polymodule.h"
#include "outputmodule.h"
#include "smt.h"
#include "substitution.h"
#include <cstdio>
#include <iostream>

PolyModule :: PolyModule()
    : do_base_products(false),
      constraints(this) {
}

PolyModule :: ~PolyModule() {
}

void PolyModule :: set_use_products(bool value) {
  do_base_products = value;
}

vector<int> PolyModule :: orient(OrderingProblem *prob) {
  int i;
  vector<int> ret;

  problem = prob;

  wout.start_method("polynomial interpretations");

  if (!monomorphic()) {
    wout.verbose_print("Requirements are not monomorphic, aborting "
      "polynomials.\n");
    wout.abort_method("polynomial interpretations");
    return ret;
  }

  string attempt = "";
  if (wout.query_verbose()) attempt = "will attempt to ";
  wout.print("We " + attempt + "orient these requirements with a "
    "polynomial interpretation in the natural numbers.\n");
  if (wout.query_verbose() && !do_base_products)
    wout.print("We will not try to use product polynomials.\n");

  // save the constraints we must have by default
  vector<PFormula> baseconstraints = problem->query_constraints();
  for (i = 0; i < baseconstraints.size(); i++)
    constraints.add_formula(baseconstraints[i]->copy());

  arities = problem->arities;
  choose_interpretations();
  interpret_requirements();
  constraints.simplify();
  if (wout.query_debugging()) {
    wout.debug_print("The interpretation constraints are simplified "
    "to:\n");
    constraints.debug_print();
  }
  PFormula fullconstraints = constraints.generate_complete_formula();
  Smt smtsolver(minimum, maximum);

  if (smtsolver.solve(fullconstraints, minimum)) {
    ret = get_solution();
    wout.succeed_method("polynomial interpretations");
  }
  else wout.abort_method("polynomial interpretations");

  vars.reset_valuation();

  return ret;
}

/* =============== STARTUP CHECKS =============== */

bool PolyModule :: monomorphic() {
  vector<PTerm> subterms;
  int i;

  vector<OrderRequirement*> reqs = problem->orientables();
  for (i = 0; i < reqs.size(); i++) {
    subterms.push_back(reqs[i]->left);
    subterms.push_back(reqs[i]->right);
  }

  for (i = 0; i < subterms.size(); i++) {
    PTerm term = subterms[i];
    for (int j = 0; j < term->number_children(); j++)
      subterms.push_back(term->get_child(j));

    if (term->query_constant()) {
      // for constants, check the type of the *main* occurrence (dif-
      // ferent occurrences of f should have the same interpretation)
      // TODO: possibly this could be changed in the future, so a
      // monomorphic part of a polymorphic system has more chance of
      // being able to be handled, but it would involve saving here
      // the parts of F which *actually* occur, including the types
      // they occur with; however, I'm not going to any particular
      // lenghts with this now, because when polymorphism becomes a
      // greater consideration, we may just support polymorphic
      // interpretations instead!
      string name = dynamic_cast<PConstant>(term)->query_name();
      PType ftype = problem->symbol_type(name);
      if (ftype == NULL) ftype = term->query_type();
      if (!ftype->vars().empty()) return false;
    }
    if (term->query_variable()) {
      if (!term->query_type()->vars().empty())
        return false;
    }
    else if (term->query_abstraction()) {
      PVariable x = dynamic_cast<Abstraction*>(term)->query_abstraction_variable();
      if (!x->query_type()->vars().empty()) return false;
    }
    else if (term->query_meta()) {
      PVariable Z = dynamic_cast<MetaApplication*>(term)->get_metavar();
      if (!Z->query_type()->vars().empty()) return false;
    }
  }

  return true;
}

/* =============== BASICS =============== */

Unknown *PolyModule :: new_unknown(int max) {
  int id = minimum.size();
  minimum.push_back(0);
  maximum.push_back(max);
  return new Unknown(id);
}

PolynomialFunction *PolyModule :: make_variable(PType type, int id) {
  vector<int> vars;

  if (type->query_data())
    return new PolynomialFunction(vars, new Polvar(id));

  // we'll actually have to make a function, and create a functional
  vector<PType> types;
  vector<PolynomialFunction*> args;

  while (type->query_composed()) {
    int num = unused_polvar_index();
    PType argtype = type->query_child(0)->copy();
    type = type->query_child(1);

    vars.push_back(num);
    types.push_back(argtype);
    args.push_back(make_variable(argtype, num));
  }

  PPol func = new Functional(id, args);
  return new PolynomialFunction(vars, types, func);
}

PolynomialFunction *PolyModule :: make_nul(PType type) {
  vector<int> vars;
  vector<PType> vartypes;
  while (type->query_composed()) {
    vars.push_back(unused_polvar_index());
    vartypes.push_back(type->query_child(0)->copy());
    type = type->query_child(1);
  }
  return new PolynomialFunction(vars, vartypes, new Integer(0));
}

/* =============== CHOOSING INTERPRETATIONS =============== */

void PolyModule :: filter_check(string symbol, int pos, PPol pol) {
  if (pos >= arities[symbol]) return;
  int is0 = constraints.add_geq(new Integer(0), pol->copy());
  constraints.add_formula(new Or(
    problem->not_filtered_away(symbol, pos+1), new Var(is0)));
}

void PolyModule :: choose_interpretations() {
  int i;

  map<string,int>::iterator it;
  for (it = arities.begin(); it != arities.end(); it++) {
    string f = it->first;
    vector<PType> vartypes;
    bool special = false;

    /***** preparations *****/

    // for the current symbol, make a list of its base type and
    // functional arguments, and also store their types (flattened
    // into types over N)
    vector<int> baseargs, funargs, nargs;
    vector<int> hyperfuns;
    PType type = problem->symbol_type(f);
    if (!type->vars().empty()) continue;
      // non-monomorphic symbols don't occur in the requirements,
      // so don't need an assignment!

    int j;
    for (j = 0; type->query_composed(); j++) {
      PType argtype = type->query_child(0);
      type = type->query_child(1);
      
      // argtype could be base-type, second-order or higher
      if (argtype->query_data()) {
        baseargs.push_back(j);
        nargs.push_back(0);
        vartypes.push_back(new DataType("N"));
      }
      // not base - check number of arguments, and order
      else {
        PType at = argtype;
        bool secondorder = true;
        int k = 0;
        while (at->query_composed()) {
          k++;
          if (!at->query_child(0)->query_data()) secondorder = false;
          at = at->query_child(1);
        }
        nargs.push_back(k);

        // at this point, we don't really have plans for higher
        // order; we just apply it to the 0 function or somesuch
        // so save these separately
        if (secondorder) funargs.push_back(j);
        else hyperfuns.push_back(j);
        vartypes.push_back(type_over_N(argtype));
      }
    }

    /***** making a basic interpretation of the alphabet *****/

    // give all variables their own index
    vector<int> varname;
    for (j = 0; j < nargs.size(); j++) {
      varname.push_back(unused_polvar_index());
    }

    // initalize the interpretation for the symbol    
    Sum *intp = new Sum();
    Unknown *sumbase = new_unknown();
    intp->add_child(sumbase);

    // deal with the #special-fun# symbols (argument functions)
    // (yes, this -is- a bit of a hack, sorry about that, we need it
    // because max in the left-hand side is a bit hard to deal with)
    if (f.substr(0,7) == "#argfun" && hyperfuns.size() == 0) {
      special = true;
      PType argtype = vartypes[0];
      int numextra = 0;
      while (argtype->query_composed()) {
        numextra++;
        argtype = argtype->query_child(1);
      }
      int relevant = nargs.size() - numextra;
      Max *max = new Max();
      for (j = 0; j < baseargs.size(); j++) {
        if (baseargs[j] >= relevant) continue;
        max->add_child(new Polvar(varname[baseargs[j]]));
      }
      for (j = 0; j < funargs.size(); j++) {
        if (funargs[j] >= relevant) continue;
        vector<PPol> args;
        for (int k = relevant; k < nargs.size(); k++)
          args.push_back(new Polvar(k));
        max->add_child(new Functional(varname[funargs[j]], args));
      }
      intp->add_child(max);
      argfunid[f] = sumbase->query_index();
    }

    // deal with the base type arguments for normal symbols
    if (!special) for (j = 0; j < baseargs.size(); j++) {
      int k = baseargs[j];
      Unknown *a = new_unknown();
      Polvar *x = new Polvar(varname[k]);
      intp->add_child(new Product(a, x));
      // if the ordering problem says it's not filterable, it has to
      // occur with parameter >= 1
      if (problem->unfilterable(f) && k < arities[f])
        minimum[a->query_index()] = 1;
      // if they say it is filtered, make sure we don't use it!
      filter_check(f, k, a);
      // also try products
      if (do_base_products) {
        for (int m = j+1; m < baseargs.size(); m++) {
          // TODO: try this with m = j as well, so also squares
          int n = baseargs[m];
          a = new_unknown();
          Polvar *x = new Polvar(varname[k]);
          Polvar *y = new Polvar(varname[n]);
          intp->add_child(new Product(a, new Product(x,y)));
          // still don't use this if either party is filtered
          filter_check(f, k, a);
          filter_check(f, n, a);
        }
      }
    }
    
    // deal with the second-order arguments
    if (!special) for (j = 0; j < funargs.size(); j++) {
      int k = funargs[j];
      int n = nargs[k];
      Unknown *a = new_unknown();
      Sum *definitelynotfiltered = new Sum();
      vector<PPol> args;

      // create a * F(0,...,0)
      for (int l = 0; l < n; l++) args.push_back(new Integer(0));
      intp->add_child(new Product(a, new Functional(varname[k], args)));
      
      // don't use this if F is filtered away, and use this or one of
      // the b * F(x1,...,xn) occurrences if not
      filter_check(f, k, a);
      definitelynotfiltered->add_child(a->copy());

      // create everything of the form b * F(x1,...,xn) or
      // c * x1 * ... * xn * F(x1,...,xn)
      vector< vector<int> > combis = combinations(baseargs, n);
      for (int m = 0; m < combis.size(); m++) {
        Unknown *b = new_unknown(), *c = new_unknown();
        int p;
        // we're not going to do this if any xi (or F) is filtered away
        filter_check(f, k, b);
        filter_check(f, k, c);
        for (p = 0; p < combis[m].size(); p++) {
          filter_check(f, combis[m][p], b);
          filter_check(f, combis[m][p], c);
        }
        // b * F(x1,...,xn)
        args.clear();
        for (p = 0; p < combis[m].size(); p++)
          args.push_back(new Polvar(varname[combis[m][p]]));
        intp->add_child(new Product(b, new Functional(varname[k], args)));
        definitelynotfiltered->add_child(b->copy());
        // c * x1 * ... * xn * F(x1,...,xn)
        for (p = 0; p < args.size(); p++) args[p] = args[p]->copy();
        Functional *func = new Functional(varname[k], args);
        for (p = 0; p < args.size(); p++) args[p] = args[p]->copy();
        args.push_back(c);
        args.push_back(func);
        intp->add_child(new Product(args));
      }
      // if the ordering problem says it's not filtered, it's used in
      // an essential way!
      if (problem->unfilterable(f) && k < arities[f]) {
        int geq1 = constraints.add_geq(
          definitelynotfiltered->simplify(), new Integer(1));
        constraints.add_formula(new Var(geq1));
      }
    }

    // deal with the properly higher-order arguments
    if (!special) for (j = 0; j < hyperfuns.size(); j++) {
      int k = hyperfuns[j];
      int n = nargs[k];
      PType type = vartypes[k];
        // this is the type of F, which has the form A1 -> ... -> An -> o

      vector<PolynomialFunction*> args;
      for (int l = 0; l < n; l++) {
        PType argtype = type->query_child(0);   // this is Ai
        type = type->query_child(1);
        args.push_back(make_nul(argtype));
      }

      Unknown *a = new_unknown();
      Functional *func = new Functional(varname[k], args);
      intp->add_child(new Product(a, func));
      if (problem->unfilterable(f) && k < arities[f])
        minimum[a->query_index()] = 1;
      filter_check(f, k, a);
    }

    vector<int> var_indexes;
    for (j = 0; j < vartypes.size(); j++)
      var_indexes.push_back(varname[j]);
    interpretations[f] =
      new PolynomialFunction(var_indexes, vartypes, intp->simplify());
  }

  /* // for debugging purposes!
  map<int,PPol> substitution;
  substitution[1] = new Integer(1); minimum[1] = maximum[1] = 1;
  for (i = 0; i < symbols.size(); i++)
    interpretations[symbols[i]]->replace_unknowns(substitution);
  */

  if (wout.query_verbose()) {
    wout.print("We start with the following parametric interpretations:\n");
    wout.start_table();
    for (it = arities.begin(); it != arities.end(); it++) {
      string symbol = it->first;
      vector<string> columns;
      map<int,int> freename, boundname;
      columns.push_back(symbol);
      columns.push_back(":");
      columns.push_back(wout.print_polynomial_function(
        interpretations[symbol], freename, boundname));
      wout.table_entry(columns);
    }
    wout.end_table();
  }
}

vector< vector<int> > PolyModule :: combinations(vector<int> &numbers,
                                                 int n) {
  vector< vector<int> > ret;

  if (n == 0) return ret;

  if (n == 1) {
    for (int i = 0; i < numbers.size(); i++) {
      vector<int> tmp;
      tmp.push_back(numbers[i]);
      ret.push_back(tmp);
    }
    return ret;
  }

  for (int i = 0; i < numbers.size(); i++) {
    vector< vector<int> > remainder = combinations(numbers, n-1);
    for (int j = 0; j < remainder.size(); j++)
      remainder[j].push_back(numbers[i]);
    ret.insert(ret.end(), remainder.begin(), remainder.end());
  }
  return ret;
}

PType PolyModule :: type_over_N(PType type) {
  if (type->query_data()) return new DataType("N");
  if (!type->query_composed()) return NULL;
    // yes, this will cause runtimes!
    // TODO: error printing
  PType left = type_over_N(type->query_child(0));
  PType right = type_over_N(type->query_child(1));
  return new ComposedType(left, right);
}

/* =============== CALCULATING TERM INTERPRETATIONS =============== */

void PolyModule :: interpret_requirements() {
  if (wout.query_verbose()) {
    wout.print("This leads to the following parametric constraints.\n");
    wout.start_table();
  }

  vector<OrderRequirement*> reqs = problem->orientables();

  for (int i = 0; i < reqs.size(); i++) {
    // DON'T add >= reqs for the symbol where the argument function
    // symbol occurs in the left-hand side; due to its interpretation
    // as a max, it's always safe
    // we only add a requirement to maybe make the equality strict
    string root = reqs[i]->left->query_head()->to_string(false);
    if (root.substr(0,7) == "#argfun" &&
        argfunid.find(root) != argfunid.end()) {
      int atleastone = constraints.add_geq(
                  new Unknown(argfunid[root]), new Integer(1));
      if (reqs[i]->definite_requirement()) {
        Or *strict = new Or(reqs[i]->orient_greater()->negate(),
                            new Var(atleastone));
        constraints.add_formula(strict);
      }
      continue;
    }

    PType type = reqs[i]->left->query_type();
    vector<PolynomialFunction*> argvars1, argvars2;
    while (type->query_composed()) {
      int varid = unused_polvar_index();
      argvars1.push_back(make_variable(type->query_child(0), varid));
      argvars2.push_back(make_variable(type->query_child(0), varid));
      type = type->query_child(1);
    }
    map<int,PolynomialFunction*> subst;
    PPol l = interpret(reqs[i]->left, subst, argvars1);
    PPol r = interpret(reqs[i]->right, subst, argvars2);

    if (wout.query_verbose()) {
      vector<string> entry;
      map<int,int> freerename, boundrename;
      entry.push_back(wout.print_polynomial(l, freerename, boundrename));
      entry.push_back(wout.polgeq_symbol());
      entry.push_back(wout.print_polynomial(r, freerename, boundrename));
      wout.table_entry(entry);
    }

    // add into the constraints
    if (reqs[i]->definite_requirement()) {
      PPol lsimp = l->simplify();
      PPol rplus = new Sum(r->copy(), new Integer(1));
      int vnumgeq = constraints.add_geq(lsimp->copy(), r->simplify());
      int vnumgre = constraints.add_geq(lsimp, rplus->simplify());
      constraints.add_formula(new Or(
        reqs[i]->orient_greater(), new Var(vnumgeq)));
      constraints.add_formula(new Or(
        reqs[i]->orient_greater()->negate(), new Var(vnumgre)));
    }
    else {
      int varnum = constraints.add_geq(l->simplify(), r->simplify());
      constraints.add_formula(new Or(
        reqs[i]->orient_at_all()->negate(), new Var(varnum)));
    }
  }
  
  if (wout.query_verbose()) {
    wout.end_table();
  }
}

PolynomialFunction *PolyModule :: interpret(PTerm term,
                 map<int,PolynomialFunction*> &subst) {
  PType type = term->query_type();
  vector<PolynomialFunction*> args;
  vector<int> varids;
  vector<PType> vartypes;
  
  while (type->query_composed()) {
    int id = unused_polvar_index();
    PType argtype = type->query_child(0);
    PolynomialFunction *newarg = make_variable(argtype, id);

    varids.push_back(id);
    vartypes.push_back(argtype->copy());
    args.push_back(newarg);

    type = type->query_child(1);
  }

  PPol main = interpret(term, subst, args);
  return new PolynomialFunction(varids, vartypes, main);
}

PPol PolyModule :: interpret(PTerm term, map<int,PolynomialFunction*>
                             &subst, vector<PolynomialFunction*> &args) {
  int i;
  PPol ret;

  // constants => return their chosen interpretations
  if (term->query_constant()) {
    string name = dynamic_cast<PConstant>(term)->query_name();
    if (name.substr(0,2) == "~c") ret = new Integer(0);
    else ret = interpretations[name]->apply(args);
    for (i = 0; i < args.size(); i++) delete args[i];
  }

  // variables => check substitution for interpretations
  if (term->query_variable()) {
    int index = dynamic_cast<PVariable>(term)->query_index();
    if (subst.find(index) == subst.end()) {
      int newvar = unused_polvar_index();
      subst[index] = make_variable(term->query_type(), newvar);
    }
    ret = subst[index]->apply(args);
    for (i = 0; i < args.size(); i++) delete args[i];
  }

  // meta-variable applications => check substitution, and apply on
  // given arguments
  if (term->query_meta()) {
    MetaApplication *ma = dynamic_cast<MetaApplication*>(term);
    PVariable Z = ma->get_metavar();
    int index = Z->query_index();
    if (subst.find(index) == subst.end()) {
      int newvar = unused_polvar_index();
      subst[index] = make_variable(Z->query_type(), newvar);
    }
    vector<PolynomialFunction*> fullargs;
    for (int i = 0; i < ma->number_children(); i++)
      fullargs.push_back(interpret(ma->get_child(i), subst));
    fullargs.insert(fullargs.end(), args.begin(), args.end());
    ret = subst[index]->apply(fullargs);
    for (int j = 0; j < fullargs.size(); j++) delete fullargs[j];
  }

  // abstractions: apply on the first argument
  if (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    int varindex = abs->query_abstraction_variable()->query_index();
    subst[varindex] = args[0];
    vector<PolynomialFunction*> remaining;
    for (int i = 1; i < args.size(); i++)
      remaining.push_back(args[i]);
    ret = interpret(term->get_child(0), subst, remaining);
    delete subst[varindex];
    subst.erase(varindex);
  }

  // application => turn further arguments into args, and use + or
  // max as necessary!
  if (term->query_application()) {
    int i;

    // write term = A b1 ... bn
    vector<PTerm> parts = term->split();
    int n = parts.size()-1;

    // get basic [A]([b1],...,[bn],args)
    PolynomialFunction *main = interpret(parts[0], subst);
    vector<PolynomialFunction*> realargs;
    for (i = 1; i < parts.size(); i++)
      realargs.push_back(interpret(parts[i], subst));
    realargs.insert(realargs.end(), args.begin(), args.end());
    ret = main->apply(realargs);
    
    // if necessary, add the applied symbols too, or use Max
    bool make_application = false;
    int start_at = 0;
    if (!parts[0]->query_constant()) make_application = true;
    else {
      string name = dynamic_cast<PConstant>(parts[0])->query_name();
      start_at = arities[name];
      if (start_at < n) make_application = true;
    }
    if (make_application && problem->unfilterable_application()) {
      vector<PPol> furtherparts;
      furtherparts.push_back(ret);
      for (i = start_at; i < n; i++) {
        vector<PolynomialFunction*> nuls;
        PType argtype = parts[i+1]->query_type();
        while (argtype->query_composed()) {
          nuls.push_back(make_nul(argtype->query_child(0)));
          argtype = argtype->query_child(1);
        }
        furtherparts.push_back(realargs[i]->apply(nuls));
        for (int j = 0; j < nuls.size(); j++) delete nuls[j];
      }
      if (problem->unfilterable_strongly_monotonic())
        ret = new Sum(furtherparts);
      else ret = new Max(furtherparts);
    }

    // delete created polynomials and args
    for (i = 0; i < realargs.size(); i++) delete realargs[i];
  }

  return ret;
}

/* ========== FINISHING OFF ========== */

vector<int> PolyModule :: get_solution() {
  // assign all unknowns to their minimum
  map<int,PPol> substitution;
  int i;
  for (i = 0; i < minimum.size() && i < maximum.size(); i++) {
    maximum[i] = minimum[i];
    substitution[i] = new Integer(minimum[i]);
  }

  problem->justify_orientables();
  bool formal_print = problem->strictly_oriented().size() > 0;

  // replace all unknowns in the interpretations and print
  wout.print("The following interpretation satisfies the requirements:\n");
  wout.start_table();
  if (formal_print) wout.formal_print("Interpretation: [\n");
  bool first = true;
  for (map<string,PolynomialFunction*>::iterator ti =
       interpretations.begin(); ti != interpretations.end(); ti++) {
    ti->second->replace_unknowns(substitution);
    map<int,int> freerename, boundrename;
    vector<string> columns;
    columns.push_back(ti->first);
    columns.push_back("=");
    columns.push_back(wout.print_polynomial_function(ti->second,
                                      freerename, boundrename));
    wout.table_entry(columns);
    if (formal_print) {
      if (!first) wout.formal_print(";\n");
      first = false;
      wout.formal_print("  J(" + ti->first + ") = " + columns[2] + " ");
    }
  }
  wout.end_table();
  if (formal_print) wout.formal_print("\n]\n\n");

  // recalculate the requirements, and determine which are strictly
  // satisfied
  wout.print("Using this interpretation, the requirements translate to:\n");
  wout.start_table();
  vector<OrderRequirement*> reqs = problem->orientables();
  for (i = 0; i < reqs.size(); i++) {
    Environment gamma;
    PolynomialFunction *l, *r;
    vector<int> abstracted;
    map<int,int> metarename;
    map<int,int> freerename, boundrename;
    map<int,string> fn, bn;
    int n, j;

    PFormula form = reqs[i]->orient_at_all()->simplify();
    bool skip = form->query_bottom();
    delete form;
    if (skip) continue;

    interpret_with_information(reqs[i]->left, reqs[i]->right, l, r,
                               metarename);
  
    n = 0;
    choose_meta_namings(reqs[i]->left, gamma,
                        metarename, freerename, n);
    choose_meta_namings(reqs[i]->right, gamma,
                        metarename, freerename, n);

    map<int,string> env;
    Varset vars = gamma.get_variables();
    for (Varset::iterator it = vars.begin(); it != vars.end(); it++)
      env[*it] = gamma.get_name(*it);

    string leftstring = problem->print_term(reqs[i]->left, env, fn, bn);
    string rightstring = problem->print_term(reqs[i]->right, env, fn, bn);
    string leftpolstring =
      wout.print_polynomial_function(l, freerename, boundrename);
    string rightpolstring =
      wout.print_polynomial_function(r, freerename, boundrename);

    string relation;
    vector<PolynomialFunction*> nulargs;
    for (j = 0; j < l->num_variables(); j++) {
      nulargs.push_back(make_nul(l->variable_type(j)));
    }
    PPol lx = l->apply(nulargs);
    PPol rx = r->apply(nulargs);
    int ll = value_at_nul(lx);
    int rr = value_at_nul(rx);
    if (ll > rr && reqs[i]->definite_requirement()) {
      relation = wout.polg_symbol();
      reqs[i]->force_strict();
    }
    else relation = wout.polgeq_symbol();
    delete lx;
    delete rx;
    for (j = 0; j < nulargs.size(); j++) delete nulargs[j];

    vector<string> columns;
    columns.push_back(wout.interpret_left_symbol() + leftstring +
      wout.interpret_right_symbol() + " = " + leftpolstring + " " +
      relation + " " + rightpolstring + " = " +
      wout.interpret_left_symbol() + rightstring +
      wout.interpret_right_symbol());
    wout.table_entry(columns);
  }
  wout.end_table();

  return problem->strictly_oriented();
}

void PolyModule :: interpret_with_information(PTerm left, PTerm right,
                       PolynomialFunction* &l, PolynomialFunction* &r,
                                           map<int,int> &metarename) {
  map<int,PolynomialFunction*> subst;
  l = interpret(left, subst);
  r = interpret(right, subst);
  l->replace_polynomial(l->get_polynomial()->simplify());
  r->replace_polynomial(r->get_polynomial()->simplify());

  // fill metarename
  Varset MVl = left->free_var(true);
  Varset MVr = right->free_var(true);
  MVl.add(MVr);

  for (map<int,PolynomialFunction*>::iterator it = subst.begin();
       it != subst.end(); it++) {
    if (!MVl.contains(it->first)) continue;
    PPol p = it->second->get_polynomial();
    if (p->query_functional()) {
      int k = dynamic_cast<Functional*>(p)->function_index();
      metarename[it->first] = k;
    }
    else if (p->query_variable()) {
      int k = dynamic_cast<Polvar*>(p)->query_index();
      metarename[it->first] = k;
    }
  }
}

void PolyModule :: choose_meta_namings(PTerm term, Environment &gamma,
                                       map<int,int> &metarename,
                                       map<int,int> &indexrename,
                                       int &n) {
  if (term->query_meta()) {
    PVariable x = dynamic_cast<MetaApplication*>(term)->get_metavar();
    int index = x->query_index();
    if (!gamma.contains(index)) {
      // store index => xn (or Fn) in gamma
      char tmp[10];
      sprintf(tmp, "_%c%d", term->query_type()->query_composed() ?
        'F' : 'x', n);
      gamma.add(index, string(tmp));

      // store <corresponding index> => n in indexrename
      indexrename[metarename[index]] = n;

      // and up n!
      n++;
    }
  }

  for (int i = 0; i < term->number_children(); i++)
    choose_meta_namings(term->get_child(i), gamma, metarename,
                        indexrename, n);
}

int PolyModule :: value_at_nul(PPol pol) {
  if (pol->query_integer())
    return dynamic_cast<Integer*>(pol)->query_value();
  if (pol->query_sum())
    return value_at_nul(dynamic_cast<Sum*>(pol)->get_child(0));
  if (pol->query_max())
    return value_at_nul(dynamic_cast<Max*>(pol)->get_child(0));
  return 0;
}

