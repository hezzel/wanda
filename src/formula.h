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

#ifndef FORMULE_H
#define FORMULE_H

#include <string>
#include <vector>
#include <map>

using namespace std;

/* We maintain a set of variables.  This is a class, to ensure proper
 * handling.
 * There should only ever be one set of variables, which is what all
 * the formulas interact with.
 */

enum Valuation { TRUE, FALSE, UNKNOWN };

class Vars {
  private:
    vector<Valuation> val;
    vector<string> descriptions;

  public:
    Vars();
    int query_size();
    Valuation query_value(unsigned int index);
    void force_value(unsigned int index, Valuation value);
    void add_vars(unsigned int number);
    void set_description(unsigned int index, string desc);
    string query_description(unsigned int index);
    bool has_description(unsigned int index);
    string to_string();
    int true_var();
      // a variable whose valuation is set to true
    int false_var();
      // a variable whose valuation is set to false
    void reset();
      // empties the set, preserving only true_var and false_var
    void reset_valuation();
      // sets the valuation of all variables (except true and false)
      // to UNKNOWN, but does keep them in the varset
};

extern Vars vars;

/* Formula is defined as a recursive data structure.  In its basic
 * form it's just a mathematical formula (and, or, not, top, bottom
 * or variable), but it has some extensions and functionality to make
 * a limited amount of SAT-solving possible.
 */

class Formula;
typedef Formula* PFormula;
class And;
class Atom;

class Formula {
  protected:
    bool changed;

    PFormula judge(PFormula original, PFormula current);
      // this formula always returns current; if original is not the same as
      // current it sets current->changed to true and deletes original
    virtual PFormula simplify_main();
      // simplify_main does the work for simplify, but doesn't self-delete or
      // set the changed variable if it returns something new
      // default return value: this (leave formula unchanged)

  public:
    Formula();

    virtual PFormula copy();    // should only be called in leaf classes!
    virtual string to_string(bool brackets = false); // same

    virtual bool query_top();
    virtual bool query_bottom();
    virtual bool query_conjunction();
    virtual bool query_disjunction();
    virtual bool query_variable();
    virtual bool query_antivariable();
    virtual bool query_negation();
    virtual bool query_special(string description);
    
    virtual PFormula negate();
      /* Returns the negation of this formula, without using Not if
       * possible (so (X /\ Y)->negate() is (!X \/ !Y), where !X and
       * !Y are the antivariables corresponding to X and Y.
       *
       * This returns a pointer to a new object; if you are replacing
       * a formula by its negation, do not forget to free the
       * original formula!
       */

    virtual PFormula simplify();
      /* simplify gives a formula a standard form:
       * - Not, Top and Bottom don't occur in it
       * - Ands and Ors are flattened, (anti-)variables in Ands and
       *   Ors placed at the start of the children list
       * - in a formula A /\ B or A \/ B, A does not occur in B if A
       *   is a variable
       * - possibly some extra standards for extended formulas!
       * Calling simplify on an already simplified formula has no
       * effect (so it never needs to be called more than once).
       *
       * Afterwards, the pointer to this object is useless; it might
       * have been deleted or (parts of it) reused in the return
       * value.
       *
       * Inheriting functions should overwrite simplify_main instead
       * (which doesn't need to bother self-deleting when a different
       * term is returned).
       */

    virtual PFormula conjunctive_form();
      /* This rewrites the current formula to a formula in conjunctive
       * normal form (that is, having the form A_1 /\ ... /\ A_n where
       * each A_i has the form B_1 \/ ... \/ B_m with each B_j a
       * distinct variable or antivariable).
       *
       * All formulas can be put into this form, but n and m may be 1
       * or even 0.  In such cases either a disjunction of
       * (anti-)variables or a single (anti-)variable will be
       * returned.
       *
       * There is no real need to overwrite this (except in And);
       * instead, overwrite the function below.
       */

    virtual PFormula conjunctive_form(int depth, And *head,
                                      bool negation_relevant,
                                      Atom *atom);
    /* This does the main work for the full conjunctive_form,
     * rewriting the current formula to a conjunctive normal form.
     * This should only be called from other Formula inherits,
     * outsiders should call conjunctive_form instead.  Inheriting
     * classes should respect the following requirements:
     * - this function should only ever be called with depth 1 or 2
     * - the formula can be assumed to be simplified before this is
     *   called, and should be simplified on returning
     * - the function should return either a disjunction, variable or
     *   antivariable, top or bottom; if depth = 2 the returnvalue
     *   should not be a disjunction
     * - "head" is the formula's top; anything added to it will be
     *   simplified and have conjunctive_form(1,top) called on it
     *   later
     * - if an inheriting class does not define his function, "this"
     *   is returned, which normally breaks the third requirement
     * - if the function returns something other than itself, it is
     *   responsible for doing the appropriate deleting
     * - negation_relevant is set if the value of the formula might
     *   become true by making this formula false; if it is not set
     *   (which is always the case if only the standard formula
     *   constructors are used) one might replace the formula by a
     *   new variable X and add a clause X => <this> to the top
     *   formula, but if it is set this should be <=>
     * - if depth = 1, negation_relevant = false (since in this case
     *   the formula is expected to be called as a direct child of
     *   the top And)
     * - if depth is anything other than 2, atom = NULL
     * - if depth is 2 and atom is not NULL, then the parent of the
     *   current formula has the form atom \/ this; this may or may
     *   not be useful
     */

    virtual PFormula substitute_variable(int index, PFormula with);
      /* Substitutes occurances of the given variable by the given
       * formula, and returns the resulting formula (usually a
       * pointer to the same object), freeing the old one if
       * necessary.  Also substitutes antivariables.
       */
    virtual int rename_variables(vector<int> org, vector<int> rep);
      /* Replaces variables Xi by Xj, and returns how many variables
       * it has replaced.  This does not manipulate memory.  Org
       * is assumed to be an ordered list.
       * Returns 0 by default.
       */

    void reset_changes();
    void set_changes();
    bool query_changes(bool reset = true);
};

/* Top and Bottom: the universal truth and falsehood.  These simple Formulas
 * are self-evident.
 */

class Top : public Formula {
  public:
    Top();

    PFormula copy();
    bool query_top();
    string to_string(bool brackets = false);
    PFormula negate();
};

class Bottom : public Formula {
  public:
    Bottom();

    PFormula copy();
    bool query_bottom();
    string to_string(bool brackets = false);
    PFormula negate();
};

/* Variables and AntiVariables.  An AntiVar should be read as the Not
 * of the corresponding Var, but it considered an atom, which Not is
 * not.  While they are complementary, they behave fundamentally the
 * same: each has an index, which maintains a value and possibly a
 * description in the vars class.
 *
 * The more complicated formula classes manipulate the atoms during
 * their simplification, forcing them to take a value temporarily and
 * so on.
 * Because of all this shared functionality, they share a parent class
 * Atom, which should never be used on itself.
 */

class Atom : public Formula {
  protected:
    unsigned int index;
    
    PFormula simplify_main();
  
  public:
    unsigned int query_index();
    virtual void force(bool maketrue = true);
    void unforce();
    int rename_variables(vector<int> org, vector<int> rep);
};

class Var : public Atom {
  public:
    Var(unsigned int ind);

    void force(bool maketrue = true);

    PFormula copy();
    bool query_variable();
    string to_string(bool brackets = false);
    PFormula negate();
    PFormula substitute_variable(int index, PFormula with);
};

// Antivar(n) is the same as Not(Var(n))
class AntiVar : public Atom {
  public:
    AntiVar(unsigned int ind);
    
    void force(bool maketrue = true);

    PFormula copy();
    bool query_antivariable();
    string to_string(bool brackets = false);
    PFormula negate();
    PFormula substitute_variable(int index, PFormula with);
};

/* And or Or: the basic elements which combine formulas.  While they
 * are complementary, they behave mostly the same, which is why most
 * of the interesting functionality is in a shared parent class AndOr
 * (which should not be inherited by anything else).
 */

class AndOr : public Formula {
  private:
    PFormula flatten_base(vector<Atom*> &atoms, vector<PFormula> &others);
      // helping function for simplify

  protected:
    vector<PFormula> children;

    virtual bool same_kind(PFormula form);

    PFormula simplify_main();
    
  public:
    ~AndOr();
    
    void add_child(PFormula child);
    int query_number_children();
    PFormula query_child(int index);
    PFormula replace_child(int index, PFormula newchild);
      // returns old child[index]
    
    PFormula copy();
    PFormula negate();
    PFormula substitute_variable(int index, PFormula with);
    int rename_variables(vector<int> org, vector<int> rep);
    PFormula top_simplify();
      // like simplify, but does not call simplify on the children
};

class And : public AndOr {
  protected:
    bool same_kind(PFormula form);

  public:
    // constructors; more than 3 children should be added with add_child
    // (or create And(a, And(b,c)), for example);
    // the constructors with 1 or 0 children are useful when children are
    // dynamically added
    And(PFormula left, PFormula middle, PFormula right);
    And(PFormula left, PFormula right);
    And(PFormula start);
    And();

    bool query_conjunction();
    string to_string(bool brackets = false);
    PFormula conjunctive_form();
    PFormula conjunctive_form(int depth, And *top,
                              bool negation_relevant, Atom *atom);
};

class Or : public AndOr {
  protected:
    bool same_kind(PFormula form);

  public:
    // constructors; more than 3 children should be added with add_child
    // (or create Or(Or(a,b), Or(c,d)), for example);
    // the constructors with 1 or 0 children are useful when children are
    // dynamically added
    Or(PFormula left, PFormula middle, PFormula right);
    Or(PFormula left, PFormula right);
    Or(PFormula start);
    Or();

    bool query_disjunction();
    string to_string(bool brackets = false);
    PFormula conjunctive_form(int depth, And *top,
                              bool negation_relevant, Atom *atom);
};

/* Finally the Not; this is never needed with basic formulas, as the negate
 * functionality doesn't use Not.  However, it may be useful for some
 * extensions of Formula.
 */

class Not : public Formula {
  protected:
    PFormula child;

    PFormula simplify_main();

  public:
    Not(PFormula _child);
    ~Not();

    bool query_negation();
    PFormula copy();
    string to_string(bool brackets = false);
    PFormula negate();
    PFormula query_child();
};

#endif

