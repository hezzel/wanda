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

#ifndef RULES_MANIPULATOR_H
#define RULES_MANIPULATOR_H

#include "alphabet.h"
#include "formula.h"
#include "matchrule.h"
#include "dependencypair.h"

typedef map<string,int> ArList;

/**
 * Makes transformations and restrictions of rules for the
 * dependency pair framework.
 */

typedef vector<MatchRule*> Ruleset;
typedef vector<PType> TypeList;
typedef map<string,TypeList> SymbolList;

class RulesManipulator {
  private:
    bool query_linear(PTerm term, Varset &encountered);
    bool query_algebraic(PTerm term);
    bool query_arguments_below(PTerm term, int num);
    bool extended(PTerm term, Varset &bound);
    bool matchable(PTerm s, PTerm t, bool mono, bool append);
      // returns true if there may be a term q which instantiates
      // both s and t; mono indicates that the system is monomorphic
      // and if append is set, extra meta-variables might be
      // appended to t to make the match work
      // might well return false positives

    void Symb(PTerm s, vector<string> &f, vector<PType> &type);
    bool add_symbol(SymbolList &list, string symbol, PType type);
    bool add_symbols(SymbolList &list, vector<string> &symbols,
                     vector<PType> &types);
    bool add_sub_symbols(SymbolList &list, PTerm l);
    bool symbol_occurs(SymbolList &list, string f, PType type);
    SymbolList formative_symbols(DPSet &DP, Ruleset &rules);
    Ruleset formative_rules_for(SymbolList &list, Ruleset &rules);
    void free_list(SymbolList &list);
    void reachable_from(string symb, set<string> &found,
                        Ruleset &rules);
    int sort_comparison_variable(string x, string y, map<string,int>
                                 &varmap, set<string> &allsorts);
    PFormula positions_okay(string sort, PType type, bool positive,
                   map<string,int> &varmap, set<string> &allsorts);
    PFormula position_accessible(PConstant f, int pos,
                    Alphabet &Sigma, ArList &arities,
                    map<string,int> &varmap, set<string> &allsorts);
    PFormula accessible(int metavar, PTerm term,
                        ArList &arities, map<string,int> &varmap,
                        set<string> &allsorts);
    bool plain_function_passing_new(Ruleset &rules, ArList &arities,
                                    map<string,int> &sortordering);
      // checks whether the given set of rules is plain function
      // passing, by attempting to find a suitable ordering for the
      // function symbols
    bool plain_function_passing_old(Ruleset &rules, ArList &arities,
                                    map<string,int> &sortordering);
      // checks whether the given set of rules is plain function
      // passing with a sort ordering where all function symbols are
      // equal; this follows an older definition than the one using
      // accessibility
    bool check_inequalities(And *formula, map<string,int> &varmap,
                            set<string> &allsorts, map<string,int> &sortord);
    set<string> identify_possible_applications(Ruleset &rules);
      // finds all constants A such that there is a rule of the form
      // A(f,x1,...,xn) -> f x1 ... xn, with f and all xi meta-variables
    vector<PTerm> find_symbol_subterms(string f, PTerm s);
      // finds all maximally applied subterms of s whose head symbol
      // is the given f
    PVariable query_fake_application(string ap, PTerm t,
                                     vector<PTerm> &args);
      // tests whether t has the form ap Z s1 ... sk, where Z is a
      // meta-variable; if so, returns Z and sets args to s1,...,sk;
      // if not, returns NULL (args may or may not be altered)
    bool find_encoded_metavars(string f, PTerm lhs, map<int,int> &arities);
      // verifies that all occurrences of f in lhs have the form
      // f Z x1,...,xk, where x1,...,xk are all distinct bound variable
    bool check_encoded_metavar_use(string f, PTerm rhs, map<int,int> &arities);
      // verifies that all occurrences of f in rhs have the form
      // f Z s1 ... sn where n >= arities[Z]
    bool used_as_application(string f, Ruleset &rules);
      // checks that in all left-hand sides where f is not the right, f
      // only occurs in the form f Z x1 ... xk, where x1,...,xk are all
      // distinct bound variables, and in the right-hand side all
      // occurrences of Z then have the form f Z s1 ... sn with n >= k
    PTerm replace_encoded_applications(string ap, PTerm s,
                                       map<int,int> &arities);
      // for any occurrences of an encoded application ap Z s1 ... sk in
      // s, where either k = arities[Z] or arities[Z] is unset (in the
      // latter case, arities[Z] is set to k), the occurrence is replaced
      // by Z[s1,...,sk]
    void replace_encoded_applications(string f, Ruleset &rules);
      // under the assumption that f is an encoded application, this
      // replaces occurrences of f Z s1,...,sn by Z[s1,...,sn]
    bool remove_redundant_rules(Ruleset &rules);
      // if some rules are instances of another rule, it removes the instance
      // true is returned if something was removed, false otherwise
    
  public:
    PTerm eta_expand(PTerm term, bool skiptop = false);
    bool left_linear(Ruleset &rules);
    bool fully_extended(Ruleset &rules);
    bool argument_free(Ruleset &rules);
    bool algebraic(Ruleset &rules);
    bool monomorphic(PTerm term);
    bool monomorphic(Ruleset &rules);
    PVariable extended_variable(PTerm term);
      // returns Z if term = /\x1...xn.Z(x1,...,xn), otherwise
      // null
    bool has_critical_pairs(Ruleset &rules);
      // assumes left-linear fully extended rules;
      // even then, may well give false positives
    bool eta_long(Ruleset &rules, bool only_functional = false);
      // if the latter is given, the actual question is:
      // "do these rules have base output types"
    bool meta_single(Ruleset &rules);
      // returns whether any occurrence of a meta-variable in a rule
      // has at most arity 1
    bool base_outputs(Ruleset &rules);
      // returns whether all rules have base output types
    bool fully_first_order(Ruleset &rules);
      // returns whether the given rules are entirely first-order
    bool plain_function_passing(Ruleset &rules, ArList &arities,
                                map<string,int> &sortordering);
      // returns whether functional variables in the left occur only
      // at accessible arguments within argument positions
      // if true is returned, sortordering is updated with numeric
      // values for the sorts in an ordering that gives accessibility
      // note: sortordering is left empty if no sortordering at all is
      // needed, so if the older style of static dependency pairs may
      // be used
    bool strong_plain_function_passing(Ruleset &rules, ArList &arities,
                                       map<string,int> &sortordering);
      // returns whether functional variables in the left occur only
      // at accessible points of argument positions, and the right
      // does not contain abstractions /\x.C[f(...,D[x],...)] with f
      // a defined symbol that might reach the head of the left;
      // if true is returned, sortordering is updated with numeric
      // values for the sorts in an ordering that gives accessibility
      // note: sortordering is left empty if no sortordering at all is
      // needed, so if the older style of static dependency pairs may
      // be used
    ArList get_arities(Alphabet &F, Ruleset &rules);
      // finds arities as they are used in the rules

    Ruleset beta_saturate(Ruleset rules);
    Ruleset eta_expand(Ruleset rules);
      // note: this will destroy the original rules!
    Ruleset formative_rules(DPSet DP, Ruleset rules);
      // note: the returned rules are (modified) copies of the given
      // ones!
    Ruleset copy_rules(Ruleset &rules);
      // make a copy of the given set of rules
    Ruleset usable_rules(DPSet DP, Ruleset rules);
      // the returned rules are the same pointers as the given ones,
      // just perhaps less of them
    bool simplify_applications(Alphabet &Sigma, Ruleset &rules);
      // notes when a given symbol is essentially an encoding for
      // application, and breaks it out if so; doing this may lose
      // termination of the program, but not of its first-order part
};

#endif

