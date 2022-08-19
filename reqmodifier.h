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

/**
 * =============================
 * THIS CODE IS NO LONGER IN USE
 * =============================
 *
 * It used to used in the dependency pair framework, to chane the
 * ordering requirements in the dependency pair processors.  However,
 * the OrderingProblem class (which has copied code from this one in
 * several isntances) has taken over this role.
 *
 * Since the requirement modifier provides certain pieces of
 * functionality which are not available elsewhere, the file is left,
 * to allow later lookups.  However, it does not occur in the Makefile
 * anymore.
 */



#ifndef REQMODIFIER_H
#define REQMODIFIER_H

#include "alphabet.h"
#include "dependencypair.h"
#include "matchrule.h"

class OrderingRequirement {
  public:
    PTerm left;
    PTerm right;
    string desc;
    vector<int> data;

    OrderingRequirement() {left = NULL; right = NULL;}
      // default constructor, should in principle not be used

    OrderingRequirement(PTerm l, PTerm r, string d, int extra = -1)
        :left(l), right(r), desc(d) {
      if (extra != -1) data.push_back(extra);
    }
    
    ~OrderingRequirement() {
      if (left != NULL) delete left;
      if (right != NULL) delete right;
    }

    string to_string() {
      Environment gamma;
      string ret = left->to_string(gamma) + " ";
      ret += desc + " " + right->to_string(gamma);
      return ret;
    }
};

typedef vector<OrderingRequirement*> Reqlist;

/**
 * The RequirementModifier is used to create the ordering
 * requirements for a dependency pair problem, and to modify them
 * with standard argument functions, type-normalising etc.
 * Note that all >? requirements remember the index of the
 * dependency pair they originated from, so if l > p is proved, the
 * corresponding dependency pair can be removed.
 *
 * Many of the functions return a string. This is a comment
 * describing what was done.
 */

typedef vector<MatchRule*> Ruleset;
typedef map<string,int> ArList;

class RequirementModifier {
  private:
    int Ccounter;
      // used for creating fresh constants

    PConstant fresh_constant(PType type);
      // creates a constant ~c<num> for a thus-far unused number
      // (this uses Ccounter to keep track of numbers currently
      // in use)
    PTerm substitute_variables(PTerm term, Substitution &gamma,
                               set<int> &binders_above_term);
      // destroys term, and returns a copy with all free variables
      // replaced by fresh constants of the same type
    PType rename_and_collapse(PType type, string a, PType renaming);
      // collapses type, but renames all occurrences of a data type a
      // by renaming before doing so (collapsing means to make a copy
      // with all data types replaced by "o")

    PTerm type_collapse(PTerm term, string a, PType renaming);
      // makes a "copy" with all data types replaced by "o"
      // however, types a are replaced by renaming before collapsing

    PTerm function_hider(PTerm term, bool top = true);
      // if term has a base-type subterm which has a functional
      // meta-variable as an argument, returns the first such term
      // otherwise returns NULL
      // if top = true, the returned value can only be a strict
      // subterm

    bool type_contains(PType big, DataType *small);
      // returns whether the big type uses the small type

    bool symbol_occurs(string symbol, PTerm term);
      // returns whether the given symbol occurs anywhere in the
      // given term
    
    bool always_ge(PTerm l, PTerm r, Renaming &ren, bool lhs);
      // returns whether l >= r using either HORPO or polynomial
      // orderings
      // if lhs is set to true, then the function may choose a
      // renaming for the meta-variables and free variables in r to
      // make this work; if lhs is set to false, the renaming is used
      // on terms in l, and any variable in l that is not renamed
      // should be considered infinitely large
      // this does not use any complex machinery like SAT-solving,
      // and may well give false negatives

    Varset unfiltered_metavars(PTerm r, ArList &arities);
      // returns meta-variables in a term which cannot get filtered
      // away, on account of not being below an untagged symbol
    
    void execute_argument_function(Reqlist &reqs, PTerm l, PTerm r);
      // replaces l everywhere by r

    void get_all_symbols(PTerm term, set<string> &symbs);
      // saves all functions symbols occurring in term in symbs

    void replace_unfiltered_constants(PTerm term, ArList &arities,
                                      bool below_abstraction);
      // replaces symbols f by f- if they occur inside an abstraction
      // and a bound variable occurs inside their arguments

    PTerm eta_everything_in(PTerm term, ArList &arities);

  public:
    RequirementModifier();
      // initialises the counter

    Reqlist create_basic_requirements(DPSet &D, Ruleset &A, bool extend);
      // turns the given two sets into a list of ordering
      // requirements (the list will contain copies),
      // with extended meta-variables if extend is true

    string make_base(Reqlist &reqs);
      // makes sure the collapsed versions of the >? requirements
      // have base type, by adding extra variables and/or small
      // symbols if necessary

    void lower(Reqlist &reqs);
      // replace symbols f# by the corresponding f
    
    string collapse_types(Reqlist &reqs, Alphabet &F, Alphabet &newF,
                          ArList &ars);
      // type-collapses the requirements (all data types become "o"),
      // and creates a new alphabet with the type-collapsed symbols of F
      // (F itself is left unmodified) -- newF contains only those
      // symbols which occur in ars (these may be elements of F, or
      // marked / tagged versions thereof)

    string remove_consequences(Reqlist &reqs);
      // removes requirements from Reqlist which are strictly implied
      // by another requirement

    bool simple_argument_functions(Reqlist &reqs, Alphabet &F,
                                     ArList &arities);
      // creates argument functions for symbols f which occur only
      // in requirements of the form f(x1,...,xn) -> r

    void tag_right_sides(Reqlist &reqs, ArList &arities);
      // replaces symbols f below an abstraction in the right-hand
      // sides of reqs by f-; the new symbols are not added to any
      // alphabet, nor to the given arity list!

    void tag_all_symbols(Reqlist &reqs);
      // replaces all function symbols f by f-

    void add_untagging_reqs(Reqlist &reqs, Alphabet &F, ArList &arities);
      // for all symbols f- occurring in the reqs, adds a requirement
      // f-(x1,...,xn) >= f(x1,...,xn), and possibly also
      // f(x1,...,xn) >= f#(x1,...,xn); also adds the symbols f- into
      // alphabet and arity list

    void add_upping_reqs(Reqlist &reqs, Alphabet &F);
      // for all symbols f# occurring in the reqs, adds a requirement
      // f(x1,...,xn) >= f#(x1,...,xn)
    
    void remove_marks(Reqlist &reqs);
      // changes symbols f# at the top back to f

    void eta_expand(Reqlist &reqs);
      // eta-expands the given requirements!

    void eta_everything(Reqlist &reqs, ArList &arities);
      // eta-expands even those parts which do not need to be eta-expanded
      // (such as meta-variables and abstractions)

    ArList get_arities_in(Reqlist &reqs, ArList &arities);
      // returns the arities of all symbols occurring in reqs; these
      // are either symbols in arities, or symbols in arities with -
      // or # appended
};

#endif

