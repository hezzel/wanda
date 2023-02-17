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

#include "alphabet.h"

Alphabet :: ~Alphabet() {
  for (map<string,PType>::iterator it = data.begin();
       it != data.end(); it++) delete it->second;
}

bool Alphabet :: contains(string name) {
  return data.find(name) != data.end();
}

void Alphabet :: add(string name, PType type) {
  if (contains(name)) delete type;
  else data[name] = type;
}

PConstant Alphabet :: get(string name) {
  map<string,PType>::iterator it = data.find(name);
  if (it == data.end()) return NULL;
  return new Constant(it->first, it->second->copy());
}

PType Alphabet :: query_type(string name) {
  map<string,PType>::iterator it = data.find(name);
  if (it == data.end()) return NULL;
  else return it->second;
}

vector<string> Alphabet :: get_all() {
  vector<string> ret;
  for (map<string,PType>::iterator it = data.begin();
       it != data.end(); it++) ret.push_back(it->first);
  return ret;
}

void Alphabet :: copy(Alphabet &target) {
  for (map<string,PType>::iterator it = data.begin();
       it != data.end(); it++) {
    target.add(it->first, it->second->copy());
  }
}

string Alphabet :: to_string() {
  string ret = "[ \n";
  for (map<string,PType>::iterator it = data.begin();
       it != data.end(); it++) {
    TypeNaming naming;
    ret += "  " + it->first + " : " + it->second->to_string(naming) + " ;\n";
  }
  return ret.substr(0,ret.length()-3) + "\n]\n";
}

void Alphabet :: remove(string elem) {
  if (data.find(elem) != data.end()) delete data[elem];
  data.erase(elem);
}

void Alphabet :: clear() {
  for (map<string,PType>::iterator it = data.begin();
      it != data.end(); it++) {
    delete it->second;
  }
  data.clear();
}

