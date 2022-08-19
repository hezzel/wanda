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

#ifndef REQUIREMENT_H
#define REQUIREMENT_H

#include "term.h"
#include "environment.h"
#include "formula.h"

/**
 * An OrderRequirement is one of the following:
 * - a constraint X => l >= r
 * - a constraint (X => l > r) /\ (-X => l >= r)
 *
 * You don't create OrderRequirements directly, but rather use the
 * static access functions.
 */
class OrderRequirement {
  public:
    PTerm left;
    PTerm right;

  private:
    bool strong;
    unsigned int condition;
    vector<int> data;
      // indexes of rules / dependency pairs corresponding to this
      // ordering requirement

    OrderRequirement(PTerm l, PTerm r, bool s, unsigned int c)
      :left(l), right(r), strong(s), condition(c) {}

  public:
    /* creates the requirement: True => l >= r */
    static OrderRequirement *geq(PTerm l, PTerm r) {
      return new OrderRequirement(l, r, false, vars.true_var());
    }

    /* creates the requirement: condition => l >= r */
    static OrderRequirement *geq(PTerm l, PTerm r, unsigned int c) {
      return new OrderRequirement(l, r, false, c);
    }

    /* creates the requirement: (c => l > r) /\ (-c => l >= r) */
    static OrderRequirement *maybe_greater(PTerm l, PTerm r, int c) {
      return new OrderRequirement(l, r, true, c);
    }

    /* creates the requirement: (c => l > r) /\ (-c => l >= r) and
    immediately initalises data */
    static OrderRequirement *maybe_greater(PTerm l, PTerm r, int c,
                                           int d) {
      OrderRequirement *ret = new OrderRequirement(l, r, true, c);
      ret->data.push_back(d);
      return ret;
    }

    /* deletes left and right, if they still exist */
    ~OrderRequirement() {
      if (left != NULL) delete left;
      if (right != NULL) delete right;
    }

    /* returns a copy of the current OrderRequirement */
    OrderRequirement *copy() {
      return new OrderRequirement(left->copy(), right->copy(),
                                  strong, condition);
    }

    /* returns true if this requirement has the maybe_greater form */
    bool definite_requirement() { return strong; }

    /* updates the condition variable for this requirement, returns
       the old one */
    unsigned int update_condition(unsigned int newcond) {
      unsigned int c = condition;
      condition = newcond;
      return c;
    }

    /* returns a formula indicating that left > right is necessary */
    PFormula orient_greater() {
      if (strong) return new Var(condition);
      else return new Bottom;
    }

    /* returns a formula indicating that left >= right is necessary
    (note: this returns false if left > right is necessary!) */
    PFormula orient_geq() {
      if (strong) return new AntiVar(condition);
      else return new Var(condition);
    }

    /* returns a formula indicating left R right is necessary */
    PFormula orient_at_all() {
      if (strong) return new Top;
      else return new Var(condition);
    }

    /* returns the current valutation for the condition */
    Valuation condition_valuation() {
      return vars.query_value(condition);
    }

    /* if this is a strict constraint, force it to be oriented
    strictly if at all possible */
    void force_strict() {
      if (strong && condition != vars.false_var())
        vars.force_value(condition, TRUE);
    }

    /* prints the current requirement */
    string to_string() {
      Environment gamma;
      string ret = left->to_string(gamma);
      if (strong) ret += " >? ";
      else ret += " >= ";
      ret += right->to_string(gamma);
      return ret;
    }

    /* stores information which can later be queried */
    void add_data(int num) { data.push_back(num); }

    /* returns all numbers added as data to this requirement */
    int data_total() { return data.size(); }
    int get_data(int index) { return data[index]; }
};

#endif

