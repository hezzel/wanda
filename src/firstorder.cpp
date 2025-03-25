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

#include "firstorder.h"
#include "environment.h"
#include "inputreaderfo.h"
#include "outputmodule.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>

FirstOrderSplitter :: FirstOrderSplitter(Alphabet &Sigma,
                                         Ruleset &rules,
                                         string FOtool,
                                         string FOnontool) {
  fotool = FOtool;
  fonontool = FOnontool;

  // initialise TFO and PHO
  vector<string> symbols = Sigma.get_all();

  // if FOtool is none, the first order splitting part should not be
  // done; we do so by considering nothing first order
  if (FOtool == "none") {
    for (int i = 0; i < symbols.size(); i++) PHO.insert(symbols[i]);
    return;
  }

  // a symbol f is potentially higher-order if either one of its
  // input arguments is not a data type, or its output type is a
  // type variable
  int i;
  for (i = 0; i < symbols.size(); i++) {
    string f = symbols[i];
    PType type = Sigma.query_type(f);
    bool pho = false;
    while (type->query_composed() && !pho) {
      if (!type->query_child(0)->query_data()) pho = true;
      type = type->query_child(1);
    }
    if (pho || !type->query_data()) { PHO.insert(f); continue; }
  }

  // a symbol is also potentially higher-order if it is the root
  // symbol of a rule of composed type
  for (i = 0; i < rules.size(); i++) {
    if (rules[i]->query_left_side()->query_type()->query_composed()) {
      PHO.insert(rules[i]->query_left_side()->query_head()->to_string(false));
    }
  }

  // closure: if there is a rule f s1 ... sn -> r with a symbol in
  // one of the si or r in PHO, then also f in PHO
  bool changed = true;
  while (changed) {
    changed = false;
    for (i = 0; i < rules.size(); i++) {
      string head = rules[i]->query_left_side()->query_head()->to_string(false);
      if (PHO.find(head) != PHO.end()) continue;

      vector<PTerm> stack;
      stack.push_back(rules[i]->query_left_side());
      stack.push_back(rules[i]->query_right_side());
      bool ok = true;
      for (int j = 0; j < stack.size() && ok; j++) {
        PTerm s = stack[j];
        if (s->query_meta() && s->subterm("0") == NULL) continue;
        PTerm shead = s->query_head();
        if (!shead->query_constant()) ok = false;
        if (PHO.find(shead->to_string(false)) != PHO.end()) ok = false;
        while (s != shead) {
          stack.push_back(s->subterm("2"));
          s = s->subterm("1");
        }
      }

      if (!ok) {
        changed = true;
        PHO.insert(head);
      }
    }
  }

  for (i = 0; i < symbols.size(); i++) {
    if (PHO.find(symbols[i]) == PHO.end()) TFO.insert(symbols[i]);
  }
}

bool FirstOrderSplitter :: first_order(PTerm term) {
  // of course, a first order term should have a data type
  if (!term->query_type()->query_data()) return false;
  
  // single variables and meta-variables are counted as first order
  if (term->query_variable()) return true;
  if (term->query_meta() && term->subterm("0") == NULL) return true;

  // otherwise, a term can be first order only if it is f s1 ... sn
  // with all si first order and f not in PHO; if f is an unknown
  // constant, we count it as TFO (if the typing makes this
  // impossible, it will abort on the subterms anyway
  PTerm head = term->query_head();
  if (!head->query_constant()) return false;
  if (PHO.find(head->to_string(false)) != PHO.end()) return false;
  while (term->query_application()) {
    if (!first_order(term->subterm("2"))) return false;
    term = term->subterm("1");
  }
  return true;
}

bool FirstOrderSplitter :: first_order(MatchRule *rule) {
  return first_order(rule->query_left_side()) &&
         first_order(rule->query_right_side());
}

bool FirstOrderSplitter :: first_order(DPSet &D) {
  for (int i = 0; i < D.size(); i++) {
    // if we're going to see it as a first-order problem, all types
    // should be data types
    if (!D[i]->query_left()->query_type()->query_data()) return false;
    if (!D[i]->query_right()->query_type()->query_data()) return false;
    
    vector<PTerm> lparts = D[i]->query_left()->split();
    vector<PTerm> rparts = D[i]->query_right()->split();
    
    // the head may be a non-TFO symbol, because it doesn't have to
    // be reduced itself, but it *should* be something that we can
    // express as a 
    if (!lparts[0]->query_constant() ||
        !rparts[0]->query_constant()) return false;
    for (int j = 0; j < lparts.size(); j++)
      if (!first_order(lparts[j])) return false;
    for (int k = 0; k < rparts.size(); k++) {
      while (rparts[k]->query_abstraction())
        rparts[k] = rparts[k]->subterm("1");
      if (!first_order(rparts[k])) return false;
    }
  }
  return true;
}

Ruleset FirstOrderSplitter :: first_order_part(Ruleset &R) {
  Ruleset ret;
  for (int i = 0; i < R.size(); i++) {
    if (first_order(R[i])) ret.push_back(R[i]);
  }
  return ret;
}

void FirstOrderSplitter :: update_connections(
                                    map<string, set<string> > &graph,
                                    set<string> &connections,
                                    string point) {

  if (connections.find(point) != connections.end()) return;
  connections.insert(point);
  for (set<string>::iterator it = graph[point].begin();
       it != graph[point].end(); it++) {
    update_connections(graph, connections, *it);
  }
}

set< set<string> > FirstOrderSplitter :: split_types(Ruleset &R) {
  int i;
  set< set<string> > ret;

  // get the relevant function symbols (only symbols occurring in the
  // left hand side of rules are relevant)
  Alphabet F;
  for (i = 0; i < R.size(); i++)
    get_constant_data(R[i]->query_left_side(), F);

  // create a graph of related sorts
  vector<string> constants = F.get_all();
  map<string, set<string> > graph;
  for (i = 0; i < constants.size(); i++) {
    if (constants[i] == "~PAIR") continue;
    set<string> subs;
    PType type = F.query_type(constants[i]);
    while (type->query_composed()) {
      PType inp = type->query_child(0);
      type = type->query_child(1);
      if (!inp->query_data()) return ret;
      string d = dynamic_cast<DataType*>(inp)->query_constructor();
      subs.insert(d);
    }
    if (!type->query_data()) return ret;
    string t = dynamic_cast<DataType*>(type)->query_constructor();
    for (set<string>::iterator it = subs.begin();
         it != subs.end(); it++) {
      graph[t].insert(*it);
      graph[*it].insert(t);
    }
  }

  // figure out all connected groups of sorts
  while (!graph.empty()) {
    string symbol = graph.begin()->first;
    set<string> connections;
    update_connections(graph, connections, symbol);
    for (set<string>::iterator it2 = connections.begin();
         it2 != connections.end(); it2++) {
      graph.erase(*it2);
    }
    ret.insert(connections);
  }

  return ret;
}

string FirstOrderSplitter :: print_functionally(PTerm term,
                                                Environment &gamma) {
  if (term->query_meta()) {
    if (term->subterm("0") != NULL) return "ERRORERRORERROR(";
      // first-order tool is definitely going berserk on an extra
      // bracket :P
    PVariable mv =
      dynamic_cast<MetaApplication*>(term)->get_metavar();
    return mv->to_string(gamma);
  }
  
  vector<PTerm> parts = term->split();
  if (!parts[0]->query_constant()) return "ERRORERRORERROR(";
  string name = parts[0]->to_string(false);
  if (parts.size() == 1) return name;
  string ret = name + "(";
  for (int i = 1; i < parts.size(); i++) {
    if (i > 1) ret += ",";
    ret += print_functionally(parts[i], gamma);
  }
  return ret + ")";
}

void FirstOrderSplitter :: create_file(vector<MatchRule*> &rules,
                                       bool innermost,
                                       string filename) {
  set<string> varnames;
  string rr;

  // for each rule, save a printed version; also remember the variable
  // name assignments used
  for (int i = 0; i < rules.size(); i++) {
    Environment gamma;
    // dummy calls to assign variables to the elements of gamma
    rules[i]->query_left_side()->to_string(gamma);
    rules[i]->query_right_side()->to_string(gamma);
    // print the rule
    rr +=
      "  " + print_functionally(rules[i]->query_left_side(), gamma) +
      " -> " + print_functionally(rules[i]->query_right_side(), gamma)
      + "\n";
    // get the variable names out of gamma
    Varset vars = gamma.get_variables();
    for (Varset::iterator it = vars.begin(); it != vars.end(); it++)
      varnames.insert(gamma.get_name(*it));
  }

  // print variables
  string txt = "(VAR";
  for (set<string>::iterator it = varnames.begin();
       it != varnames.end(); it++) {
    txt += " " + (*it);
  }

  // print rules
  txt += ")\n(RULES\n" + rr + ")\n";

  // print strategy
  if (innermost) txt += "(STRATEGY INNERMOST)\n";

  // and write this to a file!
  ofstream ofile(filename.c_str());
  ofile << txt;
  ofile.close();
}

void FirstOrderSplitter :: create_sorted_file(vector<MatchRule*> &rules,
                                              bool innermost,
                                              string filename) {
  vector<PTerm> terms;
  string rr;

  // for each rule, save a printed version, and store its left- and right-hand
  // side in the "terms" vector
  for (int i = 0; i < rules.size(); i++) {
    Environment gamma;
    // dummy calls to assign variables to the elements of gamma
    rules[i]->query_left_side()->to_string(gamma);
    rules[i]->query_right_side()->to_string(gamma);
    // print the rule
    rr +=
      "  " + print_functionally(rules[i]->query_left_side(), gamma) +
      " -> " + print_functionally(rules[i]->query_right_side(), gamma)
      + "\n";
    // store the left- and right-hand-side
    terms.push_back(rules[i]->query_left_side());
    terms.push_back(rules[i]->query_right_side());
  }

  // find all the function symbols occurring in the rules
  map<string,string> symbols;
  for (int j = 0; j < terms.size(); j++) {
    PTerm term = terms[j];
    vector<PTerm> parts = term->split();
    if (parts[0]->query_constant()) {
      string name = parts[0]->to_string(false);
      symbols[name] = "";
      PType t = parts[0]->query_type();
      while (t->query_composed()) {
        symbols[name] += t->query_child(0)->to_string() + " ";
        t = t->query_child(1);
      }
      symbols[name] += "-> " + t->to_string();
    }
    for (int k = 1; k < parts.size(); k++) terms.push_back(parts[k]);
  }

  // build the signature
  string txt = "(SIG \n";
  for (map<string,string>::iterator it = symbols.begin();
       it != symbols.end(); it++) {
    txt += "  (" + it->first + " " + it->second + ")\n";
  }
  // print rules
  txt += ")\n(RULES\n" + rr + ")\n";

  // print strategy
  if (innermost) txt += "(STRATEGY INNERMOST)\n";

  // and write this to a file!
  ofstream ofile(filename.c_str());
  ofile << txt;
  ofile.close();
}

string FirstOrderSplitter :: determine_termination(
                         vector<MatchRule*> &rules,
                         bool innermost,
                         string &reason) {

  return determine_termination_main(rules, innermost, reason);
  
  // might perhaps be useful in the future, but usually not very
  // useful because in non-orthogonal systems, we get the ~PAIR
  // rules anyway!


  set< set<string> > groups = split_types(rules);
  if (groups.size() <= 1)
    return determine_termination_main(rules, innermost, reason);
  cout << "rules has size: " << rules.size() << endl;
  
  // we have multiple groups!
  reason = "We observe that there is a complete split in the types, "
    "so by " + wout.cite("Kop13:2") + " it suffices if each "
    "sort-bound part is separately terminating.\n\n";
  for (set< set<string> >::iterator it = groups.begin();
       it != groups.end(); it++) {
    set<string> group = *it;
    cout << "got group" << endl;
    vector<MatchRule*> R;
    bool pairpresent = false;
    for (int i = 0; i < rules.size(); i++) {
      PTerm left = rules[i]->query_left_side();
      string d =
        dynamic_cast<DataType*>(left->query_type())->query_constructor();
      if (group.find(d) != group.end() ||
          left->query_head()->to_string() == "~PAIR") {
        R.push_back(rules[i]);
        cout << "included: " << rules[i]->to_string() << endl;
      }
    }
    cout << "R has size: " << R.size() << endl;
    string subreason;
    string subresult = determine_termination_main(R, innermost, subreason);
    if (subresult == "YES") reason += subreason + "\n";
    else {
      cout << "it didn't work :(" << endl;
      cout << "subresult = " << subresult << endl;
      reason = subreason;
      return subresult;
    }
  }
  cout << "returning YES!" << endl;
  return "YES";
}

static int COUNTER = 0;

string get_unique_file() {
  string ret = to_string(COUNTER);
  COUNTER++;
  return "sortedfiles/" + ret + ".mstrs";
}

string FirstOrderSplitter :: determine_termination_main(
                         vector<MatchRule*> &rules,
                         bool innermost,
                         string &reason) {
  /* the following can be used to just store the first-order part of a system
  create_sorted_file(rules, innermost, get_unique_file());
  return "MAYBE";
  */

  create_file(rules, innermost, "resources/system.trs");

  system(("./resources/" + fotool + " resources/system.trs 50 > "
         "resources/result").c_str());

  // get results
  ifstream ifile("resources/result");
  if (ifile.eof()) {
    reason = "First-order termination prover did not provide a result.\n";
    return "MAYBE";
  }
  string result;
  getline(ifile, result);
  if (result == "") {
    reason = " || First-order termination tool did not provide a result.\n";
    result = "MAYBE";
  }
  while (!ifile.eof()) {
    string input;
    getline(ifile, input);
    reason += " || " + input + "\n";
  }
  system("rm resources/result");

  return result;
}

bool FirstOrderSplitter :: valid_counterexample(string example,
                                                Alphabet &F) {
  example.erase(std::remove (example.begin(), example.end(), ' '),
                example.end());

  InputReaderFO reader;
  PTerm term = reader.parse_term(example, F);
  if (term != NULL) {
    delete term;
    return true;
  }
  return false;
}

void FirstOrderSplitter :: get_constant_data(PTerm term, Alphabet &F) {
  for (int i = 0; i < term->number_children(); i++) {
    get_constant_data(term->get_child(i), F);
  }

  if (term->query_constant()) {
    PConstant f = dynamic_cast<PConstant>(term);
    if (!F.contains(f->query_name())) {
      F.add(f->query_name(), f->query_type()->copy());
    }
  }
}

bool FirstOrderSplitter :: single_sorted_alphabet(Alphabet &F) {
  vector<string> symbols = F.get_all();
  set<string> types;
  for (int i = 0; i < symbols.size(); i++) {
    PType type = F.query_type(symbols[i]);
    while (type->query_composed()) type = type->query_child(1);
    types.insert(type->to_string());
  }
  return types.size() == 1;
}

string FirstOrderSplitter :: determine_nontermination(
                         vector<MatchRule*> &rules,
                         bool innermost,
                         string &reason) {

  create_file(rules, innermost, "resources/system.trs");

  system(("./resources/" + fonontool + " resources/system.trs 50 > "
         "resources/result").c_str());

  // get result
  ifstream ifile("resources/result");
  if (ifile.eof()) return "MAYBE";
  string result;
  getline(ifile, result);
  if (result != "NO") return result;

  // get relevant alphabet and arities
  Alphabet F;
  for (int i = 0; i < rules.size(); i++) {
    get_constant_data(rules[i]->query_left_side(), F);
    get_constant_data(rules[i]->query_right_side(), F);
  }

  // in some cases first-order non-termination immediately implies higher-order
  // non-termination
  if (single_sorted_alphabet(F) ||
      (manip.left_linear(rules) && !manip.has_critical_pairs(rules))) {
    while (!ifile.eof()) {
      string input;
      getline(ifile, input);
      reason += " || " + input + "\n";
    }
    system("rm resources/result");
    return "NO";
  }

  // get counterexample and check its validity
  string counterexample;
  getline(ifile, counterexample);
  if (!valid_counterexample(counterexample, F)) {
    reason = "We could prove non-termination, but it is not clear "
      "whether there is a well-typed non-terminating term.";
    return "MAYBE";
  }

  reason = " || The following well-typed term is terminating: " +
    counterexample + "\n || \n";

  // get comments
  while (!ifile.eof()) {
    string input;
    getline(ifile, input);
    reason += " || " + input + "\n";
  }
  system("rm resources/result");

  // and return with success!
  return result;
}

string FirstOrderSplitter :: query_tool() {
  return fotool;
}

string FirstOrderSplitter :: query_tool_name() {
  return fotool;
}

string FirstOrderSplitter :: query_non_tool() {
  return fonontool;
}

string FirstOrderSplitter :: query_non_tool_name() {
  return fonontool;
}

