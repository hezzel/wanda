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

#include "inputreaderafs.h"
#include "substitution.h"
#include "afs.h"
#include <fstream>
#include <iostream>

int InputReaderAFS :: read_file(string filename, Alphabet &F,
                                vector<MatchRule*> &rules) {
  
  ifstream file(filename.c_str());
  
  string txt;
  while (!file.eof()) {
    string input;
    getline(file, input);
    txt += input + "\n";
  }

  return read_text(txt, F, rules);
}

int InputReaderAFS :: read_text(string txt, Alphabet &F,
                                vector<MatchRule*> &rules) {
  
  MonomorphicAFS *afs = read_afs(txt);
  if (afs == NULL) return 1;
  
  string reason;
  afs->pretty_print();
  if (afs->trivially_terminating(reason)) {delete afs; return 3;}
  if (afs->trivially_nonterminating(reason)) {delete afs; return 4;}

  afs->normalise_rules();
  afs->to_afsm(F, rules);
  delete afs;
  return 0;
}

MonomorphicAFS *InputReaderAFS :: read_afs(string txt) {
  Alphabet Sigma;
  ArList arities;
  Environment Gamma;
  vector<PTerm> left, right;
  
  // split in lines
  vector<string> lines;
  int k;
  while ((k = txt.find('\n')) != string::npos) {
    lines.push_back(txt.substr(0,k));
    txt = txt.substr(k+1);
  }
  lines.push_back(txt);
  
  // read the constants, variables and rules
  for (k = 0; k < lines.size() && lines[k] != ""; k++) {
    if (!read_constant(lines[k], Sigma, arities)) return NULL;
  }
  
  if (k == lines.size()) {
    last_warning = "Variable and rules parts are missing.";
    return NULL;
  }
  
  for (k++; k < lines.size() && lines[k] != ""; k++) {
    if (!read_variable(lines[k], Gamma)) return NULL;
  }
  
  if (k == lines.size()) {
    last_warning = "Variable and rules parts are missing.";
    return NULL;
  }
  
  for (k++; k < lines.size() && lines[k] != ""; k++) {
    if (!read_rule(lines[k], Sigma, Gamma, arities, left, right)) {
      for (int i = 0; i < left.size(); i++) {
        delete left[i];
        delete right[i];
      }
      return NULL;
    }
  }

  return new MonomorphicAFS(Sigma, Gamma, left, right, arities);
}

bool InputReaderAFS :: read_constant(string line, Alphabet &Sigma,
                                      ArList &arities) {
  // start: do we have a colon, to separate name and type?
  int colon = line.find(':');
  if (colon == string::npos) {
    last_warning = "Missing colon in function declaration [" + line + "]";
    return false;
  }
    
  // is the thing before the colon a single word and legal name?
  string name = line.substr(0,colon);
  remove_outer_spaces(name);
  if (name.length() == 0) {
    last_warning = "Empty name in function declaration [" + line + "]";
    return false;
  }
  for (int i = 0; i < name.length(); i++) {
    if (!generic_character(name[i])) {
      last_warning = "Illegal characters in constant name [" + name + "]";
      return false;
    }
  }
    
  // is it unknown?
  if (Sigma.contains(name)) {
    last_warning = "Duplicate constant: " + name;
    return false;
  }

  // is the thing after it a legal type declaration?
  string typetxt = line.substr(colon+1);
  vector<PType> types = TYPEDEC(typetxt);
  if (types.size() == 0) {
    last_warning = "Could not read type declaration " + typetxt +
                   ": " + last_warning;
    return false;
  }
    
  // looks good, create the entry
  PType type = types[0];
  for (int i = types.size()-1; i > 0; i--)
    type = new ComposedType(types[i], type);
  arities[name] = types.size()-1;
  Sigma.add(name, type);

  return true;
}

bool InputReaderAFS :: read_variable(string line, Environment &Gamma) {
  // start: do we have a colon, to separate name and type?
  int colon = line.find(':');
  if (colon == string::npos) {
    last_warning = "Missing colon in variable declaration [" + line + "]";
    return false;
  }
    
  // is the thing before the colon a single word and legal name?
  string name = line.substr(0,colon);
  remove_outer_spaces(name);
  if (name.length() == 0) {
    last_warning = "Empty name in variable declaration [" + line + "]";
    return false;
  }
  for (int i = 0; i < name.length(); i++) {
    if (!generic_character(name[i])) {
      last_warning = "Illegal characters in constant name [" + name + "]";
      return false;
    }
  }
    
  // is it unknown?
  if (Gamma.lookup(name) != -1) {
    last_warning = "Duplicate variable: " + name;
    return false;
  }

  // is the thing after it a legal type?
  string typetxt = line.substr(colon+1);
  PType type = TYPE(typetxt);
  if (type == NULL) {
    last_warning = "Could not read type " + typetxt +
                   ": " + last_warning;
    return false;
  }
    
  // looks good, create the entry
  PVariable v = new Variable(type);
  Gamma.add(v, name);
  delete v;

  return true;
}

bool InputReaderAFS :: read_rule(string line, Alphabet &Sigma,
                                 Environment &Gamma, ArList &arities,
                                 vector<PTerm> &lhs, vector<PTerm> &rhs) {
  // find separating arrow
  int arrow = line.find("=>");
  if (arrow == string::npos) {
    last_warning = "missing =>.";
    return false;
  }

  // can we make a rule out of this?
  string left = remove_whitespace(line.substr(0, arrow));
  string right = remove_whitespace(line.substr(arrow+2));
  PTerm l, r;
  l = read_term(left, Sigma, Gamma, arities);
  if (l == NULL) return false;
  r = read_term(right, Sigma, Gamma, arities);
  if (r == NULL) { delete l; return false; }
  
  // is it a valid rule?
  Varset FVr = r->free_var();
  Varset FVl = l->free_var();

  if (!FVl.contains(FVr)) {
    last_warning = "Illegal rule: " + line + "\nVariables from the "
                   "right hand side should also occur in the left.";
    return false;
  }

  // all good -- add it and abort
  lhs.push_back(l);
  rhs.push_back(r);
  
  return true;
}

string InputReaderAFS :: remove_whitespace(string txt) {
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

PTerm InputReaderAFS :: read_term(string desc, Alphabet &Sigma,
                                  Environment &Gamma, ArList &ars) {

  remove_outer_brackets(desc);

  if (desc == "") {last_warning = "Empty term."; return NULL;}
  if (desc[0] == '/')
    return read_abstraction(desc, Sigma, Gamma, ars);
  if (find_substring(desc, "*") != -1)
    return read_application(desc, Sigma, Gamma, ars);
  if (desc.find('(') != string::npos)
    return read_functional(desc, Sigma, Gamma, ars);

  // it contains no brackets or application => it must be a single
  // symbol (either variable or function symbol
  if (Sigma.contains(desc)) {
    if (ars[desc] != 0) {
      last_warning = "Illegal function application: " + desc + " -- "
        "not enough parameters.";
      return NULL;
    }
    return Sigma.get(desc);
  }
  if (Gamma.lookup(desc) != -1) {
    int k = Gamma.lookup(desc);
    PType t = Gamma.get_type(k)->copy();
    return new Variable(t,k);
  }

  last_warning = "Could not parse " + desc + "!";
  return NULL;
}

PTerm InputReaderAFS :: read_abstraction(string desc, Alphabet &Sigma,
                                    Environment &Gamma, ArList &ars) {
  // check the format 
  if (desc.length() < 7) {
    last_warning = "Illegal term: " + desc + "\n  Starts " +
      "with /, but too short to be an abstraction!";
    return NULL;
  }
  if (desc[1] != '\\') {
    last_warning = "Illegal term: " + desc + "\n  Half of " +
      "the lambda operator /\\.";
    return NULL;
  }

  // find the dot, or comma as the case may be
  int dot = desc.find('.');
  if (dot == string::npos) {
    last_warning = "Illegal term: " + desc + "\n  " +
      "Abstraction without separating period.";
    return NULL;
  }
  int comma = desc.find(',');
  if (comma != string::npos && comma < dot) {
    string sub1 = desc.substr(0,comma);
    string sub2 = desc.substr(comma+1);
    desc = sub1 + "./\\" + sub2;
    dot = comma;
  }

  // find the colon for type denotation
  int colon = desc.find(':');
  if (colon == string::npos || colon > dot) {
    last_warning = "Illegal abstraction: " + desc + "\n  No type "
      "denotation given for the variable.";
    return NULL;
  }

  // split up in parts
  string varpart = desc.substr(2,colon-2);
  string typepart = desc.substr(colon+1,dot-colon-1);
  string remainder = desc.substr(dot+1);

  // see whether the varpart satisfies the requirements of a variable
  for (int i = 0; i < varpart.length(); i++) {
    if (!generic_character(varpart[i])) {
      last_warning = "Illegal abstraction: " + desc + "\n  " +
        "Not a legal variable name, " + varpart;
      return NULL;
    }
  }

  PType type = TYPE(typepart);
  if (type == NULL) return NULL;

  // add the new variable to gamma
  PType backup = NULL;
  int k = Gamma.lookup(varpart);
  if (k != -1) {
    backup = Gamma.get_type(k)->copy();
    Gamma.remove(k);
  }
  PVariable x = new Variable(type);
  Gamma.add(x, varpart);

  // parse the subterm
  PTerm s = read_term(remainder, Sigma, Gamma, ars);

  // remove the new variable from gamma
  Gamma.remove(x);
  if (backup != NULL) {
    Gamma.add(k, backup, varpart);
  }

  if (s == NULL) { delete x; return NULL; }

  return new Abstraction(x, s);
}

PTerm InputReaderAFS :: read_application(string desc, Alphabet &Sigma,
                                    Environment &Gamma, ArList &ars) {
  string description = desc;

  // split in parts  
  vector<string> parts;
  int k = find_substring(desc, "*");
  while (k != -1) {
    parts.push_back(desc.substr(0,k));
    desc = desc.substr(k+1);
    k = find_substring(desc, "*");
  }
  parts.push_back(desc);

  int i;
  // parse each of the individual parts
  vector<PTerm> tparts;
  for (i = 0; i < parts.size(); i++) {
    PTerm term = read_term(parts[i], Sigma, Gamma, ars);
    if (term == NULL) {
      for (int j = 0; j < i; j++) delete tparts[j];
      return NULL;
    }
    tparts.push_back(term);
  }

  // check typing
  PType t0 = tparts[0]->query_type();
  for (i = 1; i < tparts.size(); i++) {
    if (!t0->query_composed() ||
        !t0->query_child(0)->equals(tparts[i]->query_type())) {
      last_warning = "Application not well-typed: " + description;
      for (int j = 0; j < tparts.size(); j++) delete tparts[j];
      return NULL;
    }
    t0 = t0->query_child(1);
  }

  // make an application out of it!
  PTerm ret = tparts[0];
  for (i = 1; i < tparts.size(); i++)
    ret = new Application(ret, tparts[i]);

  return ret;
}

PTerm InputReaderAFS :: read_functional(string desc, Alphabet &Sigma,
                                   Environment &Gamma, ArList &ars) {

  int a = desc.find('(');
  int b = find_matching_bracket(desc, a);
  if (b != desc.size()-1) {
    last_warning = "Cannot parse " + desc + "!";
    return NULL;
  }

  // is the head a valid function symbol?
  string name = desc.substr(0,a);
  if (!Sigma.contains(name)) {
    last_warning = "Cannot parse " + desc + ": not a valid "
      "function symbol, " + name;
    return NULL;
  }

  // get the arguments
  string args = desc.substr(a+1,b-a-1);
  vector<string> parts;
  int k = find_substring(args, ",");
  while (k != -1) {
    parts.push_back(args.substr(0,k));
    args = args.substr(k+1);
    k = find_substring(args, ",");
  }
  parts.push_back(args);

  if (parts.size() != ars[name]) {
    last_warning = "Illegal function application " + desc + ":\n  "
      "Number of arguments does not correspond with given arity.";
    return NULL;
  }

  // parse each of the individual parts
  int i;
  vector<PTerm> tparts;
  for (i = 0; i < parts.size(); i++) {
    PTerm term = read_term(parts[i], Sigma, Gamma, ars);
    if (term == NULL) {
      for (int j = 0; j < i; j++) delete tparts[j];
      return NULL;
    }
    tparts.push_back(term);
  }

  // check typing
  PType t0 = Sigma.query_type(name);
  for (i = 0; i < tparts.size(); i++) {
    if (!t0->query_composed() ||
        !t0->query_child(0)->equals(tparts[i]->query_type())) {
      last_warning = "Function application not well-typed: " + desc;
      for (int j = 0; j < tparts.size(); j++) delete tparts[j];
      return NULL;
    }
    t0 = t0->query_child(1);
  }

  // make an application out of it
  PTerm ret = Sigma.get(name);
  for (i = 0; i < tparts.size(); i++)
    ret = new Application(ret, tparts[i]);

  return ret;
}

