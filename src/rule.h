/**************************************************************************
   Copyright 2012 Cynthia Kop

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

#ifndef RULE_H
#define RULE_H

/**
 * This class implements the generic functionality for rules,
 * although no specific form of rewriting is assumed.  Any
 * implementation of a rewriting format should ideally inherit from
 * this class.
 *
 * When adding your own rules (or rule formats), both apply_top and
 * applicable_top need to be overwritten.  Nothing else is strictly
 * necessary, although normalise implements a simple depth-most
 * reduction strategy, which you might wish to replace if this is not
 * normalising.
 */

#include "term.h"

class Rule {
  private:
    string name;

  protected:
    virtual PTerm apply_top(PTerm term);
      /* overwrite this method to apply your rule to the given
       * term (topmost)
       * if the rule is not applicable, just return term
       * if the rule applies, return the changed term; free all
       * memory in term that is not used in the returned term
       */

    virtual bool applicable_top(PTerm term);
      /* overwrite this method to return whether your rule is
       * applicable on the given term (topmost)
       */

  public:
    Rule() : name("generic rule") {}
    Rule(string _name) :name(_name) {}

    string query_name() {return name;}
    void set_name(string _name) {name = _name;}

    virtual PTerm apply(PTerm term, string position = "");
      /* this function returns a pointer to the result of having
       * this rule applied on term at the given position
       * if the rule cannot be applied there, term is simply returned
       * otherwise, the result is a modification of term; term is
       * destroyed for the rest
       *
       * child classes do not have to overwrite this function
       */
     
    virtual bool applicable(PTerm term, string position = "");
      /* returns whether this rule is applicable on the given
       * position in the given term
       *
       * child classes do not have to overwrite this function
       */

    virtual PTerm normalise(PTerm term);
      /* this function returns a pointer to the result of having the
       * given term normalised using this rule
       * this function should only be used on terminating rules with
       * a clear normalisation strategy; if no normal form can be
       * found, the normalise function will just hang indefinitely or
       * crash
       *
       * child classes should only overwrite this function if the
       * rule has a normalisation strategy other than depth-most
       * reduction
       */

    virtual bool normal(PTerm term);
      /* returns whether the given term is normal with respect to
       * this rule
       *
       * child classes do not have to overwrite this function
       */

    virtual string to_string();
      /* this returns some representation of the current rule
       * if not overwritten, this will just return name
       */
};

#endif
