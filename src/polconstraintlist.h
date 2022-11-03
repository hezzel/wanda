/**************************************************************************
   Copyright 2013 Cynthia Kop

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

#ifndef POLCONSTRAINTS_H
#define POLCONSTRAINTS_H

#include "formula.h"
#include "polynomial.h"

class PolyModule;

struct PolyConstraint {
  PPol left;
  PPol right;
  int variable_index;
  bool active;

  PolyConstraint(PPol l, PPol r, int id)
    :left(l), right(r), variable_index(id), active(true) {}
  ~PolyConstraint() { delete left; delete right; }
};

class PolConstraintList {
  private:
    vector<PFormula> formulas;
    vector<PolyConstraint*> constraints;

    PolyModule *master;

    map<string,int> constraint_lookup;

    string print_poly_constraint(PPol left, PPol right);
    string print_poly_constraint(int index);
    int index_of_constraint(PPol left, PPol right);
      // looks up the index of the given constraint, possibly adding
      // it if it doesn't exist yet; this is the index in the constraint
      // list
      // left and right are either deleted or used in a new constraint!
    int var_for_constraint(PPol left, PPol right);
      // like index_of_constraint, but returns variable_index
    bool trivial_checks(PPol left, PPol right, int vid);
      // tries to mark constraint vid (left >= right) as true or false
      // by some basic checks
    bool remove_duplicates(PPol left, PPol right, int vid);
      // takes occurrences of the same term to the same side (eg 3a >=
      // a + b becomes 2a >= b)
    bool split_max(PPol left, PPol right, int vid);
    PPol split_max(PPol pol, PPol &greater, vector<PPol> &smaller);
      // if pol = C[max(a,b1,...,bn)], turns pol into
      // C[max(b1,...,bn)] and returns C[a] (a copy);
      // greater is set to a, and smaller to [b1,...,bn]
    bool contains_max(PPol pol);
      // returns whether a Max occurs anywhere in pol
    void account_for_greater(PPol &greater, vector<PPol> &smaller,
                             PPol &pol);
      // given that greater >= all the polynomials in smaller,
      // simplify Max occurrences in pol
      // it is assumed that [greater,smaller1,...,smallern] is ordered
      // by the compare function
    void account_for_smaller(PPol &greater, vector<PPol> &smaller,
                             PPol &pol);
      // given that greater < some polynomial in smaller
      // simplify Max occurrences in pol
      // it is assumed that [greater,smaller1,...,smallern] is ordered
      // by the compare function
    bool split_parts(PPol left, PPol right, int vid);
      // deals with absolute positiveness
    vector<int> find_combination(PPol pol);
      // assuming pol has the form a1 + ... + an (possibly n = 1) and
      // is simplified, and some ai has the form b*x or b*F(x), gets
      // the indexes of all variables and functionals in this product
      // and returns those (in the order of occurrence); if no such
      // product occurs, returns an empty list
      // this assumes there is no max anywhere in pol
    void set_coefficient(PPol &pol, int number, int index = -1);
      // if this is a term i*a, sets i to number instead;
      // if this is a term a1 + ... + an, calls set_coefficiient
      // on a_index
    PPol remove_parts(PPol &pol, vector<int> &combination);
      // removes those monomials from pol for which find_combination
      // returns the given combination; the sum of removed monomials
      // is returned
    void remove_variables(PPol left, PPol right, int vid);
      // assuming that left and right are sums of products, where all
      // products contain the same set of variables and functionals,
      // makes requirements implying: vid => left >= right

  public:
    PolConstraintList(PolyModule *_master);
    ~PolConstraintList();

    void reset();
      // clears the lists
    void add_formula(PFormula formula);
      // adds a formula to the list of formulas
    int add_geq(PPol left, PPol right);
      // adds a constraint of the form l >= r, and returns its formula index
    void simplify();
      // goes through all constraints, and marks them as inactive after
      // adding suitable clauses to the list of formulas; the formula may also
      // be left as active, if it contains no variables
    And *generate_complete_formula();
      // creates the conjunction of the added formulas; this is a
      // copy, so may be manipulated as the caller sees fit
      // this also includes any remaining active constraints
      // WARNING: to preserve correctness, do not call this before calling
      // simplify!
    string print();
      // returns debugging information
    void debug_print();
      // prints debugging information using OutputModule
};

class IntegerArithmeticConstraint : public Formula {
  private:
    PPol left;
    PPol right;

  protected:
    PFormula simplify_main();

  public:
    IntegerArithmeticConstraint(PPol l, PPol r);
    ~IntegerArithmeticConstraint();

    PFormula copy();
    string to_string(bool brackets = false);
    bool query_special(string description);

    PPol query_left();
    PPol query_right();
    PPol replace_left(PPol l);
    PPol replace_right(PPol r);
};

#endif

