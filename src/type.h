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

#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include <map>
#include "varset.h"
using namespace std;

/**
 * These classes implement typing.
 *
 * Under normal usage, Types are only referenced by pointers.  The
 * user (where "user" would be something like a term or the rest of
 * the program) keeps a PType.
 *
 * As PTypes are pointers, it is usually not a good idea to reuse the
 * same type for something else.  Instead, a copy() function is
 * provided which creates a copy of the type.
 *
 * Do not delete a PType when you are done with it, just call
 * free_memory() on it.  Delete will not free up subtypes, using
 * free_memory() will.
 */

class Type;
typedef Type* PType;

class TypeSubstitution;

typedef map<int,string> TypeNaming;

class Type {
  public:
    virtual PType copy();
    virtual string to_string(TypeNaming &naming, bool brackets = false);
    virtual string to_string(bool brackets = false);
      // the first tries to give "small" names to all type variables,
      // the second gives each type variable its formal name

    virtual bool equals(PType other);
      // returns whether this and other are the same type
    virtual bool query_data();
    virtual bool query_composed();
    virtual bool query_typevar();
      // to check what kind of type this is
    virtual PType query_child(int index);
      // used to access subtypes of data type and composed type
      // without having to typecast; returns NULL if there is no
      // child with the given index, otherwise the given child
      // (not a copy!)
    virtual PType collapse();
      // returns this type but with all data types (and other non
      // composed types) replaced by the base type o;
      // this creates a new PType, it doesn't affect the current one
    virtual PType substitute(TypeSubstitution &theta);
      // applies the given substitution on this type
      // this will modify the current type, and returns a pointer to
      // the result; the old pointer to this type should be
      // forgotten, not deletes
    virtual bool instantiate(PType tau, TypeSubstitution &theta);
      // finds a type substitution such that this->substitute(theta)
      // equals tau; if no such substitution exists just returns false
    virtual Varset vars();
      // returns the type variables occurring in this type, in order
      // of occurrence
};

class DataType : public Type {
  private:
    string constructor;
    vector<PType> children;
    
  public:
    DataType(string _name);
    DataType(string _name, vector<PType> _children);
    ~DataType();
    
    // functions inherited from Type
    PType copy();
    string to_string(TypeNaming &naming, bool brackets = false);
    string to_string(bool brackets = false);
    bool equals(PType other);
    bool query_data();
    PType query_child(int index);
    PType collapse();
    PType substitute(TypeSubstitution &theta);
    bool instantiate(PType tau, TypeSubstitution &theta);
    Varset vars();
    
    // my own functions
    string query_constructor();
};


class ComposedType : public Type {
  private:
    PType left, right;

  public:
    ComposedType(PType _left, PType _right);
    ~ComposedType();

    PType copy();
    string to_string(TypeNaming &naming, bool brackets = false);
    string to_string(bool brackets = false);
    bool equals(PType other);
    bool query_composed();
    PType query_child(int index);
    PType collapse();
    PType substitute(TypeSubstitution &theta);
    bool instantiate(PType tau, TypeSubstitution &theta);
    Varset vars();
};

const int FRESHTYPEVAR = -1;

class TypeVariable : public Type {
  private:
    int index;
    string pretty_name(int k);
  
  public:
    TypeVariable(int index = FRESHTYPEVAR);
    TypeVariable(TypeVariable *alpha);
    
    PType copy();
    string to_string(TypeNaming &naming, bool brackets = false);
    string to_string(bool brackets = false);
    bool equals(PType other);
    bool query_typevar();
    PType substitute(TypeSubstitution &theta);
    bool instantiate(PType tau, TypeSubstitution &theta);
    Varset vars();
    
    int query_index();
};

#endif

