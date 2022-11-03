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

#include "environment.h"
#include "term.h"

void Environment :: merge(Environment &gamma) {
  map<int,PType>::iterator it;
  for (it = gamma.types.begin(); it != gamma.types.end(); it++) {
    if (it->second != NULL) types[it->first] = it->second->copy();
  }

  map<int,string>::iterator ti;
  for (ti = gamma.names.begin(); ti != gamma.names.end(); ti++) {
    if (ti->second != "") names[ti->first] = ti->second;
  }
}

Environment :: ~Environment() {
  map<int,PType>::iterator it;
  for (it = types.begin(); it != types.end(); it++) {
    if (it->second != NULL) delete it->second;
  }
  types.clear();
  names.clear();
}

void Environment :: add(int index, PType type, string name) {
  types[index] = type;
  names[index] = name;
}

void Environment :: add(PVariable var, string name) {
  add(var->query_index(), var->query_type()->copy(), name);
}

void Environment :: add(int index, string name) {
  names[index] = name;
  if (types.find(index) == types.end()) types[index] = NULL;
}

void Environment :: rename(int index, string name) {
  names[index] = name;
}

void Environment :: rename(PVariable var, string name) {
  rename(var->query_index(), name);
}

void Environment :: remove(int index) {
  map<int,PType>::iterator it = types.find(index);
  if (it == types.end()) return;
  else {
    delete it->second;
    types.erase(it);
    names.erase(index);
  }
}

void Environment :: remove(PVariable var) {
  remove(var->query_index());
}

bool Environment :: contains(int index) {
  return names.find(index) != names.end();
}

bool Environment :: contains(PVariable var) {
  return contains(var->query_index());
}

Varset Environment :: get_variables() {
  Varset ret;
  map<int,PType>::iterator it;
  for (it = types.begin(); it != types.end(); it++) {
    ret.add(it->first);
  }
  return ret;
}

int Environment :: lookup(string name) {
  map<int,string>::iterator it;
  for (it = names.begin(); it != names.end(); it++) {
    if (it->second == name) return it->first;
  }
  return -1;
}

string Environment :: get_name(int index) {
  map<int,string>::iterator it = names.find(index);
  if (it == names.end()) return "";
  else return it->second;
}

string Environment :: get_name(PVariable var) {
  return get_name(var->query_index());
}

PType Environment :: get_type(int index) {
  map<int,PType>::iterator it = types.find(index);
  if (it == types.end()) return NULL;
  else return it->second;
}

