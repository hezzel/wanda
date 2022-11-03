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

#ifndef SMTSOLVER_H
#define SMTSOLVER_H

/**
 * This class does limited SMT-solving: it attempts to solve problems
 * over the natural numbers.  Guessed numbers are low (typically 0-3).
 */

#include "formula.h"
#include "polconstraintlist.h"
#include "polynomial.h"
#include <utility>

typedef pair<int,int> IntPair;
typedef map<int,int> IntMap;

class Smt {
  private:
    vector<int> minima;
    vector<int> maxima;
    map<int,int> squares;
    map<int,IntPair> known_products;
    map<int,IntPair> unknown_products;
    map<int,IntPair> known_sums;
    map<int,IntPair> unknown_sums;

    bool unit_propagate(And *formula);
      // if formula has an immediate child X or -X (with X a variable),
      // forces this to be true; other children are simplified
      // (returns true if anything was done)
    bool obvious_propagate(And *formula);
      // if formula has an immediate child n >= A or A >= n where n
      // is an integer and A an unknown, then this function alters
      // minimum and maximum values to make the child true, and
      // possibly forces a value using a substitution
    bool check_single_sides(PFormula formula);
      // if certain parameters occur only on one side of the
      // inequalities, we can force them to their minimum or
      // maximum
    void global_replace(PFormula f, map<int,PPol> &substitution);
      // does the given "unknown substitution" everywhere in the
      // formula
    
    void check_minmax(PFormula formula);
      // if there are unknowns ai such that minima[i] = maxima[i],
      // then turn replace ai by minima[i] everywhere
    PFormula simplify_formula(PFormula formula);
      // simplifies integer arithmetic constraints in the given
      // formula (e.g. taking out constraints l >= 0) and splittting
      // 1 >= a*b up into 1 >= a /\ 1 >= b
    void remove_duplicates(IntegerArithmeticConstraint *p);
      // removes duplicates in left- and right-hand side of a
      // constraint, for instance turning a + b >= a + c into b >= c
    PFormula simplify_arithmetic(IntegerArithmeticConstraint *p);
      // simplifies the given integer arithmetic constraint (e.g.
      // taking out constraints l >= 0, and splitting 0 >= a + b up
      // into 0 >= a /\ 0 >= b

    void get_arithmetic(PFormula formula,
                        vector<IntegerArithmeticConstraint*> &v);
      // returns all integers arithmetic constraints in formula
    void save_squares(PFormula formula);
      // alters the given formula, replacing pairs a*a of unknowns by
      // a new unknown which represents the square of a, and is saved
      // in the squares mapping
    PPol handle_squares(PPol p, map<int,int> &used_squares);
      // helping function for save_squares which actually replaces
      // all squares in p
    void save_unknown_products(PFormula formula);
      // alters the given formula, replacing pairs a*b of unknowns by
      // a new unknown which represents the multiplication, and which
      // is saved in the unknown_products mapping
    PPol handle_unknown_products(PPol p, map<int,IntMap> &used);
      // alters the given formula, replacing all products of two
      // unknowns by a single unknown
    void save_known_products(PFormula formula);
      // alters the given formula, replacing pairs i*b with i an
      // integer and b an unknown by a new unknown which represents
      // the product, and which is saved in the known_products mapping
    PPol handle_known_products(PPol p, map<int,IntMap> &used);
      // alters the given formula, replacing all products of an
      // integer and an unknown by a single unknown
    void save_sums(PFormula formula);
      // alters the given formula, replacing sums a1 + ... + an by a
      // single unknown representing the sum
    PPol handle_sums(PPol p, map<int,IntMap> &used_known,
                     map<int,IntMap> &used_unknown);
      // helping function for save_sums
    void guarantee_existence(map<int,IntMap> &used, int id1, int id2,
                             int min, int max);
      // helping function for the various handle_ functions; checks
      // whether used[id1][id2] is set and if not, creates a new
      // entry for used[id1][id2] with the given minimum and maximum
      // values
    void test_equal(int a, int b, int overflow, string message);
      // give an error message if a != b; any two numbers >= overflow
      // are considered equal
    void test_solution(vector<int> &values, int overflow);
      // checks whether values corresponds with sums / product
      // equalities

  public:
    Smt(vector<int> &minima, vector<int> &maxima);
      // sets up the solver to solve the values for the given
      // unknowns, within the given bounds; the given bounds may be
      // high, but the prover generally won't attempt more than the
      // lowest few values

    bool solve(PFormula formula, vector<int> &values);
      // attempts to find solutions (both in Formula-variables and
      // in Polynomial-unknowns) so formula is satisfied
};

#endif

