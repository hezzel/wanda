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

#include "inputreaderatrs.h"
#include "outputmodule.h"
#include "substitution.h"
#include "typesubstitution.h"
#include <fstream>

int InputReaderATRS :: read_file(string filename, Alphabet &Sigma,
                                  vector<MatchRule*> &rules) {

  wout.print("Applicative TRSs are not interesting for WANDA, so we "
    "consider termination of the dynamically typed AFS "
    "corresponding to this ATRS.\n");
  MonomorphicAFS *afs = read_as_afs(filename);

  if (afs == NULL) return 1;
  afs->pretty_print();
  if (afs->trivially_terminating(last_warning)) return 3;
  if (afs->trivially_nonterminating(last_warning)) return 4;

  afs->normalise_rules();
  afs->to_afsm(Sigma, rules);
  delete afs;

  return 0;
}

MonomorphicAFS *InputReaderATRS :: read_as_afs(string filename) {
  // read the file into vstring (vars) and rstring (rules)
  string contents = readf(filename);
  if (contents == "") return NULL;
  string vstring = get_part(contents, "VAR");
  string rstring = get_part(contents, "RULES");
  if (rstring == "") {
    last_warning = "RULES part of file missing or strange.";
    return NULL;
  }

  // split these respective parts into "atoms"
  vector<string> vars, left, right;
  if (!read_vars(vstring, vars)) return NULL;
  if (!read_rules(rstring, left, right)) return NULL;

  // we will convert the entire system into a single term, to be
  // typed at once; to make sure the function symbols get the same
  // type everywhere, they are parsed as variables too
  vector<PPartTypedTerm> lhs, rhs;
  Naming consnames;
  Alphabet F;
  parse_rules(left, right, lhs, rhs, vars, consnames);
  PPartTypedTerm system = make_single_term(lhs, rhs, F);
  PTerm tsystem = type_term(system, F);
  if (tsystem == NULL) return NULL;

  // from this information, make an AFS
  Alphabet Sigma;
  Environment G;
  vector<PTerm> l, r;
  get_alphabet(tsystem, consnames, Sigma);
  get_environment(tsystem, G);
  get_rules(tsystem, l, r);

  return new MonomorphicAFS(Sigma, G, l, r);
}

string InputReaderATRS :: readf(string filename) {
  ifstream fp_in(filename.c_str());
  string contents;

  int empty = 0;
  while (!fp_in.eof() && empty < 100) {
    string input;
    getline(fp_in, input);
    if (input == "") empty++; else empty = 0;
    contents += input + "\n";
  }
  fp_in.close();
  fix_newlines(contents);
  normalise(contents);

  if (contents == "") {
    last_warning = "Could not read file " + filename + " (or file "
                   "is empty).";
  }

  return contents;
}

string InputReaderATRS :: get_part(string txt, string part) {
  int k = txt.find("(" + part);
  if (k == -1) return "";
  int j = find_matching_bracket(txt, k);
  if (j == -1) return "";
  int l = part.length()+1;
  return txt.substr(k+l,j-k-l);
}

bool InputReaderATRS :: read_vars(string txt, vector<string> &vars) {
  remove_outer_spaces(txt);
  if (txt == "") return true;
  int k = txt.find(" ");
  if (k == string::npos) { vars.push_back(txt); return true; }
  vars.push_back(txt.substr(0,k));
  read_vars(txt.substr(k+1), vars);
  return true;
}

bool InputReaderATRS :: read_rules(string txt, vector<string> &left,
                                   vector<string> &right) {
  remove_outer_spaces(txt);
  int k = txt.find("\n");
  while (k != string::npos) {
    if (!read_rule(txt.substr(0,k), left, right)) return false;
    txt = txt.substr(k+1);
    k = txt.find("\n");
  }
  if (!read_rule(txt, left, right)) return false;
  return true;
}

bool InputReaderATRS :: read_rule(string txt, vector<string> &left,
                                  vector<string> &right) {
  remove_outer_spaces(txt);
  if (txt == "") return true;
  
  int k = txt.find("->");
  if (k == string::npos) {
    last_warning = "Illegal rule: " + txt + ".";
    return false;
  }
  string sleft = txt.substr(0,k);
  string sright = txt.substr(k+2);
  normalise(sleft);
  normalise(sright);
  left.push_back(sleft);
  right.push_back(sright);

  return true;
}

bool InputReaderATRS :: parse_rules(vector<string> &sl,
                                    vector<string> &sr,
                                    vector<PPartTypedTerm> &pl,
                                    vector<PPartTypedTerm> &pr,
                                    vector<string> &vars,
                                    Naming &cnames) {

  for (int i = 0; i < sl.size(); i++) {
    // give all variables a unique name (we do this over and over
    // again, because the same variable may occur in different rules
    // with a different type
    Naming v;
    for (int j = 0; j < vars.size(); j++) v[vars[j]] = new_index();
    PPartTypedTerm a = parse_atrs_term(sl[i], cnames, v);
    PPartTypedTerm b = parse_atrs_term(sr[i], cnames, v);

    // error handling if one of the sides could not be parsed
    if (a == NULL || b == NULL) {
      if (a != NULL) delete a;
      if (b != NULL) delete b;
      for (int k = 0; k < pl.size(); k++) {
        delete pl[k];
        delete pr[k];
      }
      return false;
    }

    pl.push_back(a);
    pr.push_back(b);
  }

  return true;
}

PTerm InputReaderATRS :: type_term(PPartTypedTerm term, Alphabet &F) {
  Typer typer;
  PTerm ret = typer.type_term(term, F);
  delete term;
  if (ret == NULL) {
    last_warning = "Cannot find a monomorphic typing.\n  " +
      typer.query_warning();
    return NULL;
  }

  // replace type variables by monotonic types
  Varset FTV = ret->free_typevar();
  TypeSubstitution sub;
  Varset::iterator it;
  string start = "T"; char k = 'a';
  for (it = FTV.begin(); it != FTV.end(); it++) {
    string name = start; name[start.length()-1] = k;
    sub[*it] = new DataType(name);
    if (k == 'z') {
      start += "T";
      k = 'a';
    }
    //else k++;
  }
  ret->apply_type_substitution(sub);

  return ret;
}

PPartTypedTerm InputReaderATRS :: parse_atrs_term(string txt, Naming
                                                  &constants, Naming &vars) {
  normalise(txt);
  if (txt == "") {
    last_warning = "Asked to parse an empty string.";
    return NULL;
  }
  PPartTypedTerm ret = new PartTypedTerm;
  
  int k = txt.find('(');
  
  // a string not containing brackets is either a variable or a constant
  if (k == string::npos) {
    // it's a variable => encode it with a meta-variable
    if (vars.find(txt) != vars.end()) {
      ret->name = "variable";
      ret->idata = vars[txt];
    }
    // it's a constant => encode it with a variable
    else {
      ret->name = "variable";
      if (constants.find(txt) == constants.end())
        constants[txt] = new_index();
      ret->idata = constants[txt];
    }
    return ret;
  }
  
  // it's an application - find its arguments
  string args = txt.substr(k);
  remove_outer_spaces(txt);
  if (args == "" || args[0] != '(') {
    last_warning = "Parsing " + txt + " failed: app without arguments.";
    return NULL;
  }
  k = find_matching_bracket(args, 0);
  int j = find_substring(args.substr(1), ",") + 1;
  if (k != args.length()-1 || j == -1) {
    last_warning = "Parsing " + txt + " failed: does not have form "
      "app(..,..)";
    return NULL;
  }

  string part1 = args.substr(1,j-1);
  string part2 = args.substr(j+1,k-j-1);

  // convert the arguments
  PPartTypedTerm child1 = parse_atrs_term(part1, constants, vars);
  if (child1 == NULL) return NULL;
  PPartTypedTerm child2 = parse_atrs_term(part2, constants, vars);
  if (child2 == NULL) {
    delete child1;
    return NULL;
  }

  ret->name = "application";
  ret->children.push_back(child1);
  ret->children.push_back(child2);
  return ret;
}

void InputReaderATRS :: get_alphabet(PTerm &system, Naming &cnames,
                                      Alphabet &F) {
  Substitution sub;

  for (Naming::iterator it = cnames.begin(); it != cnames.end(); it++) {
    int k = it->second;
    string name = fix_name(it->first);
    PType type = system->lookup_type(k);
    if (type == NULL) continue;
    sub[k] = new Constant(name, type->copy());
    F.add(name, type->copy());
  }

  system = system->apply_substitution(sub);
}

void InputReaderATRS :: get_environment(PTerm system, Environment &G) {
  Varset all = system->free_var();
  for (Varset::iterator it = all.begin(); it != all.end(); it++) {
    int k = *it;
    PType type = system->lookup_type(k);
    if (type != NULL) G.add(k, type->copy());
  }
}

void InputReaderATRS :: get_rules(PTerm &system, vector<PTerm> &lhs,
                                  vector<PTerm> &rhs) {
  // get the rules out - system has a form like !PAIR rule1 (!PAIR rule2 !NIL)
  while (system->to_string(false,false) != "!NIL") {
    PTerm rule = system->replace_subterm(NULL, "12");
    PTerm rest = system->replace_subterm(NULL, "2");
    delete system;
    system = rest;
    
    // rule has the form !RULE left right
    PTerm left = rule->replace_subterm(NULL, "12");
    PTerm right = rule->replace_subterm(NULL, "2");
    delete rule;

    // save them!
    lhs.push_back(left);
    rhs.push_back(right);
  }
  system = NULL;
}

void InputReaderATRS :: fix_newlines(string &text) {
  for (int i = 0; i < text.length(); i++)
    if (text[i] == '\r') text[i] = '\n';
}

int InputReaderATRS :: new_index() {
  PVariable v = new Variable(new DataType("a"));
  int ret = v->query_index();
  delete v;
  return ret;
}

