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

#include "varset.h"
#include "term.h"
#include "type.h"

Varset :: Varset() {}

Varset :: Varset(int index) {
  insert(index);
}

Varset :: Varset(PVariable var) {
  insert(var->query_index());
}

Varset :: Varset(PTypeVariable var) {
  insert(var->query_index());
}

void Varset :: add(int index) {
  insert(index);
}

void Varset :: add(PVariable var) {
  add(var->query_index());
}

void Varset :: add(PTypeVariable var) {
  add(var->query_index());
}

void Varset :: add(Varset &other) {
  insert(other.begin(), other.end());
}

bool Varset :: contains(int index) {
  return find(index) != end();
}

bool Varset :: contains(PVariable var) {
  return contains(var->query_index());
}

bool Varset :: contains(PTypeVariable var) {
  return contains(var->query_index());
}

bool Varset :: contains(Varset &other) {
  for (Varset::iterator it = other.begin(); it != other.end(); it++)
    if (!contains(*it)) return false;
  
  return true;
}

void Varset :: remove(int index) {
  erase(index);
}

void Varset :: remove(PVariable var) {
  erase(var->query_index());
}

void Varset :: remove(PTypeVariable var) {
  erase(var->query_index());
}

void Varset :: remove(Varset &other) {
  for (Varset::iterator it = other.begin(); it != other.end(); it++)
    remove(*it);
}

