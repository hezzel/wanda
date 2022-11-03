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

#include "textconverter.h"
#include "environment.h"
#include "typesubstitution.h"
#include "typer.h"

// removes all outer spaces
void TextConverter :: remove_outer_spaces(string &description) {
  while (description.length() > 0 && (description[0] == ' ' || description[0] == 9)) {
    description = description.substr(1);
  }   
  while (description.length() > 0 && (description[description.length()-1] == ' ' || description[description.length()-1] == 9)) {
    description = description.substr(0, description.length()-1);
  }   
}

void TextConverter :: remove_outer_brackets(string &description) {
  // check for, and remove, outer brackets
  int n = description.length();
  if (n == 0) return;
  if (description[0] == '(' && description[n-1] == ')') {
    bool outer = true;
    int Nbrac = 1;
    for (int i = 1; i < n-1; i++) {
      if (description[i] == '(') Nbrac++;
      if (description[i] == ')') Nbrac--;
      if (Nbrac == 0) {outer = false; break;}
    }   
    if (outer) {
      description = description.substr(1, n-2);
      remove_outer_brackets(description);
    }   
  }   
}

void TextConverter :: normalise(string &description) {
  int n = description.length()+1;

  while (description.length() != n) {
    n = description.length();
    remove_outer_spaces(description);
    remove_outer_brackets(description);
  }
}

bool TextConverter :: check_correct_bracket_matching(string description) {
  int n = description.length();
  int Nbrac = 0;

  for (int i = 0; i < n; i++) {
    if (description[i] == '(') Nbrac++;
    if (description[i] == ')') {
      if (Nbrac == 0) {
        last_warning = "Incorrect bracket matching!";
        return false;
      }
      Nbrac--;
    }
  }
  if (Nbrac != 0) {
    last_warning = "Missing closing bracket!";
    return false;
  }
  return true;
}

int TextConverter :: find_matching_bracket(string description,
                                           int start, int dir) {
  if (description[start] != '(' && description[start] != ')' &&
      description[start] != '[' && description[start] != ']' &&
      description[start] != '{' && description[start] != '}') {
    return -1;
  }
  
  char a = description[start];
  char b;
  if (a == '(') b = ')';
  if (a == ')') b = '(';
  if (a == '[') b = ']';
  if (a == ']') b = '[';
  if (a == '{') b = '}';
  if (a == '}') b = '{';
  
  int counter = 1;
  for (int k = start+dir; 0 <= k && k < description.length();
       k += dir) {
    if (description[k] == a) counter++;
    if (description[k] == b) {
      counter--;
      if (counter == 0) return k;
    }
  }
  last_warning = "Unmatched bracket in " + description + ".";
  return -1;
}

bool TextConverter :: contains(string description, char c) {
  int n = description.length();
  for (int i = 0; i < n; i++) {
    if (description[i] == c) return true;
  }
  return false;
}

int TextConverter :: find_substring(string description, string sub) {
  int n = description.length();
  int Nbrac = 0;

  if (sub.length() == 0) return 0;

  for (int i = 0; i < n; i++) {
    if (description[i] == '(' || description[i] == '[' ||
        description[i] == '{') Nbrac++;
    if (description[i] == ')' || description[i] == ']' ||
        description[i] == '}') Nbrac--;
    if (description[i] == sub[0] && Nbrac == 0) {
      if (description.substr(i,sub.length()) == sub) return i;
    }
  }
  return -1;
}

bool TextConverter :: generic_character(char c) {
  return c == '!' || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
                  || ('0' <= c && c <= '9');
}


string TextConverter :: query_warning() {
  string ret = last_warning;
  last_warning = "";
  return ret;
}

PType TextConverter :: convert_type(string &description,
                                    Naming &typeenv) {
  PType type1, type2;
  string desc1, desc2;

  normalise(description);
  
  if (description == "") {
    last_warning = "Asked to convert empty type.";
    return NULL;
  }
  
  /* read the first part */
  
  // if the type starts with a bracket, simply read until the closing
  // bracket
  if (description[0] == '(') {
    int k = find_matching_bracket(description, 0);
    if (k == -1) return NULL;
    desc1 = description.substr(1,k-1);
    desc2 = description.substr(k+1);
    type1 = convert_type(desc1, typeenv);
    if (type1 == NULL) return NULL;
  }
  // if the type starts with a dollar, it has to be a type variable
  else if (description[0] == '$') {
    int k = 1;
    while (k < description.length() &&
           generic_character(description[k])) k++;
    if (k == 1) {
      last_warning = "Illegal type construction " + description +
                     ": $ should only occur as the head of a type " +
                     "variable.";
      return NULL;
    }
    desc1 = description.substr(0,k);
    if (k == description.length()) desc2 = "";
    else desc2 = description.substr(k);
    if (typeenv.find(desc1) != typeenv.end()) {
      type1 = new TypeVariable(typeenv[desc1]);
    } else {
      TypeVariable *tv = new TypeVariable();
      typeenv[desc1] = tv->query_index();
      type1 = tv;
    }
  }
  // otherwise, it's a data type - find the type constructor and
  // possibly subtypes
  else if (generic_character(description[0])) {
    int k = 1;
    while (k < description.length() &&
           generic_character(description[k])) k++;
    string constructor = description.substr(0,k);
    desc2 = description.substr(k);
    remove_outer_spaces(desc2);
    if (desc2 != "" && desc2[0] == '(') {
      k = find_matching_bracket(desc2,0);
      if (k == -1) return NULL;
      desc1 = desc2.substr(1,k-1);
      desc2 = desc2.substr(k+1);
      // find the subterms
      remove_outer_spaces(desc1);
      vector<PType> subtypes;
      while (desc1 != "") {
        PType tt = convert_type(desc1,typeenv);
        if (tt == NULL) {
          for (k = 0; k < subtypes.size(); k++) delete subtypes[k];
          return NULL;
        }
        subtypes.push_back(tt);
        remove_outer_spaces(desc1);
        if (desc1 == "") break;
        if (desc1[0] == ',') desc1 = desc1.substr(1);
        else {
          last_warning = "Illegal type construction " + description
                       + ": cannot parse part between brackets.";
          return NULL;
        }
      }
      type1 = new DataType(constructor, subtypes);
    }
    else type1 = new DataType(constructor);
  }
  else {  // string doesn't start with a legal character, no idea what this is
    last_warning = "Illegal type construction " + description + ": "+
                   "unexpected character " + string(1,description[0])
                   +".";
    return NULL;
  }
  
  /* if we reached the end of the string, or a comma separator,
     just return what we found */
     
  remove_outer_spaces(desc2);
  if (desc2 == "" || desc2[0] == ',') {
    description = desc2;
    return type1;
  }
  
  /* and if we found an arrow symbol, parse the rest of the type */
  
  if (desc2.substr(0,2) == "->") {
    description = desc2.substr(2);
    type2 = convert_type(description, typeenv);
    if (type2 == NULL) {
      delete type1;
      return NULL;
    }
    return new ComposedType(type1, type2);
  }

  /* something strange is going on! */
  
  last_warning = "Illegal type construction " + description + ".";
  delete type1;
  return NULL;
}

vector<PType> TextConverter :: convert_typedeclaration(string
                                         &description, Naming &env) {
  vector<PType> ret;
  
  if (!check_correct_bracket_matching(description)) return ret;
  int k = find_substring(description, "-->");
  
  // no separating -->, so it must be a single type
  if (k == -1) {
    PType p = convert_type(description, env);
    if (p == NULL) {
      last_warning = "Type declaration " + description + " has no "
        "--> deliminator, and parsing as type failed for the "
        "following reason:\n" + last_warning;
      return ret;
    }
    ret.push_back(p);
    return ret;
  }

  // find the separate pieces
  string inp = description.substr(0,k);
  string outp = description.substr(k+3);
  normalise(inp);
  vector<string> types;
  types.push_back(outp);
  k = find_substring(inp, "*");
  while (k != -1) {
    string start = inp.substr(0,k);
    types.push_back(start);
    inp = inp.substr(k+1);
    k = find_substring(inp, "*");
  }
  types.push_back(inp);

  // and parse them
  for (int i = 0; i < types.size(); i++) {
    PType p = convert_type(types[i], env);
    if (p == NULL) {
      for (int j = 0; j < i; j++) delete ret[j];
      ret.clear();
      return ret;
    }
    ret.push_back(p);
  }
  
  return ret;
}

PType TextConverter :: TYPE(string description) {
  if (!check_correct_bracket_matching(description)) return NULL;
  Naming tmp;
  return convert_type(description, tmp);
}

vector<PType> TextConverter :: TYPEDEC(string description) {
  Naming tmp;
  return convert_typedeclaration(description, tmp);
}

void TextConverter :: split(string description, string &A, string &B,
                            int &endpos) {
  A = description;
  B = "";
  endpos = description.length();
  
  int k = description.find("_{");
  if (k == string::npos) return;

  int j = description.find("}", k);
  if (j == string::npos) {
    last_warning = "Unterminated type denotation in " + description;
    endpos = -1;
    return;
  }

  A = description.substr(0,k);
  B = description.substr(k+2,j-k-2);
  endpos = j+1;
}

PPartTypedTerm TextConverter :: check_constant(string description,
                          Alphabet &Sigma, Naming &Theta, bool &ok) {
  // is it a constant without type denotation
  if (Sigma.contains(description)) {
    PPartTypedTerm ret = new PartTypedTerm;
    ret->name = "constant";
    ret->sdata = description;
    ret->type = NULL;
    return ret;
  }

  // it's not; see whether it's a constant -with- type denotation
  string consname, denotation;
  int endpos;
  split(description, consname, denotation, endpos);
  if (endpos == -1) {ok = false; return NULL;}
  if (endpos != description.length()) return NULL;

  if (denotation == "") return NULL;
  if (!Sigma.contains(consname)) return NULL;

  // it is; but is this a legal type?
  PType typing = convert_type(denotation, Theta);
  if (typing == NULL) {
    last_warning = "Could not parse type denotation in " +
      description + ":\n  " + last_warning;
    ok = false;
    return NULL;
  }

  PType original = Sigma.query_type(consname);
  TypeSubstitution sub;
  if (!original->instantiate(typing, sub)) {
    last_warning = "Type error: type denotation in " + description +
        " does not match type in alphabet.";
    ok = false;
    delete typing;
    return NULL;
  }

  // all good - save the result in an PartTypedTerm
  PPartTypedTerm ret = new PartTypedTerm;
  ret->name = "constant";
  ret->sdata = consname;
  ret->type = typing;
  return ret;
}

PPartTypedTerm TextConverter :: check_variable(string description,
                     Naming &Gamma, Naming &Theta, bool &ok) {

  // is this something we know?
  if (Gamma.find(description) != Gamma.end()) {
    PPartTypedTerm ret = new PartTypedTerm;
    ret->name = "variable";
    ret->idata = Gamma[description];
    ret->type = NULL;
    return ret;
  }

  // see whether we can split it
  string varname, denotation;
  int endpos;
  split(description, varname, denotation, endpos);
  if (endpos == -1) {ok = false; return NULL;}
  if (endpos != description.length()) return NULL;

  // is the varname actually a legal varname?
  bool legalvar = true;
  for (int i = 0; i < varname.length(); i++) {
    if (!generic_character(varname[i]) &&
        !(i == 0 && varname[i] == '%')) legalvar = false;
  }
  if (!legalvar) return NULL;

  // can we parse the type?
  PType typing = NULL;
  if (denotation != "") {
    typing = convert_type(denotation, Theta);
    if (typing == NULL) {
      last_warning = "Could not parse type denotation in " +
        description + ":\n  " + last_warning;
      ok = false;
      return NULL;
    }
  }

  // whether it's known or not, it's a variable!
  PPartTypedTerm ret = new PartTypedTerm;
  ret->name = "variable";
  ret->type = typing;
  if (Gamma.find(varname) != Gamma.end()) ret->idata = Gamma[varname];
  else {
    PVariable v = new Variable(new DataType("tmp"));
    Gamma[varname] = v->query_index();
    ret->idata = v->query_index();
    delete v;
  }

  return ret;
}

PPartTypedTerm TextConverter :: check_abstraction(string description,
        Alphabet &Sigma, Naming &Gamma, Naming &Zeta, Naming &Theta,
        bool &ok, bool metadef) {

  if (description.length() == 0 || description[0] != '/') return NULL;
    // it starts with /, so it has to be an abstraction
    // that means it must have the form: /\<var>:<type>.<term>

  // check the format 
  if (description.length() < 5) {
    last_warning = "Illegal term: " + description + "\n  Starts " +
      "with /, but too short to be an abstraction!";
    ok = false; return NULL;
  }
  if (description[1] != '\\') {
    last_warning = "Illegal term: " + description + "\n  Half of " +
      "the lambda operator /\\.";
    ok = false; return NULL;
  }

  int dot = description.find('.');
  if (dot == string::npos) {
    last_warning = "Illegal term: " + description + "\n  " +
      "Abstraction without separating period.";
    ok = false; return NULL;
  }

  // this might be an abstraction of the form /\x,y.s
  int comma = description.find(',');
  if (comma != string::npos && comma < dot) {
    string sub1 = description.substr(0,comma);
    string sub2 = description.substr(comma+1);
    description = sub1 + "./\\" + sub2;
    dot = comma;
  }

  // this might also have the form /\x:type.s instead of
  // /\x_{type}.s
  int colon = description.find(':');
  if (colon != string::npos && colon < dot) {
    string sub1 = description.substr(0,colon);
    string sub2 = description.substr(colon+1,dot-colon-1);
    string sub3 = description.substr(dot);
    description = sub1 + "_{" + sub2 + "}" + sub3;
    dot = description.find('.');
  }

  string varpart = description.substr(2,dot-2);
  string restpart = description.substr(dot+1);
  normalise(varpart);

  // see whether the varpart satisfies the requirements of a variable
  Naming Gamma2;
  PPartTypedTerm x = check_variable(varpart, Gamma2, Theta, ok);
  if (x == NULL) {
    last_warning = "Could not parse abstraction " + description +
      ": abstraction variable " + varpart + " not a variable." +
      (ok ? "" : "\n  " + last_warning);
    ok = false;
    return NULL;
  }

  // if this variable already occurs in Gamma, we need to back it up
  string varname = Gamma2.begin()->first;
  int backup = -1;
  if (Gamma.find(varname) != Gamma.end()) backup = Gamma[varname];
  Gamma[varname] = Gamma2[varname];

  PPartTypedTerm sub = convert_term(restpart, Sigma, Gamma, Zeta,
                                    Theta, metadef);

  if (backup != -1) Gamma[varname] = backup;
  else Gamma.erase(varname);

  if (sub == NULL) {
    delete x;
    ok = false;
    return NULL;
  }

  PPartTypedTerm ret = new PartTypedTerm;
  ret->name = "abstraction";
  ret->children.push_back(x);
  ret->children.push_back(sub);
  return ret;
}

bool TextConverter :: check_parts(string description,
                                  vector<string> &ret) {
  remove_outer_spaces(description);
  if (description == "") return true;

  // does it have the form (...)SOMETHING ?
  if (description[0] == '(') {
    int k = find_matching_bracket(description, 0);
    if (k < 0) return false;
    ret.push_back(description.substr(1,k-1));
    string rest = description.substr(k+1);
    return check_parts(rest, ret);
  }

  // is it an abstraction? if so, the whole term is a part
  if (description[0] == '/') {
    ret.push_back(description);
    return true;
  }

  // if it doesn't start with something in brackets, it has to start
  // with either a variable or a constant symbol
  if (description[0] != '%' && !generic_character(description[0])) {
    last_warning = "Do not know how to handle " + description;
    return false;
  }

  int i = 1;
  while (i < description.length() &&
         generic_character(description[i])) i++;

  // the whole string is a single symbol
  if (i == description.length()) {
    ret.push_back(description);
    return true;
  }

  // the string has the form xSOMETHING
  if (description[i] == '(' || description[i] == ' ') {
    string sub = description.substr(i);
    ret.push_back(description.substr(0,i));
    return check_parts(description.substr(i), ret);
  }

  // the string has the form x[y]SOMETHING
  if (description[i] == '[') {
    int k = find_matching_bracket(description, i);
    if (k == string::npos) {
      last_warning = "Unterminated meta-variable application in " +
        description;
      return false;
    }
    ret.push_back(description.substr(0,k+1));
    return check_parts(description.substr(k+1), ret);
  }

  // the string has the form x_{y}SOMETHING
  if (description[i] == '_') {
    int k;
    if (i == description.length()-1 ||
        description[i+1] != '{' ||
        (k = description.find('}', i)) == string::npos) {
      last_warning = "Unterminated type denotation in " + description;
      return false;
    }
    ret.push_back(description.substr(0,k+1));
    return check_parts(description.substr(k+1), ret);
  }

  // this has some very strange form...
  last_warning = "Do not know how to handle " + description;
  return false;
}

PPartTypedTerm TextConverter :: check_application(string description,
        Alphabet &Sigma, Naming &Gamma, Naming &Zeta, Naming &Theta,
        bool &ok, bool metadef) {

  normalise(description);
  if (description.length() == 0) return NULL;

  vector<string> parts;
  ok = check_parts(description, parts);

  if (!ok) return NULL;
  if (parts.size() == 1) return NULL;
  
  vector<PPartTypedTerm> tparts;
  for (int i = 0; i < parts.size(); i++) {
    PPartTypedTerm tmp = convert_term(parts[i], Sigma, Gamma, Zeta,
                                      Theta, metadef);
    if (tmp == NULL) {
      for (int j = 0; j < i; j++) delete tparts[j];
      return NULL;
    }
    tparts.push_back(tmp);
  }
  
  // put the parts together!
  PPartTypedTerm ret = tparts[0];
  for (int j = 1; j < tparts.size(); j++) {
    PPartTypedTerm tp = new PartTypedTerm;
    tp->name = "application";
    tp->children.push_back(ret);
    tp->children.push_back(tparts[j]);
    ret = tp;
  }
  return ret;
}

PPartTypedTerm TextConverter :: check_meta(string description,
        Alphabet &Sigma, Naming &Gamma, Naming &Zeta, Naming &Theta,
        bool &ok) {
        
  string metavar, args;
  
  // split in (possible) variable and arguments list
  int k = description.find('[');
  if (k == string::npos) {
    metavar = description;
    args = "";
  }
  else {
    int j = find_matching_bracket(description, k);
    if (j == -1) ok = false;
    if (j != description.length()-1) return NULL;
    metavar = description.substr(0,k);
    args = description.substr(k+1,description.length()-k-2);
  }
  
  // split further in meta-variable and type denotation
  string var, denotation;
  split(metavar, var, denotation, k);
  if (k == -1) {ok = false; return NULL;}
  if (k != metavar.length()) return NULL;
  
  // is the candidate for a variable actually a valid variable?
  if (var == "") return NULL;
  if (Gamma.find(var) != Gamma.end()) return NULL;
  if (var[0] != '%' && !generic_character(var[0])) return NULL;
  for (k = 1; k < var.length(); k++) {
    if (!generic_character(var[k])) return NULL;
  }
  
  // looks like we're good!  Let's see about the type denotation
  PType type = NULL;
  int Nargs = -1;
  if (denotation != "") {
    vector<PType> parts = convert_typedeclaration(denotation, Theta);
    if (parts.size() == 0) { ok = false; return NULL; }
    Nargs = parts.size()-1;
    type = parts[0];
    for (k = parts.size()-1; k > 0; k--)
      type = new ComposedType(parts[k], type);
  }
  
  // and split the arguments list
  vector<string> arglist;
  while ((k = find_substring(args, ",")) != -1) {
    arglist.push_back(args.substr(0,k));
    args = args.substr(k+1);
  }
  if (args != "") arglist.push_back(args);
  
  // do the arity of the meta-variable and the number of arguments
  // correspond?
  if (Nargs != -1 && Nargs != arglist.size()) {
    last_warning = "Arity of meta-variable and number of arguments "
      "do not correspond in " + description;
    delete type;
    ok = false;
    return NULL;
  }
  
  // all right... can we get the subterms?
  vector<PPartTypedTerm> subs;
  for (int i = 0; i < arglist.size(); i++) {
    PPartTypedTerm tmp = convert_term(arglist[i], Sigma, Gamma, Zeta,
                                      Theta, true);
    if (tmp == NULL) {
      for (int j = 0; j < i; j++) delete subs[j];
      ok = false;
      return NULL;
    }
    subs.push_back(tmp);
  }
  
  // make the variable as well
  PPartTypedTerm mvar = new PartTypedTerm;
  mvar->name = "variable";
  mvar->type = type;
  if (Zeta.find(var) != Zeta.end()) mvar->idata = Zeta[var];
  else {
    PVariable v = new Variable(new DataType("tmp"));
    Zeta[var] = v->query_index();
    mvar->idata = v->query_index();
    delete v;
  }
  
  // and make the meta-application!
  PPartTypedTerm ret = new PartTypedTerm;
  ret->name = "meta";
  ret->children.push_back(mvar);
  ret->children.insert(ret->children.end(), subs.begin(), subs.end());
  return ret;
}

PPartTypedTerm TextConverter :: convert_term(string description,
                     Alphabet &Sigma, Naming &Gamma, Naming &Zeta,
                     Naming &Theta, bool metadef) {
  
  normalise(description);
  if (description.length() == 0) {
    last_warning = "Empty term.";
    return NULL;
  }

  bool ok = true;
  PPartTypedTerm attempt;
  
  attempt = check_constant(description, Sigma, Theta, ok);
  if (attempt == NULL && !ok) return NULL;
  if (attempt != NULL) return attempt;

  if (metadef) {
    attempt = check_meta(description, Sigma, Gamma, Zeta, Theta, ok);
    if (attempt == NULL && !ok) return NULL;
    if (attempt != NULL) return attempt;
  }
  
  attempt = check_variable(description, Gamma, Theta, ok);
  if (attempt == NULL && !ok) return NULL;
  if (attempt != NULL) return attempt;

  attempt = check_abstraction(description, Sigma, Gamma, Zeta,
                              Theta, ok, metadef);
  if (attempt == NULL && !ok) return NULL;
  if (attempt != NULL) return attempt;

  attempt = check_application(description, Sigma, Gamma, Zeta,
                              Theta, ok, metadef);
  if (attempt == NULL && !ok) return NULL;
  if (attempt != NULL) return attempt;
  
  last_warning = "Could not determine form of " + description + ".";
  return NULL;
}

PTerm TextConverter :: TERM(string description, Alphabet &Sigma,
                           PType expected_type) {
  last_warning = "";
  if (!check_correct_bracket_matching(description)) return NULL;

  Naming Gamma, Zeta, Theta;

  PPartTypedTerm result = convert_term(description, Sigma, Gamma,
                                       Zeta, Theta, false);

  if (result == NULL) return NULL;
  
  Typer typer;
  PTerm ret = typer.type_term(result, Sigma, expected_type);
  if (ret == NULL) last_warning = typer.query_warning();
  
  delete result;
  return ret;
}

void TextConverter :: RULE(string left, string right,
                           Alphabet &Sigma,
                           PTerm &tleft, PTerm &tright,
                           PType expected_type) {
  
  tleft = NULL;
  tright = NULL;
  
  last_warning = "";
  if (!check_correct_bracket_matching(left) ||
      !check_correct_bracket_matching(right)) return;
  
  Naming Gamma, Zeta, Theta;
  PPartTypedTerm pleft = convert_term(left, Sigma, Gamma,
                                      Zeta, Theta, true);
  if (pleft == NULL) return;
  
  PPartTypedTerm pright = convert_term(right, Sigma, Gamma,
                                       Zeta, Theta, true);
  
  if (pright == NULL) {
    delete pleft;
    return;
  }
  
  // we type both meta-terms at the same time, by considering the term
  // x_{expected_type -> expected_type -> whatever} left right
  PPartTypedTerm head = new PartTypedTerm;
  head->name = "variable";
  PVariable v = new Variable(new DataType("o"));
  head->idata = v->query_index();
  delete v;
  PType alpha;
  if (expected_type == NULL) alpha = new TypeVariable();
  else alpha = expected_type->copy();
  head->type = new ComposedType(alpha,
                 new ComposedType(alpha->copy(),
                                  new DataType("rule")));
  
  PPartTypedTerm ap1 = new PartTypedTerm;
  ap1->name = "application";
  ap1->children.push_back(head);
  ap1->children.push_back(pleft);
  PPartTypedTerm ap2 = new PartTypedTerm;
  ap2->name = "application";
  ap2->children.push_back(ap1);
  ap2->children.push_back(pright);

  Typer typer;
  PTerm ret = typer.type_term(ap2, Sigma);
  if (ret == NULL) {
    last_warning = typer.query_warning();
    return;
  }
  
  tright = ret->replace_subterm(NULL, "2");
  tleft = ret->replace_subterm(NULL, "12");
  delete ap2;
  delete ret;
}

PPartTypedTerm TextConverter :: make_single_term(
                                        vector<PPartTypedTerm> &lhs,
                                        vector<PPartTypedTerm> &rhs,
                                        Alphabet &F) {
  // declare the constant symbols we need to string the system together
  F.add("!RULE", TYPE("$a -> $a -> list"));
  F.add("!PAIR", TYPE("list -> list -> list"));
  F.add("!NIL", TYPE("list"));
  
  PPartTypedTerm system = new PartTypedTerm;
  system->name = "constant"; system->sdata = "!NIL";

  // add all the rules!
  for (int i = lhs.size()-1; i >= 0; i--) {
    // create the constants we'll need
    PPartTypedTerm rule = new PartTypedTerm;
    rule->name = "constant"; rule->sdata = "!RULE";
    PPartTypedTerm pair = new PartTypedTerm;
    pair->name = "constant"; pair->sdata = "!PAIR";

    // make a pptt for the rule
    PPartTypedTerm rl = new PartTypedTerm;
    rl->name = "application";
    rl->children.push_back(new PartTypedTerm);
    rl->children.push_back(rhs[i]);
    rl->children[0]->name = "application";
    rl->children[0]->children.push_back(rule);
    rl->children[0]->children.push_back(lhs[i]);

    // add the rule at the front of the list
    PPartTypedTerm tmp = system;
    system = new PartTypedTerm;
    system->name = "application";
    system->children.push_back(new PartTypedTerm);
    system->children.push_back(tmp);
    system->children[0]->name = "application";
    system->children[0]->children.push_back(pair);
    system->children[0]->children.push_back(rl);
  }

  return system;
}

string TextConverter :: fix_name(string name) {
  for (int i = 0; i < name.length(); i++) {
    char c = name[i];
    if (generic_character(c) && c != '!') continue;
    string rep;
    if (c == '!') rep = "!fac";
    else if (c == '?') rep = "!question";
    else if (c == '.') rep = "!dot";
    else if (c == ':') rep = "!colon";
    else if (c == ';') rep = "!semicolon";
    else if (c == '*') rep = "!times";
    else if (c == '+') rep = "!plus";
    else if (c == '-') rep = "!minus";
    else if (c == '/') rep = "!div";
    else if (c == '$') rep = "!dollar";
    else {
      char ss[20];
      ss[0] = '!';
      int j = 1;
      while (c != 0) {
        ss[j] = (unsigned char)(c) % 9 + '1';
        c = c / 9;
        j++;
      }
      ss[j] = '0';
      ss[j+1] = '\0';
      rep = string(ss);
    }
    name = name.substr(0,i) + rep + name.substr(i+1);
  }
  return name;
}

