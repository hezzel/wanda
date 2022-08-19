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

#ifndef HORPOJUSTIFIER_H
#define HORPOJUSTIFIER_H

#include "alphabet.h"
#include "environment.h"
#include "orderingproblem.h"

class Horpo;

class HorpoJustifier {
  private:
    Horpo *horpo;
    map<string,int> arities;

    void start_justification();
    bool show_modifications(vector<string> &symbols);
    void show_split(vector<string> &symbols);
    void show_precedence(vector<string> &symbols);
    void show_filtered_constraints(OrderingProblem *problem);
    void show_explanation(OrderingProblem *problem);

    void register_constant(string name, PType type, int arity,
                           Alphabet &alf, map<string,int> &ars);
    PTerm filter(PTerm term, Alphabet &alf, map<string,int> &ars);
    string print_filtered(PTerm term, map<int,string> &metaname,
                          map<int,string> &freename);
    void justify(int variable, int &printindex, map<int,string> &metaname,
                 map<int,string> &freename, map<int,int> &previous_proofs,
                 vector< vector<string> > &entries);
      // the function alters printindex, changing it to the
      // number where the next justification should be printed

    // these functions just query the values of the corresponding
    // variables in the horpo class
    bool arg_filtered(string symbol, int index);
    bool symbol_filtered(string symbol);
    bool permutation(string symbol, int i, int j);
    bool arg_length_min(string symbol, int num);
    bool minimal(string f);
    bool prec(string f, string g);
    bool precstrict(PConstant f, PConstant g);
    bool precequal(PConstant f, PConstant g);
    bool lex(string f);

  public:
    void run(Horpo *horpo, OrderingProblem *op,
             map<string,int> &alphabet);
      // req_indexes contains the indexes of the variables associated
      // to the given constraints: first reqs.size() indexes for the
      // >= constraints, then in order the indexes for the > constraints
};

#endif

