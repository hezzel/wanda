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

#ifndef FIRSTORDER_H
#define FIRSTORDER_H

/**
 * This class helps the Dependency Pair approach by recognising a
 * first-order problem, if such a cycle is given, and employing a
 * usable rules approach on it.
 */
#include <set>
#include "matchrule.h"
#include "dependencypair.h"
#include "alphabet.h"
#include "rulesmanipulator.h"

typedef vector<MatchRule*> Ruleset;

class FirstOrderSplitter {
  private:
    RulesManipulator manip;
    set<string> PHO, TFO;
    string fotool;
    string fonontool;

    bool first_order(PTerm term);
    string print_functionally(PTerm term, Environment &gamma);
    void create_file(vector<MatchRule*> &rules, bool innermost, string fname);
    void create_sorted_file(vector<MatchRule*> &rules, bool innermost,
                            string fname);
    void get_constant_data(PTerm term, Alphabet &F);
    bool valid_counterexample(string example, Alphabet &F);
    void update_connections(map<string, set<string> > &graph,
                            set<string> &connections, string point);
    set< set<string> > split_types(Ruleset &R);
    string determine_termination_main(vector<MatchRule*> &rules,
                                      bool innermost, string &reason);
    bool single_sorted_alphabet(Alphabet &F);

  public:
    FirstOrderSplitter(Alphabet &Sigma, Ruleset &Rules,
                       string FOtool, string FOnontool);
    bool first_order(DPSet &D);
    bool first_order(MatchRule *rule);
    Ruleset first_order_part(Ruleset &R);
    string determine_termination(vector<MatchRule*> &rules,
                                 bool innermost,
                                 string &reason);
      // returns whether the given first-order term rewrite system
      // is (innermost) terminating (YES, NO or MAYBE); reason
      // contains a proof
    string determine_nontermination(vector<MatchRule*> &rules,
                                 bool innermost,
                                 string &reason);
      // similar to determine_termination, but focuses on proving
      // non-termination; moreover, this only returns "NO" (meaning
      // non-termination) if a SORTABLE counterexample was found;
      // reason contains a proof in the case of YES or NO
    string query_tool();
    string query_tool_name();
    string query_non_tool();
    string query_non_tool_name();
};

#endif

