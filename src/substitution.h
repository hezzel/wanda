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

#ifndef SUBSTITUTION_H
#define SUBSTITUTION_H

#include <map>
#include "term.h"

class Environment;

/**
 * This class encodes a substitution; it is simply a container class
 * mapping variable indices to terms.
 *
 * It is the responsibility of the class using Substitution (mostly
 * children of Term) to handle a substitution.
 * A substitution may not duplicate terms in the Substitution!  They
 * must always be copied, as otherwise more than one pointer to the
 * same term might exist, which breaks the moment you change one.
 */
class Substitution  {
  private:
    map<int,PTerm> data;
    
    // for speed:
    int lastcheck;
    map<int,PTerm>::iterator lastlookup;
  
  public:
    Substitution();
    ~Substitution();
    bool contains(const int index);
    bool contains(const PVariable var);
    PTerm &operator[](const int index);
    PTerm &operator[](const PVariable var);
    void remove(const int index);
    void remove(const PVariable var);
    string to_string(Environment environment);
};

#endif

