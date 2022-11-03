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

#ifndef ALPHABET_H
#define ALPHABET_H
/**
 * This class implements an Alphabet: a simple class to keep track of
 * the function symbols which may occur in terms, together with their
 * (polymorphic) types.
 */

#include <map>
#include <vector>
#include "type.h"
#include "term.h"

class Alphabet {
  private:
    map<string,PType> data;

  public:
    ~Alphabet();

    bool contains(string name);
    void add(string name, PType type);
      // type becomes property of the alphabet after this call
    PConstant get(string name);
      // returns NULL if the given name is not known
    PType query_type(string name);
      // only returns the type, which remains property of Alphabet
    vector<string> get_all();
    string to_string();
    void remove(string name); 
      // removes an element
    void clear();
    void copy(Alphabet &target);
      // copies all elements of the current alphabet into target
};

#endif

