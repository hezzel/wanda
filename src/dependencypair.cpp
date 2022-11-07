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

#include "dependencypair.h"
#include "environment.h"

DependencyPair :: DependencyPair(PTerm _left, PTerm _right, int _style)
    :left(_left), right(_right), style(_style) {
}

DependencyPair :: ~DependencyPair() {
  delete left;
  delete right;
}

DependencyPair *DependencyPair :: copy() {
  return new DependencyPair(left->copy(), right->copy(), style);
}

PTerm DependencyPair :: query_left() {
  return left;
}

PTerm DependencyPair :: query_right() {
  return right;
}

string DependencyPair :: to_string(bool showtypes) {
  Environment env;
  string ret;
  if (showtypes) {
    TypeNaming tenv;
    ret = left->to_string(env, tenv);
    ret += " ~~> " + right->to_string(env, tenv);
  }
  else {
    ret = left->to_string(env);
    ret += " ~~> " + right->to_string(env);
  }
  if (style == 1) ret += " (left-most)";
  return ret;
}

void DependencyPair :: set_noneating(int Z, int pos) {
  if (pos >= 31) return;
  noneating[Z] |= (1 << pos);
}

bool DependencyPair :: query_noneating(int Z, int pos) {
  if (pos >= 31) return false;
  if (noneating.find(Z) == noneating.end()) return false;
  return noneating[Z] & (1 << pos) != 0;
}

map<int,int> DependencyPair :: query_noneating_mapping() {
  return noneating;
}

void DependencyPair :: set_headmost(bool value) {
  if (value) style = 1;
  else style = 0;
}

bool DependencyPair :: query_headmost() {
  return style == 1;
}

