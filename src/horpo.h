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

#ifndef HORPO_H
#define HORPO_H

#include "alphabet.h"
#include "formula.h"
#include "horpoconstraintlist.h"
#include "requirement.h"
#include "orderingproblem.h"

class Horpo {
  private:
    OrderingProblem *problem;

    string comments;
    bool monomorphic;
      // set to true if all occurrences of application have a
      // monomorphic type; variables and function symbols may be
      // non-monomorphic

    map<string,int> alphabet;
    HorpoConstraintList constraints;
    map<OrderRequirement*,int> greater_requirements;
    map<OrderRequirement*,int> geq_requirements;

    // boundaries for the standard variables
    map<string,int> var_symbol_filtered;
    map<string,int> var_permutation;
    map<string,int> var_arg_length_min;
    map<string,int> var_minimal;
    map<string,int> var_precedence;
    map<string,int> var_lex;

    string str(int num);
      // turns a number into a string
    bool has_monomorphic_applications(PTerm term);
      // returns whether all occurrences of some subterm a*b that
      // cannot be seen as a functional term have monomorphic type
    void make_application_free(PTerm term);
      // makes the given meta-term application-free
    void save_constraints();
      // saves application-free variations of the ordering constraints
      // in the list of horpo constraints
    void create_basic_variables();
      // allocates variables for the precedence, status and argument
      // function
    void save_argfun_constraints();
      // saves constraints for the argument function
    void save_precedence_constraints();
      // saves constraints for the precedence (this includes the
      // requirement that the status respects the precedence)
    void force_minimality();
      // sets the formulas so symbols of arity 0 which only occur in
      // the left-hand sides of the constraints are mapped to minimal
      // symbols
    bool same_typevars(PType a, PType b, bool exactly);
      // returns whether all type variables occurring in b also
      // occur in a, and at least as often; if exactly is true, then
      // they must occur exactly as often in a as in b
    void check_irrelevant_constraints(PFormula formula);
      // the sat solver has a tendency to make variables false if they
      // are not necessary for the formula to hold; consequently, only
      // one constraint is typically eliminated, as the rest is
      // irrelevant
      // since it is preferable to get rid of more than one constraint
      // every time, we check here which constraint formulas are
      // irrelevant, and make them true!
      // the formula is automatically deleted afterwards

  public:
    Horpo();
    vector<int> orient(OrderingProblem *prob);

    string query_comments();

    // getting the variables out (used by other horpo classes)
    int arg_filtered(string symbol, int index);
    int symbol_filtered(string symbol);
    int permutation(string symbol, int id1, int id2);
      // returns whether pi(id1) = id2
    int arg_length_min(string symbol, int len);
    int minimal(string f);
    int lex(string f);

    int prec(string f, string g);
    int precstrict(string f, string g);
    int precequal(string f, string g);
      // should be called only for elements of alphabet, and can
      // give false results in a polymorphic system when you should
      // call the alternative functions with actual constants
    int precstrict(PConstant f, PConstant g);
    int precequal(PConstant f, PConstant g);

    vector<Or*> force_type_equality(PType type1, PType type2);
      // if there is no type changing function such that type1 ==
      // type2, returns a vector containing an empty Or()
      // (note that an empty Or() simplifies to false)
      // if there is such a type changing function, saves the
      // constraints for it in the return value
      // in this case, the returned vector may be empty, too
      // same as the other force_type_variable, 

    vector<Or*> query_justification(int variable);
      // returns a list of Ors (all fresh copies, so can be altered
      // as necessary) indicating the formulas which justify the given
      // variable (which is assumed to be the index of a constraint)

    void get_constraint(int variable, PTerm &left, PTerm &right,
                        string &relation, string &rule);
      // checks the constraint list for a constraint with the given
      // index, and alters the remaining parameters to reflect the
      // constraint as well as possible
    int get_constraint_index(OrderRequirement *req);
      // returns the index in the constraint list of the strict or
      // nonstrict version of the given ordering requirement,
      // depending on the valuation of the associated formula; this
      // requirement must be one of the orientables of the
      // OrderingProblem HORPO was initialised with, and the
      // corresponding variables must have a valuation set!
};

#endif

