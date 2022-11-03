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

#ifndef VARSET_H
#define VARSET_H

#include <set>
using namespace std;

class Variable;
typedef Variable* PVariable;
class TypeVariable;
typedef TypeVariable* PTypeVariable;

class Varset;

/**
 * The Varset is just a Set of variable or type variable indexes.
 * Since we use it so often, there are standard functions for
 * common behaviour.
 */

class Varset : public set<int> {
  public:
    Varset();
    Varset(int index);
    Varset(PVariable var);
    Varset(PTypeVariable var);
    
    void add(int index);
    void add(PVariable var);
    void add(PTypeVariable var);
    void add(Varset &other);
    bool contains(int index);
    bool contains(PVariable x);
    bool contains(PTypeVariable alpha);
    bool contains(Varset &other);
    void remove(int index);
    void remove(PVariable var);
    void remove(PTypeVariable var);
    void remove(Varset &other);
};

#endif

