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

#ifndef AFS_H
#define AFS_H

#include <map>
#include <vector>
#include "term.h"
#include "alphabet.h"
#include "environment.h"
#include "matchrule.h"

/**
 * A MonomorphicAFS represents the format used in the termination
 * competition.  Function symbols have arities, and rules are tuples
 * of two terms without restrictions other than that the free
 * variables of the right-hand side must also occur on the left.
 */

typedef map<string,int> ArList;
class Converter;

class MonomorphicAFS {
  friend class Converter;
  private:
    Alphabet Sigma;
    Environment gamma;
    vector<PTerm> lhs;
    vector<PTerm> rhs;
    ArList arities;
    bool normalised;
    bool normalising_took_effort;

    // used for the normalising functionality
    vector<PType> dangerous_types;
    map<string,string> ats;
    map<string,string> lams;
    
    void adjust_arities(PTerm term);
      // makes sure arities respects the arities in the given term

    PTerm respect_arity(PTerm term, bool ignoretop = false);
      // eta-expands a term so it respects arity

    void rename_variables();
      // make sure all variables have a unique name (respecting old
      // names whenever possible)

    vector<PVariable> find_free_head_variables(PTerm l, Varset &bound);
      // finds the free variables occurring at the head of an
      // application, excluding bound variables

    void mark_leading_abstraction_types(PTerm s); 
      // for all abstractions in s which occur at the head of an
      // application, add their types to dangerous_types

    bool query_dangerous(PType type);
      // returns whether type is in the dangerous_types vector

    bool rule_exists(PTerm left, PTerm right);
      // returns whether rule duplicates something in afsrules

    int maximal_left_arity(string f);
      // returns the greatest number k such that f occurs with k
      // arguments in some left-hand side (or the minimal arity of f
      // if f does not occur)

    map<string,int> instantiate_head_variables();
      // for all rules and head variables in the left-hand side,
      // add a rule instantiating that rule with any fitting
      // functional term; the return value maps each symbol f to
      // maximal_left_arity(f)

    vector<string> constants_with_type(PType output,
                                       map<string,int> &maxarities);
      // finds the constant symbols with a matching output type
      // arity is taken into account, so f : [a] -> a does not have
      // output type a -> a! We consider the output types from
      // arities[f] to maxarities[f], where maxarities[f] is here set
      // to maximal_left_arity(f) and stored for later use

    PConstant at(PType type);
      // find a good name (within the naming conventions) for a
      // symbol @_{type}; this immediately adds the constant to the
      // alphabet and adds a corresponding rule

    PConstant lam(PType type);
      // find a good name (within the naming conventions) for a
      // symbol ^_{type}; this immediately adds the constant to the
      // alphabet and adds a corresponding rule

    PTerm add_at(PTerm term, map<string,int> &maxar);
      // replace subterms s t of term with at(sigma) s t, if
      // s : sigma and sigma is a dangerous type

    PTerm add_lam(PTerm term);
      // replace subterms /\x.s : sigma with at(sigma) /\x.s, if
      // sigma is a dangerous type

    void setup(Alphabet &F, Environment &V,
               vector<PTerm> &left, vector<PTerm> &right,
               ArList &lst);
      // initialisation; F and V are copied, left and right just
      // taken over directly (so callers should not manipulate or
      // destroy thoese afterwards)

    bool check_normal();
      // checks whether the current system is in normalised form

  public:
    MonomorphicAFS(Alphabet &F, Environment &V,
                   vector<PTerm> &left, vector<PTerm> &right,
                   ArList &lst);
      // initialisation; F and V are copied, left and right just
      // taken over directly (so callers should not manipulate or
      // destroy thoese afterwards)
    
    MonomorphicAFS(Alphabet &F, Environment &V,
                   vector<PTerm> &left, vector<PTerm> &right);
      // initialisation with empty arity

    ~MonomorphicAFS();

    void recalculate_arity();
      // if the system is already normalised, assigns a maximal
      // arity to the functional symbols with respect to the rules
      // if the system is not normalised, does nothing

    void recalculate_arity_eta();
      // like recalculate_arity, but only makes the arities respect
      // the left-hand sides; right-hand sides are eta-expanded (note
      // that this DOES affect the termination question)

    void normalise_rules();
      // modifies the afsrules so:
      // * all rules have a form f l1 ... ln -> r
      // * both sides of all rules are beta-normal
      // * the left-hand sides of rules have no subterms x*s, with x
      //   a free variable

    void to_afsm(Alphabet &F, vector<MatchRule*> &rules);
      // transforms this AFS to an AFSM

    bool trivially_terminating(string &reason);
      // returns true if it is evident that this is terminating

    bool trivially_nonterminating(string &reason);
      // returns true if it is evident that this is terminating

    string to_string();
      // returns a text in the format that can also be read

    void pretty_print();
      // prints this AFS to the pretty printer
};

#endif

