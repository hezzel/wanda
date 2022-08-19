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
 * The NonTerminator attempts to find a proof that the system is
 * not terminating.  No advanced loop analysis, just some very basic
 * tricks.
 */

#include "matchrule.h"
#include "beta.h"
#include "alphabet.h"

class NonTerminator {
  private:
    Beta *beta;
    vector<MatchRule*> rules;
    Alphabet F;
    bool immediate_beta;

    bool obvious_loop(MatchRule *rule);
      // returns whether there is a reduction in 0 or more steps
      // from the right-hand side of the rule to a term containing
      // an instantiation of the left-hand side

    vector<string> clean_metavariables(PTerm &left, PTerm &right);
      // transform occurrences of /\x1...xn.Z[x1...xn] into Z
      // and returns a list of all positions where such
      // meta-variables occur in left

    vector<string> find_metavar(PTerm term, PVariable X);
      // returns all positions in term where X occurs

    string find_base_superterm(PTerm term, string pos);
      // returns the longest position p in term such that
      // term->subterm(p) does not have composed type, and pos
      // extends p

    void omega(PTerm l, PTerm r, string basepos, string lposX,
               string rposapp, vector<string> lposY, int offender);
      // constructs a counter example for termination

    bool reachable(PTerm term, string position, Substitution &gamma);
      // returns whether the substitution of term with gamma^n
      // contains the (instantiated version of the) subterm at the
      // given position; this may give false negatives if
      // immediate_beta is set to true

    PTerm copy_as_term(PTerm term);
      // makes a copy of a meta-term, but replaces all subterms
      // Z(s1,...,sn) by the application x_Z*s1***sn

  public:
    NonTerminator(Alphabet &_F, vector<MatchRule*> _rules, bool _immediate);
    ~NonTerminator();
  
    bool non_terminating();
      /* return value true: definitely non-terminating
       * return value false: possibly non-terminating
       */
    
    void possible_reductions(PTerm term, vector<Rule*> &rule,
                             vector<string> &pos);
      /* given a term, returns a vector of pairs (encoded with two
       * vectors) of a rule and a position where the rule can be
       * applied; the rule is either one of the rules supplied to the
       * constructor, or Beta
       */

    bool lambda_calculus(MatchRule *rule);
      /* returns true if the given rule can be used to simulate
       * the untyped lambda calculus
       */

    /*
    void meta_reductions(PTerm term, vector<Rule*> &rule,
                         vector<string> &pos);
    */
      /* given a meta-term, returns a vector of pairs (encoded with
       * two vectors) of a rule and a position where the rule can be
       * applied, no matter how meta-variables in term are
       * instantiated; this will not return positions inside a
       * meta-variable application, for example
       */
};

