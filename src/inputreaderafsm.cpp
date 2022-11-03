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

#include "inputreaderafsm.h"
#include "matchrule.h"
#include "outputmodule.h"
#include <fstream>
#include <iostream>

PConstant InputReaderAFSM :: read_constant(string description) {
  // start: do we have a colon, to separate name and type?
  int colon = description.find(':');
  if (colon == string::npos) {
    last_warning = "missing colon.";
    return NULL;
  }

  // is the thing before the colon a single word and legal name?
  string name = description.substr(0,colon);
  while (name.length() > 0 && name[0] == ' ') name = name.substr(1);
  while (name.length() > 0 && name[name.length()-1] == ' ')
    name = name.substr(0,name.length()-1);
  if (name.length() == 0) {
    last_warning= "missing constant name.";
    return NULL;
  }
  for (int i = 0; i < name.length(); i++) {
    if (!generic_character(name[i])) {
      last_warning= "illegal characters in " + name + ".";
      return NULL;
    }
  }

  // is the thing after it a legal type?
  string typetxt = description.substr(colon+1);
  PType type = TYPE(typetxt);
  if (type == NULL) {
    last_warning = "could not read type: " + last_warning;
    return NULL;
  }

  return new Constant(name, type);
}

MatchRule *InputReaderAFSM :: read_rule(string description,
                                         Alphabet &Sigma) {
  // find separating arrow
  int arrow = description.find("=>");
  if (arrow == string::npos) {
    last_warning = "missing =>.";
    return NULL;
  }

  // can we make a rule out of this?
  string left = description.substr(0, arrow);
  string right = description.substr(arrow+2);
  PTerm l, r;
  RULE(left, right, Sigma, l, r);
  if (l == NULL || r == NULL) return NULL;

  // is it a valid rule?
  MatchRule *rule = new MatchRule(l,r);
  if (!rule->query_valid()) {
    last_warning = "invalid rule: " + rule->query_invalid_reason() + ".";
    delete rule;
    return NULL;
  }

  return rule;
}

bool InputReaderAFSM :: read_manually(Alphabet &Sigma,
                                       vector<MatchRule*> &rules) {
  cout << "Please enter each symbol in the alphabet, together with "
       << "its type, in the format f : (o -> o) -> o.  Finish with "
       << "an empty line.  Type ABORT to abort." << endl;

  while (true) {
    string input;
    getline(cin, input);
    if (input == "") break;
    if (input == "ABORT") {return false;}
    PConstant f = read_constant(input);
    if (f == NULL) {
      cout << "Could not parse function symbol [" + input + "]: "
           << last_warning << endl;
      last_warning = "";
    }
    else {
      Sigma.add(f->to_string(), f->query_type()->copy());
      delete f;
    }
  }

  cout << "Please enter all rules in the system, in the form "
       << "l => r.  Finish with an empty line.  Type ABORT to "
       << "abort." << endl;

  while (true) {
    string input;
    getline(cin, input);
    if (input == "") break;
    if (input == "ABORT") {return false;}
    MatchRule *rule = read_rule(input, Sigma);
    if (rule == NULL) {
      cout << "Could not parse rule [" << input << "]: "
           << last_warning << endl;
      last_warning = "";
    }
    else rules.push_back(rule);
  }

  return true;
}

bool InputReaderAFSM :: read_file(string fname, Alphabet &Sigma,
                                   vector<MatchRule*> &rules) {
  ifstream fin(fname.c_str());

  while (true) {
    string input;
    getline(fin, input);
    if (input == "") break;
    PConstant f = read_constant(input);
    if (f == NULL) {
      last_warning = "Could not parse function symbol [" + input +
                     "]:\n"+ last_warning;
      return false;
    }
    else {
      Sigma.add(f->to_string(), f->query_type()->copy());
      delete f;
    }
  }

  while (true) {
    string input;
    getline(fin, input);
    if (input == "") break;
    MatchRule *rule = read_rule(input, Sigma);
    if (rule == NULL) {
      last_warning = "Could not parse rule [" + input + "]:\n" +
                     last_warning;
      return false;
    }
    else rules.push_back(rule);
  }

  wout.print_system(Sigma, rules);

  return true;
}

