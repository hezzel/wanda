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

#include "inputreaderfo.h"
#include "outputmodule.h"
#include "rulesmanipulator.h"
#include "substitution.h"
#include "typer.h"
#include <fstream>
#include <iostream>

int InputReaderFO :: read_file(string filename, Alphabet &F,
                                vector<MatchRule*> &rules,
                                bool &innermost) {
  
  ifstream file(filename.c_str());
  
  string txt;
  while (!file.eof()) {
    string input;
    getline(file, input);
    txt += input + "\n";
  }

  return read_text(txt, F, rules, innermost);
}

int InputReaderFO :: read_text(string txt, Alphabet &F,
                               vector<MatchRule*> &rules,
                               bool &innermost) {
  
  // split into parts VAR, RULES and maybe STRATEGY
  map<string,string> parts = split_parts(txt);
  if (last_warning != "") {}
  else if (parts.find("RULES") == parts.end())
    last_warning = "RULES not given!";
  else if (parts.find("VAR") == parts.end())
    last_warning = "VAR not given!";
  else for (map<string,string>::iterator it = parts.begin();
            it != parts.end(); it++) {
    if (it->first != "RULES" &&
        it->first != "VAR" &&
        it->first != "STRATEGY")
      last_warning = "Unexpected part: " + it->first;
  }

  if (last_warning != "") return 2;

  // parse strategy
  if (parts.find("STRATEGY") != parts.end()) {
    if (parts["STRATEGY"] == "INNERMOST") innermost = true;
    else if (parts["STRATEGY"] == "FULL") innermost = false;
    else {
      last_warning = "Unexpected strategy: " + parts["STRATEGY"];
      return 2;
    }
  }
  else innermost = false;
  
  // parse variables
  int k = 0, j;
  string vars = parts["VAR"] + " ";
  Environment Gamma;
  while ((j = vars.find(' ', k)) != string::npos) {
    if (j == k) { k++; continue; }
    string var = vars.substr(k, j-k);
    PVariable x = new Variable(new DataType("o"));
    Gamma.add(x, var);
    delete x;
    k = j+1;
  }

  // parse rules
  k = 0;
  string prules = parts["RULES"] + "\n";
  while ((j = prules.find('\n', k)) != string::npos) {
    if (j == k) { k++; continue; }
    string rule = prules.substr(k, j-k);
    if (rule.find("->") != string::npos) {
      PTerm left, right;
      if (!read_rule(rule, F, Gamma, left, right)) return 2;
      if (left == NULL || right == NULL) return 4;
      rules.push_back(new MatchRule(left, right));
    }
    k = j+1;
  }
  
  // Looks like everything worked! Give feedback, assign types, and return.
  ArList arities = wout.arities_for_system(F, rules);
  wout.print("We are asked to determine termination of the following "
    "first-order TRS.\n");
  wout.print_alphabet(F, arities);
  wout.print_rules(rules, F, arities);
  RulesManipulator manip;
  if (innermost ||
      (manip.left_linear(rules) && !manip.has_critical_pairs(rules))
     ) {
    if (assign_types(F, rules)) {
      if (innermost) {
        wout.print("We only need to prove innermost termination, so "
          "by " + wout.cite("FuhGieParSchSwi11") + " it suffices to "
          "prove termination of the typed system, with sort "
          "annotations chosen to respect the rules, as follows:\n");
      }
      else {
        wout.print("As the system is orthogonal, it is terminating "
          "if it is innermost terminating by " + wout.cite("Gra95") +
          ".  Then, by " + wout.cite("FuhGieParSchSwi11") + ", it "
          "suffices to prove (innermost) termination of the typed "
          "system, with sort annotations chosen to respect the "
          "rules, as follows:\n");
      }
      wout.print_alphabet(F, arities);
    }
  }
  return 0;
}

map<string,string> InputReaderFO :: split_parts(string txt) {
  map<string,string> ret;
  while (true) {
    int k = txt.find('(');
    if (k == string::npos) return ret;
    int j = find_matching_bracket(txt, k);
    if (j == -1) return ret;
    string full = txt.substr(k+1, j-k-1);
    txt = txt.substr(j+1);
    int space = full.find(' ');
    if (space == string::npos) {
      last_warning = "Illegal part: " + full;
      return ret;
    }
    ret[full.substr(0, space)] = full.substr(space+1);
  }
}

bool InputReaderFO :: assign_types(Alphabet &Sigma,
                                   Ruleset &rules) {

  // turn the whole system into a term
  vector<PPartTypedTerm> lhs, rhs;
  map<string,int> cnaming;
  int i;
  for (i = 0; i < rules.size(); i++) {
    PTerm l = rules[i]->query_left_side();
    PTerm r = rules[i]->query_right_side();
    lhs.push_back(make_parttyped_term(l, cnaming));
    rhs.push_back(make_parttyped_term(r, cnaming));
  }
  Alphabet alf;
  PPartTypedTerm complete = make_single_term(lhs, rhs, alf);

  // send it to the typer
  Typer typer;
  PTerm typed = typer.type_term(complete, alf);
  delete complete;
  if (typed == NULL) return false;

  // do we actually have more than one type here?
  Varset typevars = typed->free_typevar();
  if (typevars.size() <= 1) { delete typed; return false; }

  // we do! Coolish, let's give new types :)
  vector<string> constants = Sigma.get_all();
  Sigma.clear();
  for (i = 0; i < constants.size(); i++) {
    int idata = cnaming[constants[i]];
    PType ctype = typed->lookup_type(idata);
    if (ctype == NULL) continue;
    Sigma.add(constants[i], make_sort_constants(ctype));
  }

  // and finish off!
  delete typed;
  return true;
}

PType InputReaderFO :: make_sort_constants(PType polytype) {
  if (polytype->query_typevar()) {
    string name = polytype->to_string().substr(1);
    return new DataType(name);
  }

  if (polytype->query_composed()) {
    PType t1 = make_sort_constants(polytype->query_child(0));
    PType t2 = make_sort_constants(polytype->query_child(1));
    return new ComposedType(t1, t2);
  }

  return polytype->copy();
}

PPartTypedTerm InputReaderFO :: make_parttyped_term(PTerm term, map<string,int> &naming) {
  if (term->query_abstraction()) return NULL;
  if (term->query_variable()) return NULL;
  if (term->query_meta() && term->number_children() != 0) return NULL;
  
  PPartTypedTerm ret = new PartTypedTerm;

  if (term->query_meta()) {
    MetaApplication *ma = dynamic_cast<MetaApplication*>(term);
    PVariable Z = ma->get_metavar();
    ret->name = "variable";
    ret->idata = Z->query_index();
  }

  if (term->query_constant()) {
    string name = term->to_string(false);
    ret->name = "variable";
    if (naming.find(name) == naming.end()) {
      int k = naming.size();
      naming[name] = -k - 2;
        // note: -2 avoids a constant with id 0 (which might overlap
        // with a variable) and a constant with id -1 (which, when we
        // try to create it as a variable, gives a fresh variable!)
    }
    ret->idata = naming[name];
  }

  if (term->query_application()) {
    ret->name = "application";
    PTerm child1 = term->get_child(0);
    PTerm child2 = term->get_child(1);
    ret->children.push_back(make_parttyped_term(child1, naming));
    ret->children.push_back(make_parttyped_term(child2, naming));
  }

  return ret;
}

bool InputReaderFO :: read_rule(string line, Alphabet &Sigma,
                                 Environment &Gamma,
                                 PTerm &lhs, PTerm &rhs) {
  // find separating arrow
  int arrow = line.find("->");
  if (arrow == string::npos) {
    last_warning = "missing ->.";
    return false;
  }

  // can we make a rule out of this?
  string left = remove_whitespace(line.substr(0, arrow));
  string right = remove_whitespace(line.substr(arrow+2));
  lhs = read_term(left, Sigma, Gamma, NULL, false, true);
  if (lhs == NULL) return false;
  rhs = read_term(right, Sigma, Gamma, NULL, false, true);
  if (rhs == NULL) { delete lhs; return false; }
  
  // is it a valid rule?
  Varset FVl = lhs->free_var(true);
  Varset FVr = rhs->free_var(true);

  if (!FVl.contains(FVr)) {
    last_warning = "Silly rule: " + line + "\nVariables from the "
                   "right hand side should also occur in the left.";
    delete lhs;
    delete rhs;
    lhs = rhs = NULL;
  }

  return true;
}

string InputReaderFO :: remove_whitespace(string txt) {
  int whitespace = txt.find(" ");
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(" ", whitespace);
  }
  whitespace = txt.find(string(1,9));
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(string(1,9), whitespace);
  }
  
  return txt;
}

PTerm InputReaderFO :: parse_term(string desc, Alphabet &Sigma) {
  desc = remove_whitespace(desc);
  Environment Gamma;
  return read_term(desc, Sigma, Gamma, NULL, true, false);
}

PTerm InputReaderFO :: read_term(string desc, Alphabet &Sigma,
                                 Environment &Gamma,
                                 PType expected_type,
                                 bool funsdeclared,
                                 bool varasmeta) {

  remove_outer_brackets(desc);
  if (desc == "") {last_warning = "Empty term."; return NULL;}

  if (desc.find('(') != string::npos)
    return read_functional(desc, Sigma, Gamma, expected_type,
                           funsdeclared, varasmeta);

  if (Sigma.contains(desc))
    return read_functional(desc + "()", Sigma, Gamma, expected_type,
                           funsdeclared, varasmeta);

  if (funsdeclared || Gamma.lookup(desc) != -1)
    return read_variable(desc, Gamma, expected_type, varasmeta);

  return read_functional(desc + "()", Sigma, Gamma, expected_type,
                         funsdeclared, varasmeta);
}

PTerm InputReaderFO :: read_functional(string desc, Alphabet &Sigma,
                                       Environment &Gamma,
                                       PType expected_type,
                                       bool fdec, bool varasmeta) {

  int a = desc.find('(');
  int b = find_matching_bracket(desc, a);
  if (b != desc.size()-1) {
    last_warning = "Cannot parse " + desc + "!";
    return NULL;
  }

  // is the head a valid function symbol?
  string name = desc.substr(0,a);
  if (!fdec) name = fix_name(name);
    // TODO: make this depend on a separate parameter
  if (fdec && !Sigma.contains(name)) {
    last_warning = "Cannot parse " + desc + ": not a valid "
      "function symbol, " + name;
    return NULL;
  }

  // get the arguments
  string args = desc.substr(a+1,b-a-1);
  vector<string> parts;
  if (args.size() > 0) {
    int k = find_substring(args, ",");
    while (k != -1) {
      parts.push_back(args.substr(0,k));
      args = args.substr(k+1);
      k = find_substring(args, ",");
    }
    parts.push_back(args);
  }

  // get function type, and do necessary checks
  PType type;
  if (Sigma.contains(name)) {
    type = Sigma.query_type(name);
    PType output_type = type;
    int Nargs = 0;
    while (output_type->query_composed()) {
      Nargs++;
      output_type = output_type->query_child(1);
    }
    if (!output_type->query_data()) {
      last_warning = "Symbol " + name + " has a polymorphic type!";
      return NULL;
    }
    if (Nargs != parts.size()) {
      last_warning = "Symbol " + name + " used with incorrect arity!";
      return NULL;
    }
    if (expected_type != NULL &&
        !expected_type->equals(output_type)) {
      last_warning = "Occurrence of " + name + " not well-typed!";
      return NULL;
    }
  }
  else {
    if (expected_type != NULL &&
        expected_type->to_string() != "o") {
      last_warning = "Incomplete typing: symbol " + name + " has "
        "no type in alphabet, but a type is expected!";
      return NULL;
    }
    type = new DataType("o");
    for (int i = 0; i < parts.size(); i++)
      type = new ComposedType(new DataType("o"), type);
    Sigma.add(name, type);
  }

  // parse each of the individual parts
  int i;
  vector<PTerm> tparts;
  for (i = 0; i < parts.size(); i++) {
    PType expected = type->query_child(0);
    type = type->query_child(1);
    PTerm term = read_term(parts[i], Sigma, Gamma, expected,
                           fdec, varasmeta);
    if (term == NULL) {
      for (int j = 0; j < i; j++) delete tparts[j];
      return NULL;
    }
    tparts.push_back(term);
  }

  // make an application out of it
  PTerm ret = Sigma.get(name);
  for (i = 0; i < tparts.size(); i++)
    ret = new Application(ret, tparts[i]);

  return ret;
}

PTerm InputReaderFO :: read_variable(string desc, Environment &Gamma,
                               PType expected_type, bool varasmeta) {
  int id = Gamma.lookup(desc);
  PVariable x;
  if (id == -1 ) {
    if (expected_type == NULL) {
      last_warning = "Cannot derive type of variable " + desc + ".";
      return NULL;
    }
    x = new Variable(expected_type->copy());
    Gamma.add(x);
  }
  else {
    PType gtype = Gamma.get_type(id);
    if (expected_type != NULL && !gtype->equals(expected_type)) {
      last_warning = "Type error: variable " + desc + " has both "
        "types " + gtype->to_string() + " and " +
        expected_type->to_string() + "!";
      return NULL;
    }
    x = new Variable(gtype->copy(), id);
  }
  if (varasmeta) return new MetaApplication(x);
  else return x;
}

