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

#ifndef ALGEBRA_FRAMEWORK_H
#define ALGEBRA_FRAMEWORK_H

#include "alphabet.h"
#include "orderingproblem.h"
#include "polynomial.h"

#include "polconstraintlist.h"

#define MAX_UNKNOWN 3
#define MAX_SPECIAL_UNKNOWN 7

typedef map<string,int> ArList;

class PolyModule {
  private:
    OrderingProblem *problem;
    bool do_base_products;
    ArList arities;

    void comment(string txt);

    map<string,PolynomialFunction*> interpretations;
    map<string,int> argfunid;
    vector<int> minimum, maximum;
      // has lower and upper bounds for all unknowns
    PolConstraintList constraints;

    bool monomorphic();
    void choose_interpretations();
      // chooses interpretations \x1...xn.p for all symbols in the
      // alphabet of problem
    vector< vector<int> > combinations(vector<int> &numbers, int n);
      // returns all combinations of length n of the variables
      // numbers[0] ... numbers[numbers.size()-1] (admitting
      // duplicates)
    PType type_over_N(PType type);
      // creates a copy where all data types are replaced by N
    void filter_check(string symbol, int pos, PPol pol);
      // enforces that pol = 0 if position pos of symbol is
      // filtered away
    void interpret_with_information(PTerm left, PTerm right,
             PolynomialFunction *&l, PolynomialFunction *&r,
             map<int,int> &metarename);
    void interpret_requirements();
    PolynomialFunction *interpret(PTerm term,
                           map<int,PolynomialFunction*> &subst);
    PPol interpret(PTerm term, map<int,PolynomialFunction*> &subst,
                   vector<PolynomialFunction*> &args);
    PolynomialFunction *make_nul(PType type);

    vector<int> get_solution();
      // assuming all unknowns are known, or can be set to anything
      // in their range, returns which dependency pairs have been
      // strictly oriented and comments an explanation of the
      // constraints
    void choose_meta_namings(PTerm term, Environment &gamma,
                             map<int,int> &metarename,
                             map<int,int> &indexrename, int &n);
      // chooses a renaming of the meta-variables in term which is
      // consistent with the naming we will use for variables in the
      // polynomials (metarename maps variable indexes to polvar
      // indexes, indexrename maps polvarindexes to a "name")
    int value_at_nul(PPol pol);
      // assuming there are no unknowns in pol, and pol is simplified,
      // returns its value when all variables and functionals are
      // assumed to be 0
  
  public:
    PolyModule();
    ~PolyModule();
  
    void set_use_products(bool value);
    vector<int> orient(OrderingProblem *prob);
      // orients the given ordering requirements using a polynomial
      // interpretation returns the data for the ones which have been
      // strictly oriented

    PolynomialFunction *make_variable(PType type, int id);
      // helping function which is also used in PolConstraintList
    Unknown *new_unknown(int maximum = MAX_UNKNOWN);
      // creates a new unknown which ranges from 0-maximum
      // (also a helping function used in PolConstraintList)
};

#endif

