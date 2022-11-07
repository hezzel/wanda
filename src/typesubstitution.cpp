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

#include "typesubstitution.h"

TypeSubstitution :: TypeSubstitution() {}
TypeSubstitution :: ~TypeSubstitution() {clear();}

PType &TypeSubstitution :: operator[](const int index) {
  map<int,PType>::iterator it = data.find(index);
  if (it == data.end()) {
    data[index] = NULL;
    return data[index];
  } 
  else return it->second;
}

PType &TypeSubstitution :: operator[](TypeVariable *v) {
  return operator[](v->query_index());
}

void TypeSubstitution :: remove(const int index) {
  delete data[index];
  data.erase(index);
}

void TypeSubstitution :: remove(TypeVariable *v) {
  remove(v->query_index());
}

void TypeSubstitution :: clear() {
  map<int,PType>::iterator it;
  for (it = data.begin(); it != data.end(); it++)
    delete it->second;
  data.clear();
}

string TypeSubstitution :: to_string() {
  map<int,PType>::iterator it;
  string ret = "";

  ret += "{";
  for (it = data.begin(); it != data.end(); it++) {
    TypeVariable *v = new TypeVariable(it->first);
    string name = dynamic_cast<PType>(v)->to_string();
    delete v;
    ret += "  " + name + " --> " + (it->second == NULL ?
      "NULL" : it->second->to_string()) + "\n";
  }
  ret += "}\n";
  return ret;
}

void TypeSubstitution :: compose(TypeSubstitution &other) {
  map<int,PType>::iterator it;

  for (it = data.begin(); it != data.end(); it++) {
    it->second = it->second->substitute(other);
  }
}

