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

#ifndef DEPENDENCYPAIR_H
#define DEPENDENCYPAIR_H

#include "term.h"
#include <map>

class DependencyPair {
  private:
    PTerm left;
    PTerm right;
    int style;
      // style: 0 for a normal dependency pair,
      //        1 for a headmost dependency pair
    
    map<int,int> noneating;
      // restrictions: Z is non-eating at position k if bit k is set
      // in noneating[Z]

  public:
    DependencyPair(PTerm _left, PTerm _right, int _style = 0);
    ~DependencyPair();

    DependencyPair *copy();
    PTerm query_left();
    PTerm query_right();
    string to_string(bool showtypes = false);
    void set_noneating(int Z, int pos);
    bool query_noneating(int Z, int pos);
    map<int,int> query_noneating_mapping();
    void set_headmost(bool value);
    bool query_headmost();
};

typedef vector<DependencyPair*> DPSet;

#endif

