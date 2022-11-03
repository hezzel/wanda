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

#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include "type.h"
#include <set>

/**
 * Polynomials, as used for the weakly monotonic algebras method
 * defined by Jaco van de Pol.
 */

class Polynomial;
typedef Polynomial *PPol;
class PolynomialFunction;
class Polvar;

typedef map<int,PolynomialFunction*> PolynomialSubstitution;

class Polynomial {
  protected:
    int cmp(int a, int b);
      // helping function for compare, returns -1 if a < b, 1 if
      // a > b and 0 if they are equal

    void sort(vector<PPol> &vec);
      // sorts the given vector, using bubblesort and the compare
      // function

  public: // but for internal use only!

    virtual int query_type();
      // returns 0 for an integer, 2 for an unknown, 4 for a
      // variable, 6 for a functional, 8 for a max, 10 for a
      // sum; given a product, returns 1 plus the highest of
      // the types of its children

    virtual int compare(PPol other);
      // imposes an ordering on all polynomials; returns -1 if this
      // polynomial is smaller than other 1 if it is larger, or 0 if
      // they are equal

  public:
    virtual string to_string(bool brackets = false);
    virtual string to_string(map<int,int> &freerename,
                             map<int,int> &boundrename,
                             bool brackets = false);
    virtual PPol copy();
    virtual PPol simplify();
    virtual PPol replace_unknowns(map<int,PPol> &substitution);
    virtual PPol apply_substitution(PolynomialSubstitution &subst);
    bool equals(PPol other);
      // uses compare
    virtual bool query_similar(PPol other, int &a, int &b);
      // assuming both this and other are simplified, returns true
      // if they have the forms i*x and j*x with x a variable
      // respectively; a and b are set to i and j
      // the implementation in Polynomial passes everything on to
      // product, if one of them's a product (if not, the two must
      // be equal)

    virtual bool query_integer();
    virtual bool query_unknown();
    virtual bool query_variable();
    virtual bool query_functional();
    virtual bool query_max();
    virtual bool query_sum();
    virtual bool query_product();

    virtual int number_children();
    virtual PPol get_child(int index);
    virtual PPol replace_child(int index, PPol replacement);

    virtual void vars(vector<Polvar*> initial);

    PType get_function_type();
};

class Integer : public Polynomial {
  private:
    int value;

  public:
    Integer(int val);

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    PPol copy();

    bool query_integer();
    int query_value();
    void set_value(int val);
};

class Unknown : public Polynomial {
  private:
    int index;

  public:
    Unknown(int index);

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    PPol copy();
    PPol replace_unknowns(map<int,PPol> &substitution);

    bool query_unknown();
    int query_index();
};

class Polvar : public Polynomial {
  private:
    int index;

  public:
    Polvar(int index);

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename,
                     bool brackets = false);
    PPol copy();
    PPol apply_substitution(PolynomialSubstitution &subst);

    bool query_variable();
    int query_index();

    void vars(vector<Polvar*> initial);
};

class Functional : public Polynomial {
  private:
    int funindex;
    vector<PolynomialFunction *> args;

  public:
    Functional(int index);
    Functional(int index, PPol arg);
    Functional(int index, vector<PPol> args);
    Functional(int index, vector<PolynomialFunction*> args);
    ~Functional();

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename,
                     bool brackets = false);
    PPol copy();
    PPol simplify();
    PPol replace_unknowns(map<int,PPol> &substitution);
    PPol apply_substitution(PolynomialSubstitution &subst);

    bool query_functional();
    int function_index();
    int number_children();
    PPol get_child(int index);
      // gets the polynomial from the child at the given index
    PolynomialFunction *get_function_child(int index);
      // actually gets the child at the given index
    void add_argument(PolynomialFunction *arg);
    PPol replace_child(int index, PPol replacement);
      // replaces the polynomial from the child at the given index

    void weak_delete();
      // deletes the functional, but not its children

    void vars(vector<Polvar*> initial);
};

class Max : public Polynomial {
  private:
    vector<PPol> parts;

  public:
    Max();
    Max(PPol a, PPol b);
    Max(vector<PPol> parts);
    ~Max();

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename,
                     bool brackets = false);
    PPol copy();
    PPol simplify();
    PPol replace_unknowns(map<int,PPol> &substitution);
    PPol apply_substitution(PolynomialSubstitution &subst);

    bool query_max();
    int number_children();
    PPol get_child(int index);
    void add_child(PPol arg);
    PPol replace_child(int index, PPol replacement);
    void flat_free();

    void vars(vector<Polvar*> initial);
};

class Sum : public Polynomial {
  private:
    vector<PPol> parts;

  public:
    Sum();
    Sum(PPol part);
    Sum(PPol left, PPol right);
    Sum(vector<PPol> parts);
    ~Sum();

    int query_type();
    int compare(PPol other);

    string to_string(bool brackets = false);
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename,
                     bool brackets = false);
    PPol copy();
    PPol simplify();
    PPol replace_unknowns(map<int,PPol> &substitution);
    PPol apply_substitution(PolynomialSubstitution &subst);

    bool query_sum();
    int number_children();
    PPol get_child(int index);
    void add_child(PPol arg);
    PPol replace_child(int index, PPol replacement);
    void flat_free();

    void vars(vector<Polvar*> initial);
};

class Product : public Polynomial {
  private:
    vector<PPol> parts;

    void flatten_nested();
    PPol sum_children();
      // helping functions for simplify

  public:
    Product();
    Product(PPol left, PPol right);
    Product(vector<PPol> parts);
    ~Product();

    int query_type();
    int compare(PPol other);
    bool query_similar(PPol other, int &a, int &b);

    string to_string(bool brackets = false);
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename,
                     bool brackets = false);
    PPol copy();
    PPol simplify();
    PPol replace_unknowns(map<int,PPol> &substitution);
    PPol apply_substitution(PolynomialSubstitution &subst);

    bool query_product();
    int number_children();
    PPol get_child(int index);
    void add_child(PPol arg);
    PPol replace_child(int index, PPol replacement);
    void flat_free();

    void vars(vector<Polvar*> initial);
};

class PolynomialFunction {
  private:
    vector<int> variables;
    vector<PType> variable_types;
    PPol p;
    PType mytype;

    void calculate_type();

  public:
    // after calling this, the types become the property of the
    // PolynomialFunction class, as does "main"
    PolynomialFunction(vector<int> vars,
                       vector<PType> types,
                       PPol main);
    PolynomialFunction(vector<int> vars, PPol main);
    ~PolynomialFunction();

    PolynomialFunction *copy();

    string to_string();
    string to_string(map<int,int> &freerename,
                     map<int,int> &boundrename);
    PType get_function_type();
    PPol apply(vector<PolynomialFunction*> args);
      // calling this does not affect the current PolynomialFunction,
      // nor the arguments
    PPol get_polynomial();
      // returns the underlying polynomial (not a copy!)
    void replace_unknowns(map<int,PPol> &substitution);
      // replaces unkonwns in p, and immediately simplifies
    void replace_polynomial(PPol new_p);
      // replaces p by new_p, without touching (or deleting) the old p
    int num_variables();
    int variable_index(int num);
    PType variable_type(int num);

    void vars(vector<Polvar*> initial);
};

int unused_polvar_index();

#endif

