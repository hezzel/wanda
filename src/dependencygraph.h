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

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include "dependencypair.h"
#include "typer.h"
#include "matchrule.h"

typedef vector<bool> graph_entry;
typedef vector<int> intlist;

class DependencyGraph {
  private:
    Typer typer;

    vector<MatchRule*> rules;
    vector<DependencyPair*> pairs;
      // given (not copied) from the ones in the dependency framework
    vector<graph_entry> graph;
    vector<graph_entry> reachable;
    map<string,graph_entry> noneatingpos;
      // noneatingpos[f,i] is true if a variable occurring somewhere
      // in the i^th argument of f cannot be reduced away
    map<string,bool> can_reduce_to;
      // can_reduce_to[f : g] is true if a term headed by symbol f
      // can reduce to a term headed by symbol g; f must be a function
      // symbol, but g may also be #ABS or #VAR

    bool is_constructor(PTerm symbol);
      // returns whether the given symbol is a constructor

    /* ======== confirming or rejecting edges in the graph ======= */

      // splits an application into its components
    bool connection_possible(DependencyPair *p1, DependencyPair *p2);
      // returns true if the dependency graph approximation should
      // have an edge from p1 to p2
    bool reduction_possible(PTerm from, PTerm to,
                            DependencyPair *to_source);
      // returns true if there might be a reduction from an
      // instantiation of from to an instantiation of to (gives false
      // positives, but no false negatives)
    Varset get_certain_variables(PTerm term,
                                 DependencyPair *dp);
      // returns all variables (not meta-variables) occurring inside
      // term, excepting those inside potentially eating
      // meta-applications (dp is given for the eating check)
    bool at_non_eating_pos(PTerm s, int Z);
      // returns whether Z occurs in s at a position that is
      // definitely safe from eating variables (using noneatingpos)
      
    void get_eating_info(Alphabet &Sigma);
      // fill the noneatingpos mapping
    void get_reduction_info(Alphabet &Sigma);
      // fill the can_reduce_to mapping

    /* ============= determining cycles in the graph ============= */
    
    void calculate_reachable();
      // fill the reachable array, so reachable[i][j] is true if
      // there is a path from i to j in the array

  public:
    DependencyGraph(Alphabet &Sigma, DPSet &P, vector<MatchRule*> &R);

    string to_string();
      // debug functionality

    void print_self();
      // proper printing functionality, uses the output module
      // the graph is printed with numbers as edges

    void print_self(Alphabet &F, map<string,int> &arities);
      // proper printing functionality, uses the output module
      // the graph is printed with numbers as edges; unlike the other
      // print_self, a legenda is printed to explain what dependency
      // pairs the numbers refer to

    DPSet get_scc();
      // returns a single strongly connected component

    vector<DPSet> get_sccs();
      // returns all strongly connected components; if keep_graph is
      // false, this is a bit more efficient, but the graph cannot be
      // reused afterwards

    void remove_pairs(DPSet &pairs);
      // should be the same pairs as returned by get_scc or get_sccs
      // (or a subset), not copies!
};

#endif

