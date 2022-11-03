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

#include "substitution.h"
#include "environment.h"

Substitution :: Substitution() {
  lastcheck = -1;
}

Substitution :: ~Substitution() {
  map<int,PTerm>::iterator it;
  for (it = data.begin(); it != data.end(); it++) {
    delete it->second;
  }
  lastcheck = -1;
  data.clear();
}

bool Substitution :: contains(const int index) {
  if (lastcheck == index) return lastlookup->second != NULL;
  lastlookup = data.find(index);
  if (lastlookup == data.end()) {
    lastcheck = -1;
    return false;
  }
  lastcheck = index;
  return true;
}

bool Substitution :: contains(const PVariable v) {
  return contains(v->query_index());
}

PTerm &Substitution :: operator[](const int index) {
  if (lastcheck == index) return lastlookup->second;
  lastcheck = index;
  lastlookup = data.find(index);
  if (lastlookup == data.end()) {
    data[index] = NULL;
    lastcheck = -1;
    return data[index];
  }
  else return lastlookup->second;
}

PTerm &Substitution :: operator[](const PVariable v) {
  return operator[](v->query_index());
}

void Substitution :: remove(const int index) {
  delete data[index];
  data.erase(index);
}

void Substitution :: remove(const PVariable var) {
  remove(var->query_index());
}

string Substitution :: to_string(Environment env) {
  map<int,PTerm>::iterator it;
  string ret = "";

  ret += "{";
  for (it = data.begin(); it != data.end(); it++) {
    ret += "  " + env.get_name(it->first) + " --> " +
           it->second->to_string(env) + "\n";
  }
  ret += "}\n";
  return ret;
}

