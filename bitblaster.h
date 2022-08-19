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

#ifndef BITBLASTER_H
#define BITBLASTER_H

#include "polynomial.h"
#include "formula.h"

#define MAXBITS 8

typedef pair<int,int> IntPair;
typedef map<int,IntPair> PairMap;

/**
 * This class takes a number of polynomial constraints without variables
 * or functionals, as well as bounds for the unknowns in the polynomials,
 * and finds a satisfying assignment, if one exists, by encoding the
 * constraints into a satisfiability problem of proposition logic and
 * sending this to a sat-solver.
 */

class BitBlaster {
  private:
    vector<int> minima, maxima, bitstart, numbits;
    int interestingnums;
    And *sat_formula;
    int TRUEBIT, FALSEBIT;

    
    void handle_formula(PFormula formula);
      // encodes the given formula into sat_formula (without
      // affecting the given formula)

    int bit(int index, int i);
      // returns the i^th bith of unknown index
    int overflow_bit(int index);
      // returns the overflow bit of unknown index (if it has one)
    int new_var();
      // creates a new propositional variable and returns it

    void is_equal(int x, int y, int anticond = -1);
      // adds the requirement that bit x = bit y
    void is_different(int x, int y, int anticond = -1);
      // adds the requirement that bit x != bit y
    void is_and(int x, int y, int z, int anticond = -1);
      // adds the requirement that bit x = bit y /\ bit z, or anticond
    void is_or(int x, int y, int z, int anticond = -1);
      // adds the requirement that bit x = bit y /\ bit z, or anticond
    void is_atleasttwo(int x, int y, int z, int u, int anticond = -1);
      // adds the requirement that x holds if and only if at least
      // two out of y, z and u are true, or anticond is true
    void is_iff(int x, int y, int z, int anticond = -1);
      // adds the requirement that x holds if and only if y <-> z,
      // or anticond is true
    void is_xor(int x, int y, int z, int anticond = -1);
    void is_triple_xor(int x, int y, int z, int u, int anticond = -1);
      // adds the requirement that x holds if and only if y xor z,
      // or y xor z xor u, holds, or anticond is true

    int recover_number(int index);
      // determines the value of the given number (possibly a bit too
      // low, in case of overflows)
    vector<int> representation(int unknown);
      // takes an unknown and turns it into a vector of bits
    vector<int> number_representation(int number);
      // turns a number into a vector of forced-true or forced-false
      // bits

    void addreq(PFormula formula);
      // adds formula->simplify() as a child of sat_formula
    void addreqif(PFormula formula, int conditional);
      // if conditional = -1, adds formula as a requirement
      // otherwise, adds conditional -> formula
    void addreqifnot(PFormula formula, int conditional);
      // if conditional = -1, adds formula as a requirement
      // otherwise, adds -conditional -> formula

    void inequality_left(int unknown, int num, int cond = -1);
      // adds the requirement: a_{unknown} >= num
      // if cond is given, this becomes c -> a_u >= n
    void inequality_right(int num, int unknown, int cond = -1);
      // adds the requirement: num >= a_{unknown}
      // if cond is given, this becomes c -> n >= a_u
    void inequality_both(int unknown1, int unknown2, int cond = -1);
      // adds the requirement: unknown1 >= unknown2
      // if cond is given, this becomes c -> u1 >= u2
    void inequality(PPol a, PPol b, int cond = -1);
      // adds the requirement cond -> a >= b
    void set_sum(int result, vector<int> &a, vector<int> &b);
      // adds the requirement: result = a + b
    void set_product(int result, vector<int> &a, vector<int> &b);
      // adds the requirement: result = a * b

  public:
    BitBlaster(vector<int> &minima, vector<int> &maxima);
      // sets up the bit blaster to solve the values for the given
      // unknowns, with given minimum and maximum values

    void set_squares(map<int,int> &squares);
      // make sure that if squares[i] = j then ai = aj * aj
    void set_unknown_products(PairMap &prods);
      // make sure that if prods[i] = (j,k) then ai = aj * ak
    void set_known_products(PairMap &prods);
      // make sure that if prods[i] = (j,k) then ai = j * ak
    void set_unknown_sums(PairMap &sums);
      // make sure that if sums[i] = (j,k) then ai = aj + ak
    void set_known_sums(PairMap &sums);
      // make sure that if sums[i] = (j,k) then ai = j + ak

    int overflow_bound();
      // all numbers above <return value> are considered equal

    bool solve(PFormula formula, vector<int> &values);
      // takes all l >= r constraints in formula and replaces them by
      // SAT formulas, then calls the SAT-solver, and if this is
      // successful, sets the values of the unknowns and returns true
      // formula is deleted
};

#endif

