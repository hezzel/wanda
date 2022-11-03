/**************************************************************************
   Copyright 2012, 2013, 2019 Cynthia Kop

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
#include "dependencygraph.h"
#include "alphabet.h"
#include "rulesmanipulator.h"
#include "firstorder.h"
#include "orderingproblem.h"

/**
 * This class implements the global dependency pair framework.
 *
 * This roughly follows the definitions of [Kop12], but we don't
 * consider the full dependency pair framework; all dependency pairs
 * are stored in a global graph, which is updated by removing
 * dependency pairs only.  While both termination and non-termination
 * properties are preserved, this implementation does not attempt to
 * find counterexamples for termination, except as a side effect to
 * calls to the first-order prover.
 *
 * At first, given the symbols and rules present in the system, its
 * dynamic dependency pairs are calculated; pairs originating from
 * first-order rules are skipped, as the first-order part is directly
 * fed into a first-order termination tool.  If the system is
 * plain-function-passing, eta-long, and the right-hand sides of
 * rules do not have subterms /\x.C[f ... D[x] ...] with f a defined
 * function symbol, collapsing dependency pairs are also omitted, for
 * in this case the non-collapsing dynamic dependency pairs are
 * exactly the collapsing static dependency pairs.
 *
 * Subsequently the dependency graph is calculated, pairs and
 * connections which cannot lead to self-loops are removed, and the
 * framework attempts to solve all the cycles.  Various methods can
 * be employed to prove cycles terminating, although at present we
 * only consider HORPO, polynomial interpretations and the subterm
 * criterion, combined with usable and formative rules.
 *
 * Textual information as to what is done is stored in the output
 * module.
 */

class DependencyFramework {
  private:
    vector<DPSet> Ps;
    vector<Ruleset> Rs;

    class DPProblem {
      public:
        int P;
        int R;
        DPProblem(int _P, int _R) :P(_P), R(_R) {}
    };

  private:
    Alphabet F;
    RulesManipulator manip;
    FirstOrderSplitter splitter;
    map<string,int> arities;
    map<string,int> sortordering;
    set<string> defineds;
    vector<DPProblem*> problems;
    vector<MatchRule*> original_rules;
      // stored for checking and comparing, but not considered
      // property of the framework

    int FOstatus; // 0: unchecked, 1: terminating, 2: non-terminating
    bool expanded;
    bool allow_graph;
    bool allow_static;
    bool allow_dynamic;
    bool allow_subcrit;
    bool allow_polynomials;
    bool allow_product_polynomials;
    bool allow_horpo;
    bool allow_usable;
    bool allow_formative;
    bool allow_uwrt;
    bool allow_fwrt;

    bool formative_flag;
    int static_flag;
      // these two flags are preserved for all DP problems we
      // technically use; if we are not considering static dependency
      // chains, then we consider minimal ones
    bool afs; // indicates whether this system is an afs
    bool leftlinear;
    bool fullyextended;
    bool abstraction_simple;

    bool found_counterexample;
    string counterexample;
    bool orthogonality_for_counter;
    bool original_firstorder;
      // used for some features which have not been proved sound in
      // the higher-order setting

    void free_memory();
      // empties each of Ps, Rs and problems, and frees the memory
      // they use

    void setup(bool skipFO);
      // having established all the properties (such as the static
      // flag and left-linearity of the system), calculates the
      // dependency pairs and initialises the problems set; if
      // skipFO is set to true, then first-order dependency pairs are
      // not included

    PTerm up(PTerm up);
      // given f(s1,...,sn), returns f#(s1,...,sn); given any other
      // term, just returns a copy

    void add_pair(PTerm l, PTerm p, int style, vector<int> noneating,
                  DPSet &DP);
      // adds the given dependency pair, unless it corresponds to an
      // existing pair (whose cached string version is in compare)

    void add_top_dp(PTerm l, PTerm r, DPSet &DP);
      // adds topmost dependency pairs l\vec{x} ==> r\vec{x} if r is
      // headed by a meta-variable application, and is a functional
      // term

    void add_normal_dp(PTerm l, PTerm r, vector<int> noneating,
                       DPSet &DP);
      // adds normal dependency pairs for the rule l -> r
      // this will not add collapsing pairs if the static flag is
      // set

    bool search_firstorder_counterexample(Ruleset &FOR, string &explanation);
      // tries to find a counterexample to termination for a
      // non-orthogonal system, using an external first-order
      // tool

    bool first_order_part_terminating(Ruleset &R);
      // returns whether the first-order subset of the rules is terminating

    void user_information();
      // prints information about the current settings to the user
      // at the start of a dependency pair approach

    void list_problems();
      // lists all the open dependency pair problems (without naming
      // the respective rules and dependency pairs explicitly)

    void print_problem(DPProblem *prob);
      // prints the given dependency pair problem on the output module

    string print_problem_brief(DPProblem *prob);
      // prints (P_i, R_j), with appropriate flags

    bool termination_loop();
      // iterates over the current dependency pair problems, trying to
      // prove termination; returns true iff these attempts succeed

    bool force_static_approach();
      // eta-expands the system and restarts the dependency pair
      // approach with static dependency pairs (if possible)

    bool graph_processor(DPProblem *prob);
      // tries to apply the dependency graph processor on the given
      // DP problem; returns true if this had any effect (in this
      // case the resulting problems are added to the problems list)

    bool empty_processor(DPProblem *prob);
      // tries to apply the empty set processor on the given DP
      // problem

    bool subcrit_processor(DPProblem *prob);
      // tries to apply the subterm criterion processor on the given
      // DP problem; returns true if this had any effect (in this
      // case the resulting problem is added to the problems list)

    bool static_processor(DPProblem *prob);
      // tries to apply the static subterm criterion processor on the
      // given DP problem; returns true if this had any effect (in
      // this case the resulting problem is added to the problems
      // list)

    bool formative_processor(DPProblem *prob);
      // tries to apply the formative rules processor on the given DP
      // problem

    bool redpair_processor(DPProblem *prob);
      // tries to apply the reduction pair processor, with horpo or
      // polynomial interpretations, on the given DP problem

    bool horpo_processor(DPProblem *prob, DPOrderingProblem *ord,
                         bool with_nasty_eta);
    bool poly_processor(DPProblem *prob, DPOrderingProblem *ord,
                        bool with_pprod);
      // tries to apply either reduction pair processor on the given
      // DP problem

    void add_reduced_problem(DPProblem *prob, vector<int> &ok);
      // adds a new problem to the dependency pair list, which is
      // prob with all elements in ok removed

  public:
    DependencyFramework(Alphabet &Sigma, Ruleset &rules,
                        string fotool, string fonontool,
                        bool allow_static = true,
                        bool allow_dynamic = true);
    ~DependencyFramework();

    bool terminating();
      // returns true if the system is terminating, false if this
      // could not be determined

    bool proved_non_terminating();
      // returns true if a counterexample was encountered during the
      // termination checks - should only be called AFTER you call
      // terminating()!

    bool first_order_non_terminating();
      // does the basic non-terminating checks, by asking a
      // first-order tool whether the first-order part of the system
      // is terminating and verifying the counterexample (if any);
      // can be called both before or after terminating(), with neither
      // option being more efficient than the other

    void document_non_terminating();
      // prints documentation to the output module explaining why the
      // system is not terminating (only call if
      // first_order_non_terminating or proved_non_terminating
      // returns true!)

    void disable_graph();
    void disable_subcrit();
    void disable_polynomials();
    void disable_product_polynomials();
    void disable_horpo();
    void disable_formative();
    void disable_usable();
    void disable_fwrt();
    void disable_uwrt();
    void disable_abssimple();
};

