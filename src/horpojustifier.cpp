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

#include "horpojustifier.h"
#include "horpo.h"
#include "outputmodule.h"
#include <cstdio>
#include <iostream>

void HorpoJustifier :: run(Horpo *_horpo, OrderingProblem *problem,
                           map<string,int> &alphabet) {
  vector<string> symbols;

  start_justification();

  // preparations: save parameters, and find a convenient format for symbols
  for (map<string,int>::iterator it = alphabet.begin();
       it != alphabet.end(); it++) {
    symbols.push_back(it->first);
  }

  horpo = _horpo;
  arities = alphabet;

  // show argument filterings and permutations, status and precedence
  bool modified = show_modifications(symbols);
  show_split(symbols);
  show_precedence(symbols);

  // show the constraints
  if (modified) show_filtered_constraints(problem);
  show_explanation(problem);

  wout.succeed_method("horpo justification");
}

void HorpoJustifier :: start_justification() {
  wout.start_method("horpo justification");
  wout.print("We use a recursive path ordering as defined in " +
    wout.cite("Kop12", "Chapter 5") + ".\n");
}

bool HorpoJustifier :: show_modifications(vector<string> &symbols) {
  bool did_anything = false;
  int i, j;
  string given, result;

  wout.print("Argument functions:\n");
  wout.start_table();

  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    int ar = arities[f];
    vector<string> columns;

    given = wout.interpret_left_symbol() + f;
    if (ar > 0) {
      given += "(";
      for (j = 1; j <= ar; j++) {
        if (j != 1) given += ", ";
        given += "x_" + wout.str(j);
      }
      given += ")";
    }
    given += wout.interpret_right_symbol();
    columns.push_back(given);
    columns.push_back("=");

    // symbols which are filtered away
    if (symbol_filtered(f)) {
      result = "x_";
      for (j = 1; j <= ar; j++) {
        if (!arg_filtered(f,j)) result += wout.str(j);
      }
      columns.push_back(result);
      wout.table_entry(columns);
      symbols[i] = "";
      did_anything = true;
      continue;
    }

    // symbols mapped to bottom
    if (minimal(f)) {
      columns.push_back(wout.bottom_symbol());
      wout.table_entry(columns);
      symbols[i] = "";
      did_anything = true;
      continue;
    }

    // no symbols are filtered away or mapped to a minimal symbol!
    // check whether any arguments are filtered
    bool all_arguments_present = true;
    for (j = 1; j <= ar && all_arguments_present; j++) {
      if (arg_filtered(f,j)) all_arguments_present = false;
    }

    if (ar == 0) continue;

    // in the multiset case, order of arguments does not matter
    if (!lex(f) || ar == 1) {
      if (all_arguments_present) continue;
      result = f;
      bool printed_argument = false;
      for (j = 1; j <= ar; j++) {
        if (arg_filtered(f,j)) continue;
        if (!printed_argument) result += "(";
        else result += ", ";
        printed_argument = true;
        result += "x_" + wout.str(j);
      }
      if (printed_argument) result += ")";
      columns.push_back(result);
      wout.table_entry(columns);
      did_anything = true;
    }

    // in the lexicographic case, however, it does!
    else {
      bool trivial = all_arguments_present;
      for (j = 1; j <= ar && trivial; j++) {
        for (int k = 1; k <= ar && trivial; k++) {
          if (j == k) continue;
          if (permutation(f,j,k)) trivial = false;
        }
      }
      if (!trivial) {
        result = f;
        bool printed_argument = false;
        for (j = 1; j <= ar; j++) {
          for (int k = 1; k <= ar; k++) {
            if (!permutation(f,k,j)) continue;
            if (!printed_argument) result += "(";
            else result += ", ";
            printed_argument = true;
            result += "x_" + wout.str(k);
          }
        }
        if (printed_argument) result += ")";
        columns.push_back(result);
        wout.table_entry(columns);
        did_anything = true;
      }
    }
  }
  wout.end_table();

  if (!did_anything) {
    wout.verbose_print("  None.\n");
    wout.abort_method("horpo justification");
    start_justification();
    return false;
  }
  return true;
}

void HorpoJustifier :: show_split(vector<string> &symbols) {
  vector<string> lexsymb, mulsymb;
  int i;
  string lexes = "Lex = {", muls = "Mul = {";

  for (i = 0; i < symbols.size(); i++) {
    if (symbols[i] == "") continue;
      // the previous function may have removed function symbols
      // which are either symbol filtered or mapped to bottom
    if (lex(symbols[i])) lexsymb.push_back(symbols[i]);
    else mulsymb.push_back(symbols[i]);
  }

  for (i = 0; i < lexsymb.size(); i++) {
    if (i != 0) lexes += ", ";
    lexes += lexsymb[i];
  }
  lexes += "}";
  for (i = 0; i < mulsymb.size(); i++) {
    if (i != 0) muls += ", ";
    muls += mulsymb[i];
  }
  muls += "}";
  wout.print("We choose " + lexes + " and " + muls + ", and ");
}

void HorpoJustifier :: show_precedence(vector<string> &symbols) {
  int nsymbols = 0;
  int i;
  wout.print("the following precedence: ");
  string ret = "";

  for (i = 0; i < symbols.size(); i++)
    if (symbols[i] != "") nsymbols++;

  while (nsymbols > 0) {
    // find out which symbols are not strictly below anything in the
    // precedence; their indexes will be saved in topsymbols
    vector<int> topsymbols;

    for (i = 0; i < symbols.size(); i++) {
      if (symbols[i] == "") continue;
      string f = symbols[i];
      bool anything_larger = false;
      for (int j = 0; j < symbols.size() && !anything_larger; j++) {
        if (symbols[i] == "") continue;
        string g = symbols[j];    
        if (prec(g,f) && !prec(f,g)) anything_larger = true;
      }
      if (!anything_larger) topsymbols.push_back(i);
    }

    // print the top symbols as maximal, and remove them
    for (i = 0; i < topsymbols.size(); i++) {
      if (i != 0) ret += " = ";
      ret += symbols[topsymbols[i]];
      symbols[topsymbols[i]] = "";
    }
    nsymbols -= topsymbols.size();
    if (nsymbols > 0) ret += " > ";
  }
  wout.print(ret + "\n");
}

void HorpoJustifier :: show_filtered_constraints(OrderingProblem *p) {
  wout.print("Taking the argument function into account, and fixing "
    "the greater / greater equal choices, the constraints can be "
    "denoted as follows:\n");
  
  wout.start_table();
  vector<OrderRequirement*> reqs = p->orientables();

  for (int i = 0; i < reqs.size(); i++) {
    PTerm left, right;
    string relation, rule;
    map<int,string> metaname, freename;
    vector<string> columns;
    OrderRequirement *req = reqs[i];

    int varid = horpo->get_constraint_index(req);
    horpo->get_constraint(varid, left, right, relation, rule);
    columns.push_back(print_filtered(left, metaname, freename));
    if (relation == ">") columns.push_back(wout.gterm_symbol());
    else columns.push_back(wout.geqterm_symbol());
    columns.push_back(print_filtered(right, metaname, freename));
    wout.table_entry(columns);
  }
  
  wout.end_table();
}

void HorpoJustifier :: register_constant(string name, PType type, int arity,
                                         Alphabet &alf, ArList &ars) {
  if (!alf.contains(name)) {
    alf.add(name, type->copy());
    ars[name] = arity;
  }
  else if (arity < ars[name]) ars[name] = arity;
}

PTerm HorpoJustifier :: filter(PTerm term, Alphabet &alf, ArList &ars) {
  int i;

  vector<PTerm> split = term->split();

  if (!split[0]->query_constant()) {
    for (i = 0; i < term->number_children(); i++)
      term->replace_child(i, filter(term->get_child(i), alf, ars));
    return term;
  }

  PConstant f = dynamic_cast<PConstant>(split[0]);
  string fname = f->query_name();
  string fnamefull = fname;
  if (fname[fname.length()-1] == '*')
    fname = fname.substr(0,fname.length()-1);

  if (minimal(fname)) {
    PTerm ret = new Constant(wout.bottom_symbol(), term->query_type()->copy());
    delete term;
    return ret;
  }

  if (term->query_constant()) {
    register_constant(fnamefull, f->query_type(), 0, alf, ars);
    return term;
  }

  if (symbol_filtered(fname)) {
    for (i = 1; i < split.size(); i++) {
      if (arg_filtered(fname, i)) delete split[i];
      else term = filter(split[i], alf, ars);
    }
    delete f;
    return term;
  }

  if (!lex(fname)) {
    // mul + nothing filtered: no further changes needed
    if (arg_length_min(fname, split.size()-1)) {
      register_constant(fnamefull, f->query_type(), split.size()-1, alf, ars);
      PTerm backup = term;
      while (term->query_application()) {
        term->replace_child(1, filter(term->get_child(1), alf, ars));
        term = term->get_child(0);
      }
      return backup;
    }

    // create the constant for f'
    PType type = term->query_type()->copy();
    int arity = 0;
    for (i = split.size()-1; i >= 1; i--) {
      if (!arg_filtered(fname, i)) {
        type = new ComposedType(split[i]->query_type()->copy(), type);
        arity++;
      }
    }
    PTerm ret = new Constant(f->query_name(), type);
    register_constant(fnamefull, type, arity, alf, ars);

    // create the filtered term (we can safely ignore the permutation)
    for (i = 1; i < split.size(); i++) {
      if (!arg_filtered(fname, i))
        ret = new Application(ret, filter(split[i]->copy(), alf, ars));
    }
    delete term;
    return ret;
  }
  else {
    // if nothing is even put in a different place, then also
    // nothing is filtered; nothing needs to be done
    bool anythingpermutated = false;
    for (i = 1; i < split.size() && !anythingpermutated; i++)
      if (!permutation(fname, i, i)) anythingpermutated = true;

    if (!anythingpermutated) {
      // note that if s1 .. sk ... sn not permutated, then also
      // s1 ... sk not permutated, so this is safe
      register_constant(fnamefull, f->query_type(), split.size()-1, alf, ars);
      PTerm backup = term;
      while (term->query_application()) {
        term->replace_child(1, filter(term->get_child(1), alf, ars));
        term = term->get_child(0);
      }
      return backup;
    }

    // determine the new order
    vector<PTerm> newargs;
    int arity = split.size();
    if (ars[fname] < arity) arity = ars[fname];
    int j;
    for (j = 1; j < arity; j++) {
      for (i = 1; i < arity; i++) {
        if (permutation(fname, i, j)) {
          newargs.push_back(filter(split[i]->copy(), alf, ars));
        }
      }
    }
    for (j = arity+1; j < split.size(); j++)
      newargs.push_back(filter(split[j]->copy(), alf, ars));

    // create the new constant
    PType type = term->query_type()->copy();
    for (i = newargs.size()-1; i >= 0; i--)
      type = new ComposedType(newargs[i]->query_type()->copy(), type);
    PTerm ret = new Constant(f->query_name(), type);
    register_constant(fnamefull, type, newargs.size(), alf, ars);

    // create the filtered term
    for (i = 0; i < newargs.size(); i++)
      ret = new Application(ret, newargs[i]);
    delete term;
    return ret;
  }
}

string HorpoJustifier :: print_filtered(PTerm term, map<int,string> &m,
                                        map<int,string> &f) {
  // I realise the implementation of this method is very inefficient;
  // not only is a copy made, the filtering then uses additional
  // memory, and is executed on parts which are filtered away.
  // However, this method is hardly ever called, and has the great
  // bonus of being comparatively simple. :)
  
  map<int,string> b;
  Alphabet filtered_alphabet;
  ArList filtered_arities;
  PTerm filtered = term->copy();
  filtered = filter(filtered, filtered_alphabet, filtered_arities);
  string ret = wout.print_term(filtered, filtered_arities,
                               filtered_alphabet, m, f, b);
  delete filtered;
  return ret;
}

void HorpoJustifier :: show_explanation(OrderingProblem *problem) {
  int num = 1;
  map<int,int> previous_proofs;
  vector<OrderRequirement*> reqs = problem->orientables();

  wout.print("With these choices, we have:\n");

  for (int i = 0; i < reqs.size(); i++) {
    int variable = horpo->get_constraint_index(reqs[i]);
    map<int,string> m, f;
    wout.start_table();
    vector< vector<string> > entries;
    justify(variable, num, m, f, previous_proofs, entries);
    for (int j = 0; j < entries.size(); j++)
      wout.table_entry(entries[j]);
    wout.end_table();
  }
}

void HorpoJustifier :: justify(int variable, int &justifyindex,
                               map<int,string> &metaname,
                               map<int,string> &freename,
                               map<int,int> &previous_proofs,
                               vector< vector<string> > &entries) {
  PTerm left, right;
  string relation, rule;
  string status = "";
  vector<string> reasons;
  vector<int> subconstraints;
  bool equal_subconstraint = false;
  int i;

  // basics: get the constraint, and the justification
  horpo->get_constraint(variable, left, right, relation, rule);
  vector<Or*> justification = horpo->query_justification(variable);

  if (left == NULL) {
    cout << "ERROR: trying to justify a non-existing constraint."
         << endl;
  }
  string leftprint = print_filtered(left, metaname, freename);
  string rightprint = print_filtered(right, metaname, freename);

  // find out what exactly the justification is, without caring for
  // unprintable things like argument filterings
  for (i = 0; i < justification.size(); i++) {
    if (justification[i]->query_number_children() == 0) {
      cout << "ERROR: empty justification for constraint "
           << variable << endl;
    }
    Atom *atom = dynamic_cast<Atom*>(justification[i]->query_child(0));
    string description = vars.query_description(atom->query_index());

    if (description.substr(0,8) == "Minimal[" &&
        atom->query_variable()) rule = "(Bot)";
    
    if (description.substr(0,12) == "ArgFiltered[" ||
        description.substr(0,15) == "SymbolFiltered[" ||
        description.substr(0,12) == "Permutation[" ||
        description.substr(0,13) == "ArgLengthMin[" ||
        description.substr(0,8) == "Minimal[" ||
        description == "False" ||
        description[0] == 'X') {
      //reasons.push_back(atom->to_string());
      continue;
    }
  
    if (description.substr(0,7) == "PrecGr[" ||
        description.substr(0,7) == "PrecEq[") {
      int k = description.find(',');
      string f = description.substr(7,k-7);
      string g = description.substr(k+1,description.length()-k-2);
      string relation = " > ";
      if (description[4] == 'E') {
        if (f == g) continue;
        relation = " = ";
      }
      if (atom->query_variable()) reasons.push_back(f + relation + g);
      else reasons.push_back("not " + f + relation + g);
      continue;
    }

    if (description.substr(0,4) == "Lex[") {
      string f = description.substr(4,description.length()-5);
      if (atom->query_variable()) f + " " + wout.in_symbol() + " Lex";
      else status = f + " " + wout.in_symbol() + " Mul";
      continue;
    }

    if (description.substr(0,11) == "constraint " &&
        atom->query_variable()) {
      PTerm l, r;
      string rel, rul;
      horpo->get_constraint(atom->query_index(), l, r, rel, rul);
      // if the subconstraint is the same, we probably don't need to
      // print this one
      string lprint = print_filtered(l, metaname, freename);
      string rprint = print_filtered(r, metaname, freename);
      if (relation == rel && leftprint == lprint &&
          rightprint == rprint) equal_subconstraint = true;
      else if (rel == ">=" && rule == "") {
        if (relation == ">" && lprint.length() == leftprint.length()+1)
          rule = "definition";
        else if (left->query_abstraction() && right->query_abstraction())
          rule = "(Abs)";
        else if (relation == ">=" && rprint == rightprint &&
                 lprint.length() == leftprint.length()+1)
          rule = "(Star)";
      }
      subconstraints.push_back(atom->query_index());
      continue;
    }

    cout << "ERROR: unknown variable description: "
         << description << endl;
    cout << "justification = " << justification[i]->to_string() << endl;
  }

  if (status != "") reasons.push_back(status);

  previous_proofs[variable] = justifyindex;

  // if there is no reason to print anything about this case, don't!
  if (reasons.size() == 0 && subconstraints.size() == 1 &&
      equal_subconstraint) {
    justify(subconstraints[0], justifyindex, metaname, freename,
            previous_proofs, entries);
    return;
  }

  // add an indication of (Var) and (Meta) clauses
  if (rule == "" && left->query_variable()) rule = "(Var)";
  if (rule == "" && left->query_meta()) rule = "(Meta)";

  // looks like there is a reason, start printing it
  vector<string> columns;
  columns.push_back(wout.str(justifyindex) + "]");
  columns.push_back(print_filtered(left, metaname, freename));
  if (relation == ">") columns.push_back(wout.gterm_symbol());
  else columns.push_back(wout.geqterm_symbol());
  columns.push_back(print_filtered(right, metaname, freename));
  columns.push_back("");
  int cur = entries.size();
  entries.push_back(columns);
  justifyindex++;

  // subconstraints need to be printed separately
  string rest = "";
  for (i = 0; i < subconstraints.size(); i++) {
    if (previous_proofs.find(subconstraints[i]) ==
        previous_proofs.end()) {
      justify(subconstraints[i], justifyindex, metaname, freename,
              previous_proofs, entries);
    }
    reasons.push_back("[" + wout.str(previous_proofs[subconstraints[i]]) + "]");
  }

  // print all justifications for the clause!
  for (i = 0; i < reasons.size(); i++) {
    if (i == 0) entries[cur][4] += " because ";
    else if (i == reasons.size()-1) entries[cur][4] += " and ";
    else entries[cur][4] += ", ";
    entries[cur][4] += reasons[i];
  }

  // and the rule of course
  if (rule != "") {
    if (reasons.size() > 0) entries[cur][4] += ",";
    entries[cur][4] += " by " + rule;
  }
}

bool HorpoJustifier :: arg_filtered(string symbol, int index) {
  return vars.query_value(horpo->arg_filtered(symbol, index)) == TRUE;
}

bool HorpoJustifier :: symbol_filtered(string symbol) {
  return vars.query_value(horpo->symbol_filtered(symbol)) == TRUE;
}

bool HorpoJustifier :: permutation(string symbol, int i, int j) {
  return vars.query_value(horpo->permutation(symbol, i, j)) == TRUE;
}

bool HorpoJustifier :: arg_length_min(string symbol, int num) {
  return vars.query_value(horpo->arg_length_min(symbol, num)) == TRUE;
}

bool HorpoJustifier :: minimal(string f) {
  return vars.query_value(horpo->minimal(f)) == TRUE;
}

bool HorpoJustifier :: prec(string f, string g) {
  return vars.query_value(horpo->prec(f,g)) == TRUE;
}

bool HorpoJustifier :: precstrict(PConstant f, PConstant g) {
  return vars.query_value(horpo->precstrict(f,g)) == TRUE;
}

bool HorpoJustifier :: precequal(PConstant f, PConstant g) {
  return vars.query_value(horpo->precequal(f,g)) == TRUE;
}

bool HorpoJustifier :: lex(string f) {
  return vars.query_value(horpo->lex(f)) == TRUE;
}

