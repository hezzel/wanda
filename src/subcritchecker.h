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

#include "dependencypair.h"
#include "formula.h"

typedef map<string,int> ArList;

class SubtermCriterionChecker {
  private:
    And *formula;
    ArList arities;
    map<string,int> varstart;
    int strictstart;

    bool non_collapsing(DPSet &set);
    void get_maximal_arities(DPSet &set);
      // finds maximal arities for all TOPMOST symbols, ignoring
      // what happens inside the terms, and saves them in arities
    void declare_nu_variables();
      // "declare" variables X_{f,i}, and adds the requirement that
      // always nu(f) = i for exactly one i to formula
    void declare_strictness_variables(int setsize);
      // "declare" variables Y_i indicating that dependency pair
      // i is strictly oriented, and add the requirement that at
      // least one pair is strictly oriented
    void add_main_requirements(DPSet &set, bool accessible,
                               map<string,int> &sortordering,
                               ArList &arities);
      // for all dependency pairs DP(i) = l ~~> p, add the
      // requirement that either nu(l) |> nu(p), or nu(l) = nu(p) /\
      // -Y_i (where Y_i indicates strict orientation)
    int check_subterm(PTerm a, PTerm b);
      // returns 0 if a = b, 1 if a |> b, -1 otherwise
    int check_accessible(PTerm a, PTerm b, map<string,int> &sortord,
                         ArList &arities);
      // returns 0 if a = b, 1 if a ]] b, -1 otherwise
    bool check_accessible_meta_or_subterm(PTerm a, PTerm b,
                                       PVariable Z,
                                       map<string,int> &sortordering,
                                       ArList &arities);
    bool check_accessible_subterm(PTerm a, PTerm b,
                                  map<string,int> &sortordering,
                                  ArList &arities);
    bool positions_okay(PType sigma, string kappa,
                        map<string,int> &sortord, bool positive);
      // returns whether Pos^x(kappa, sigma) = Pos(kappa, sigma),
      // where x is + if positive is true, otherwise -
    void get_solution(DPSet &set, DPSet &strict, DPSet &nonstrict,
                      Alphabet &F, ArList &arities, bool accessible);
      // assuming all SAT-variables are instantiated, print the
      // solution in the output module, and put the strictly oriented
      // dependency pairs in the removed set

  public:
    bool run(DPSet &dps, DPSet &strict, DPSet &nonstrict,
             Alphabet &F, ArList &arities);
      // tries to find a suitable projection function for the given
      // set of dependency pairs; returns true if the criterion was
      // applicable and splits dps into the pairs which are ordered
      // strictly, and the pairs which are ordered non-strictly (so
      // with =);
      // the alphabet and arity list are only used for printing!

    bool run_static(DPSet &dps, DPSet &strict, DPSet &nonstrict,
                    Alphabet &F, ArList &arities,
                    map<string,int> &sortordering);
      // tries to apply the accessible subterm criterion rather than
      // the normal one
};

