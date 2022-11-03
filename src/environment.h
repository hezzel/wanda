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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <map>
#include "type.h"
#include "varset.h"

/**
 * This class implements an environment of variables.
 *
 * An Environment keeps track of the free variables present in a
 * term, along with their type.  For practical (printing) purposes it
 * also remembers a name for each variable, although names need not
 * be unique (and can be set to "").
 *
 * As in the theory behind it, environments change depending on their
 * place in a term.  Roughly, if gamma |- (/\ x:o.C[x]) : o->a, then
 * gamma + {x:o} |- C[x]:a.  As such, environments can have elements
 * added and removed.
 */

class Variable;
typedef Variable* PVariable;

class Environment {
  private:
    map<int,string> names;
    map<int,PType> types;
  
  public:
    ~Environment();

    void add(int index, PType type, string name = "");
    void add(PVariable var, string name = "");
    void add(int index, string name);
    void rename(int index, string name);
    void rename(PVariable var, string name);
    void remove(int index);
    void remove(PVariable var);
    bool contains(int index);
    bool contains(PVariable var);
    Varset get_variables();
    void merge(Environment &gamma);
    
    int lookup(string name);
      // returns -1 if not found, otherwise the first match
    string get_name(int index);
    string get_name(PVariable var);
      // these might return an empty string when no name is set
    PType get_type(int index);
      // returns the type in the environment, callers need to make a
      // copy if they want to use it!
};

#endif

