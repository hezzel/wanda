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

#ifndef TYPESUBSTITUTION_H
#define TYPESUBSTITUTION_H

#include <map>
#include "type.h"

/**
 * This class encodes a type substitution; it is simply a container
 * class mapping typevariable indices to types.
 *
 * It is the responsibility of the class using TypeSubstitution
 * (mostly children of Type) to handle a substitution.
 * Applying a substitution may not duplicate types in the
 * Substitution!  They must always be copied, as otherwise more than
 * one pointer to the same type might exist, which causes trouble the
 * moment you change one.
 */
class TypeSubstitution  {
  private:
    map<int,PType> data;

    void limit_help(int i);
    
  public:
    TypeSubstitution();
    ~TypeSubstitution();

    PType &operator[](const int index);
    PType &operator[](TypeVariable *var);
    void remove(const int index);
    void remove(TypeVariable *var);
    void clear();
    string to_string();
    void compose(TypeSubstitution &other);
      // assigns to each type variable x in the domain the
      // value other(data[x])
};

#endif

