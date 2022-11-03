/**************************************************************************
   Copyright NEWYEAR Cynthia Kop

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

#ifndef ORDERING_PROBLEM_H
#define ORDERING_PROBLEM_H

#include "alphabet.h"
#include "formula.h"
#include "matchrule.h"
#include "dependencypair.h"
#include "requirement.h"

typedef map<string,int> ArList;

/**
 * These classes turn a problem (either a set of rules, or a
 * dependency pair problem) into a set of ordering requirements, along
 * with formulas indicating under what conditions which requirements
 * need to be oriented.
 * This takes usable and formative rules into account, as well as
 * tagging. Since the conditions for this rely on argument filtering,
 * the class also manages a set of logical variables indicating
 * whether certain function arguments are filtered away.
 */

typedef vector<MatchRule*> Ruleset;
typedef vector<PType> TypeList;
typedef map<string,TypeList> SymbolList;

class OrderingProblem {
  private:
    map<string, int> filterable;
      // filtered[name] + index - 1 = [variable indicating that
      // position <index> (1-based) of function <name> is filtered
      // away]; any symbol not in this list cannot have any parameters
      // filtered!

    int unfiltered_properties;
      // 0 if neither monotonicity or subterm steps are required;
      //   moreover, application is not unfilterable
      // 1 if we we do require subterm steps for unfilterable symbols
      // 2 if we require strong monotonicity for unfilterable symbols
  
    void get_actual_arities(DPSet &dps, Ruleset &rules);
      // saves the actual arities as they occur in the rules

  protected:
    Alphabet alphabet;
      // the alphabet over which the ordering requirements are built
    vector<OrderRequirement*> reqs;
      // ordering requirements
    vector<PFormula> constraints;
      // constraints that must be satisfied along with the requirements

    OrderingProblem(DPSet &dps, Ruleset &rules, Alphabet &F, int uprop);
      // creates a basic OrderingProblem, without any requirements;
      // arities are calculated from dps and rules
    OrderingProblem(Ruleset &rules, Alphabet &F);
      // creates a basic OrderingProblem, without any requirements;
      // arities are calculated from rules

    void set_filterable(string symbol);
      // marks the symbol (which should already be in arities!) as
      // filterable in all arguments; when this is not called, the
      // symbol will be treated as not filterable
    void set_unfiltered_properties(int value);
      // 0 if neither monotonicity or subterm steps are required;
      //   moreover, application is not unfilterable
      // 1 if we we do require subterm steps for unfilterable symbols
      // 2 if we require strong monotonicity for unfilterable symbols
    void require_atleastone(int start, int end);
      // adds a constraint start \/ ... \/ end (assuming start <= end)

  public:
    map<string, int> arities;
      // arities of all symbols occurring in any constraint,
      // dependency pair or rule we care about
      // TODO: make this not accessible as variable
    
    ~OrderingProblem();
      // frees the constraints and requirements; please do not use
      // the results from query_constraints() or orientables()
      // afterwards!

    bool changed_arities(map<string,int> original_arities);
      // returns whether the arities we calculated are different from
      // the arities originally used in the system

    PFormula is_filtered_away(string symbol, int index);
      // returns a formula indicating that the given argument of the
      // symbol (1-based) is filtered away; this formula becomes the
      // property of the caller (which should eventually free it)

    PFormula not_filtered_away(string symbol, int index);
      // returns a formula indicating that the given argument of the
      // symbol (1-based) is not filtered away; this formula becomes
      // the property of the caller (which should free it)

    int filtered_variable(string symbol, int index);
      // returns the index of a variable whose truth is equivalent to
      // is_filtered_away; returns -1 if the given index is not an
      // index of symbol

    int arity(string symbol);
    int arity(PConstant symbol);
      // returns the updated arity of the given symbol

    PType symbol_type(string symbol);
      // looks up the symbol's type in the internal alphabet, and
      // returns it; this is NOT a copy!

    string print_term(PTerm term, map<int,string> env,
                      map<int,string> freerename,
                      map<int,string> boundrename);
      // interface to wout.print_term, using the internal arities and
      // alphabet

    const vector<PFormula> query_constraints();
      // returns the constraints on the variables in orientables and
      // the filter variables

    const vector<OrderRequirement*> orientables();
      // returns the list of requirements we must orient

    bool unfilterable(string symbol);
      // returns true if the given symbol MAY NOT be filtered,
      // which is stronger than that is IS NOT filtered (a symbol
      // which is not filtered by us may be afterwards (partially)
      // filtered by a reduction pair processor)

    bool unfilterable_application();
      // returns whether application MAY be filtered (even though this
      // is never done by the OrderingProblem itself);

    bool unfilterable_strongly_monotonic();
      // returns true if it is required that unfilterable symbols
      // (including application) are strongly monotonic, so
      // F x > F' x whenever F > F, and F x > F x' whenever x > x'

    bool unfilterable_substeps_permitted();
      // returns true if it is required that unfilterable symbols
      // (include applications) have the subterm property, i.e.
      // f(...,s,...) >= s ~c1 ... ~cn

    void print();
      // prints the current ordering problem to the output module

    vector<int> strictly_oriented();
      // assuming that all logical variables have been assigned a
      // value, returns the union of all data associated with the
      // strictly oriented orientables

    virtual void justify_orientables();
      // prints an explanation of which rules were oriented; this
      // does not do anything (but can be overwritten to)
};

/**
 * A PlainOrdering is used to orient one or more rules, with a
 * monotonic ordering (so no arguments are filtered away).  All rules
 * must be ordered either strictly or non-strictly, and at least one
 * is ordered strictly.
 */
class PlainOrderingProblem : public OrderingProblem {
  public:
    PlainOrderingProblem(Ruleset &rules, Alphabet &F);
};

/**
 * A DPOrdering is used to orient rules and dependency pairs.  Due to
 * the various possible setups in the dependency pair framework, we
 * have to cater for three different situations:
 * - given a non-collapsing set of DPs, we create an ordering where
 *   any arguments can be filtered away, with usable and formative
 *   rules
 * - given a collapsing set of DPs, if use_tagging is true, we create
 *   an ordering where symbols below an abstraction in the right-hand
 *   sides are tagged, and tagged symbols may not be filtered;
 *   moreover, we add obligatory rules f-(x1,...,xn) -> f(x1,...,xn);
 *   f-(x1,...,xn) -> f#(x1,...,xn).  Formative rules are possible,
 *   but not usable ones.
 * - given a collapsing set of DPs, if use_tagging is false, we
 *   create an ordering where only the marked symbols f# may have any
 *   arguments filtered away, and where f(x1,...,xn) >= f#(x1,...,xn)
 *   is required; formative rules are possible, but not usable ones.
 */
class DPOrderingProblem : public OrderingProblem {
  private:
    int Ccounter;
      // a counter of freshly created constants
    map<int,int> meta_arities;
      // used for extending and de-extending
    bool metas_extended;
    string used_wrt;

    PConstant fresh_constant(PType type);
      // returns a constant which is thus far not in use in the
      // alphabet, assuming no other class creates symbols starting
      // with ~c.
    bool collapsing(DPSet &dps);
      // returns whether dps contains any collapsing rules
    int find_meta_arity(PTerm term, PVariable metavar);
      // returns the arity with which the given meta-variable occurs
      // in term; -1 if it doesn't occur!
    PTerm meta_extend(int index, PTerm term);
      // creates a copy of term with extended meta-variables, e.g.
      // X[a] * b becomes X[a,b]; the original arity of X is saved
      // under meta_arities[index]
    PTerm meta_alter(int index, PTerm term);
      // alters a term X[a1,...,an] * b1 *** bm back to X[a1,...,ai]
      // * a_{i+1} *** an * b1 *** bm if i = meta_arities[index], and
      // leaves it unchanged if meta_arities[index] is unset;
      // meta_arities[index] is now set to n, so you can use this to
      // swap between two situations
    PTerm make_functional(PConstant f);
      // creates f(x1,...,xn), where n = arities[f]
    PTerm substitute_variables(PTerm term, Substitution &gamma,
                               set<int> &binders_above_term);
      // destroys term, and returns a copy with all free variables
      // replaced by fresh constants of the same type
    void make_dp_requirements(DPSet &dps);
      // adds each of the dependency pairs, flattened to a base type,
      // into the requirements, each with its own variable indicating
      // whether it should be oriented weakly or strongly
    void make_rule_requirements(Ruleset &rules);
      // adds each of the rules into the requirements, oriented with
      // >=, each with its own variable indicating whether it actually
      // needs to be oriented
    void tag_below_abstraction(PTerm term, bool below,
                               map<string,string> &tagged);
      // for any function f(s1,...,sn) occurring below an abstraction
      // \x.q, if x occurs in some si, replaces f by f- and adds f ->
      // f- into the given mapping

    void handle_collapsing_case();
      // creates the problem for a non-collapsing DP problem
      // (assuming requirements have already been created)
    void handle_untagged_case(DPSet &dps);
      // creates the problem for a collapsing DP problem when we may
      // not use tagging; this will add reqs f >= f#
    void handle_tagged_case();
      // creates the problem for a collapsing DP problem when we may
      // use tagging; this will alter the reqs

    void get_all_symbols(PTerm term, set<string> &symbs);
      // saves all functions symbols occurring in term in symbs

    void execute_argument_function(PTerm l, PTerm r);
      // replaces l everywhere in reqs by r

    void update_metavar_subs(PTerm term, PVariable x,
                             vector< vector< pair<string,int> > > &subs);
      // updates subs to contain, for every occurrence of xj in term,
      // the functions it passes on the way

    bool constructor_match(PTerm from, PTerm to, set<string> &defs);
      // returns whether an instance of from can reduce to an instance
      // of to, paying particular attention to the constructor symbols
      // (all symbols not in defs); this admits false positives
    void add_usable_rules_requirements();
      // adds the requirements to apply usable rules with respect to
      // an argument filtering; currently, this is NOT PROVED CORRECT
      // and UNTESTED for the higher-order setting, although the code
      // should be correct when applied to a first-order system

  public:
    DPOrderingProblem(DPSet &dps, Ruleset &rules, Alphabet &F,
                      bool &use_tagging, bool use_usable,
                      bool use_formative);

    bool meta_deextend_stricts();
      // by default, we use extended meta-variables, so a right-hand
      // side may have the form F[r1,...,rn] even if F has arity below
      // n; this function undoes that, and gives meta-variables the
      // arity they also have in the left; returns whether anything
      // was changed
    bool meta_extend_stricts();
      // having done a meta_deextend_strics(), this function will
      // return the ordering problem back to its default form; returns
      // whether anything was changed

    bool simple_argument_functions();
      // if there are ordering requirements of the form
      // f(x1,...,xn) >= r, then normalise the rules using the
      // argument filtering rule f(x1,...,xn) -> r; returns true if
      // anything was done

    void justify_orientables();
      // prints an explanation of which rules was oriented, in
      // particular explaining the argument filtering that was used
      // and pointing out usable rules with respect to an argument
      // filtering (unless no rules were omitted, in which case we
      // don't print anything)
};

#endif

