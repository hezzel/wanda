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

#ifndef HORPOCONSTRAINTS_H
#define HORPOCONSTRAINTS_H

#include "requirement.h"
#include "formula.h"

class Horpo;

struct HorpoConstraint {
  PTerm left;
  PTerm right;
  string relation;  // >, >=, >=A, where A in {Fun,Mul,...,stdr,RST}
  PTerm restriction_term;
  int restriction_num;
  int variable_index;
  int justification_start;

  HorpoConstraint(PTerm _left, PTerm _right, string _relation,
                  PTerm _resterm, int _resnum, int _varindex);

  ~HorpoConstraint();
};


class HorpoConstraintList {
  private:
    vector<PFormula> formulas;
    vector<HorpoConstraint*> constraints;
    int handled;
    Horpo *master;
    Environment environment;
      // used for printing the meta-variables in the constraints;
      // these must be consistently printed with the same names

    map<string,int> constraint_lookup;

    string print_horpo_constraint(PTerm left, PTerm right, string relation,
                                  PTerm resterm, int resnum);
    string print_horpo_constraint(HorpoConstraint *constraint);
    int add_constraint(PTerm left, PTerm right, string relation,
                       PTerm resterm, int resnum);
    int index_of_constraint(PTerm left, PTerm right, string relation,
                            PTerm resterm, int resnum);
      // looks up the index of the given constraint, possibly adding
      // it if it doesn't exist yet; this is the index in the constraint
      // list
    int var_for_constraint(PTerm left, PTerm right, string relation,
                           PTerm resterm = NULL, int resnum = 0);
      // looks up the value of variable_index corresponding to the
      // given constraint (adding the constraint if necessary)

    void handle_greater(PTerm left, PTerm right, int index);
    void handle_basic_geq(PTerm left, PTerm right, int index);
    void handle_standard_right(PTerm left, PTerm right, int index,
                               PTerm resterm, int resnum);
      // handles the >=stdr case, but also the >=RST case if we
      // already know that the >=RST path does not need to be
      // aborted
    void handle_fun(PTerm left, PTerm right, int index);
    void handle_eta(PTerm left, PTerm right, int index,
                    map<string,int> &arities);
    void handle_lex(PTerm left, PTerm right, vector<PTerm> &lsplit,
                    vector<PTerm> &rsplit, int index, string fname,
                    string gname, int n, int k, int m);
    void handle_mul(PTerm left, PTerm right, vector<PTerm> &lsplit,
                    vector<PTerm> &rsplit, int index, string fname,
                    string gname, int n, int k, int m);
    void handle_stat(PTerm left, PTerm right, int index,
                     map<string,int> &arities);
    void handle_fabs(PTerm left, PTerm right, int index);
    void handle_copy(PTerm left, PTerm right, int index);
    void handle_select(PTerm left, PTerm right, int index,
                       PTerm resterm, int resnum);
    void handle_restricted(PTerm left, PTerm right, int index,
                           PTerm resterm, int resnum);
    bool markable(PTerm term);
    PConstant get_main_symbol(PTerm term);
      // assuming term has the form lambda x1...xn.f s1 ... sm,
      // returns the constant f (segfaults if term does not have this form)
    Or *bigOr(PFormula a, PFormula b, PFormula c, PFormula d);
    Or *bigOr(PFormula a, PFormula b, PFormula c, PFormula d, PFormula e);
    PTerm retype(PTerm term, PType newtype);
      // changes the type of the constant in a term f* l1 ... ln, and
      // returns the result; this result uses parts of the original
      // term and should not just be deleted!
    void destroy_retyped(PTerm term);
      // delete a retyped version in a way that does not destroy the
      // parts occurring in the original
    int measure(PTerm term);
      // determines a measure for a term; for an RST step l >= r (RST
      // l',n) to lose the restriction, measure(l) must be
      // <= measure(l')

  public:
    HorpoConstraintList(Horpo *_master);

    void reset();
      // clears the list
    int add_geq(PTerm left, PTerm right);
      // adds a constraint of the form l >= r, and returns its formula index
    int add_greater(PTerm left, PTerm right);
      // adds a constraint of the form l > r, and returns its formula index
    void simplify(map<string,int> &arities);
      // goes through all constraints, and marks them as done after
      // adding suitable clauses to the list of formulas
    void add_formula(PFormula formula);
    And *generate_complete_formula();
      // creates the conjunction of the added formulas; this is a
      // copy, so may be manipulated as the caller sees fit
    int query_constraint_variable(PTerm left, PTerm right, string relation);
      // returns the variable associated to the given constraint; the
      // constraint MUST already exist before this is called!
    HorpoConstraint *constraint_by_index(int index);
      // given a variable index for an existing constraint, returns
      // the constraint this variable corresponds to
    vector<PFormula> justify_constraint(int index);
      // given a variable index for an existing constraint, returns
      // the formulas which were generated to simplify that constraint
    vector<int> constraint_variables();
      // list all variables used for a constraint
    string print();
      // returns debugging information
};

#endif

