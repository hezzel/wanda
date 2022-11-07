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

#ifndef TERM_H
#define TERM_H

#include "type.h"
#include "varset.h"
#include <map>
#include <vector>

/**
 * These classes implement terms (or, in the HODRS formalism,
 * actually meta-terms).  PTerms as defined here are polymorphic,
 * applicative expressions.  PTerms are only referenced by pointers.
 * Constants are uniquely identified by their name and  type,
 * variables by an index and applications/abstractions by their
 * subterms (more information above the respective definition).
 *
 * It is usually not a good idea to reuse the same term pointer in a
 * second place; instead, a copy() function is provided which returns
 * a copy of the same term; only the pointers to variables and
 * constants are unchanged.
 *
 * Each subterm in a term carries its own type; in an abstraction
 * /\x.c, the pointer to the type of c will not be used in the type
 * of the whole abstraction.
 */

class Term;
class Constant;
class Variable;

typedef Term* PTerm;
typedef Variable* PVariable;
typedef Constant* PConstant;

typedef map<int,int> Renaming;

class Environment;
class Substitution;

class Term {
  protected:
    PType type;

    string pretty_name(int index, bool bound);
      /* finds a good name for a (meta-)variable (starting with %);
       * there is no randomness involved, so pretty_name(i,b) always
       * returns the same for fixed i and b, and pretty_name(i,b) is
       * different from pretty_name(j,g) if i != j; meta-variables
       * and free variables are capitalised while other variables are
       * not
       */

  public:
    Term();
    Term(PType t);
    ~Term();
      // the destructor also deletes subterms and the type, but
      // checks whether subterms are NULL before deleting (so to
      // delete only the top term, replace subterms by NULL first)

    PType query_type();
      /* returns the type of this term, do not delete it */

    virtual bool query_constant();
    virtual bool query_variable();
    virtual bool query_abstraction();
    virtual bool query_application();
    virtual bool query_meta();
    virtual bool query_special(string description);
      /* query_special always returns false for the terms defined
       * here; it is used to convenience different subclasses
       */

    virtual void apply_type_substitution(TypeSubstitution &s);
      /* this will apply the given type substitution on the current
       * term
       */
    virtual PTerm apply_substitution(Substitution &s);
      /* this will modify the current term, and returns a pointer to
       * the result; the old pointer to this term should be forgotten,
       * not deleted
       * (it will only point to different memory if this term is a
       * substituted variable, in which case the variable is freed)
       */
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma);
      /* if it is possible to extend theta and gamma in such a way
       * that this theta gamma = term, does so and returns true,
       * otherwise returns false (even when false is returned, theta
       * and gamma may have some elements added)
       */
    virtual PTerm subterm(string position);
      /* should be a string of 1s and 2s;
       * this returns an actual subterm of this term, so if anything
       * dangerous is going to happen with it, the caller should make
       * a copy() of it!
       * if there is no subterm on the given position, NULL is
       * returned
       * See also: MetaApplication::subterm
       */
    virtual PTerm replace_subterm(PTerm subterm, string position);
      /* replaces the subterm on the given position by subterm, and
       * returns a pointer to the old subterm (which is not deleted
       * or otherwise modified); returns NULL if no such position
       * exists, or the position is empty
       */
    virtual int number_children();
      /* returns the number of subterms; this is 1 for an abstraction
       * and 2 for an application (but possibly more for a
       * meta-application)
       */
    virtual PTerm get_child(int index);
      /* returns the child with the given index, or NULL if no such
       * child exists; note: this is 0-based, unlike subterm */

    virtual PTerm replace_child(int index, PTerm newchild);
      /* replaces the child at the given position by the new term;
       * returns NULL if no such position exists, otherwise the
       * original child
       */

    virtual Varset free_var(bool metavars = false);
      /* returns a set with either all the variables which occur
       * freely, or all the metavariables in the term
       */
    virtual Varset free_typevar();
      /* returns a vector of all type variables occurring in this
       * term, in order of occurrence (but each typevar only once)
       */
    virtual PTerm query_head();
      /* returns the head term (where the head term of an application
       * s0 s1 ... sn is s0 as long as s0 is not an application); if
       * the term is beta-normalised, this is either a variable or a
       * constant, otherwise it might also be an abstraction
       *
       * note: this returns a direct subterm, not a copy; do not just
       * delete
       */
    virtual vector<string> query_positions(string startwith = "");
      /* returns all strings which encode a valid position in this
       * term; startwith is added in front of all results
       */
    virtual vector<PTerm> split();
      /* if this term is an application, returns the parts of the
       * application; if not, returns a vector of one element (this)
       */
    virtual PType lookup_type(int v);
      /* returns the type in this term of a variable or meta-variable
       * with index v, or NULL if such a variable cannot be found
       */
    virtual bool query_pattern();
      /* returns whether this meta-term is a pattern, that is, the
       * arguments to meta-applications are different variables
       */
    virtual void adjust_arities(map<string,int> &arities);
      /* updates the given arities list with the arities as they
       * appear in the term; if the arity for a given symbol is
       * already in the list, it can be adjust downwards, but not
       * upwards
       */

    PTerm copy();
      /* returns a deep copy of this term */
    bool equals(PTerm other);
      /* returns whether this term is alpha-equal to the given other;
       * this should be the case exactly of to_string() ==
       * other->to_string(), but the equals call is more efficient
       * (as it can often cut off and return 'no' immediately)
       */
    string to_string(bool annotated = false, bool addtype = false);
      /* returns a string representing this term, assigning names of
       * choice to the free variables; if "annotated" is set all
       * function symbols and variables are annotated with their
       * type, and if "addtype" is set the term is printed in the
       * form "term : type".
       */
    string to_string(Environment &env, bool annotated = false,
                     bool addtype = false);
      /* returns a string representing this term, using the names in
       * the given environment for the free variables; for any
       * variables encountered which are not in env a new name will
       * be chosen, and saved in env
       */
    string to_string(Environment &env, TypeNaming &tenv);
      /* like the last one, but also saves the names of the type
       * annotations; annotated is assumed true and addtype false
       */

    /**
     * The following functions are only intended to be used
     * internally by the inherited classes of PTerm; the standard
     * versions above are likely to be more convenient for external
     * calling.
     */
    virtual PTerm copy_recursive(Renaming &boundrename);
      /* returns a deep copy of this term, where the variables in
       * boundreplace are renamed to their image
       */
    virtual bool equals_recursive(PTerm other, Renaming &boundrename);
      /* returns whether this term is equal to other, assuming that
       * variables in boundrename are replaced by their image (in
       * this term, not in other)
       */
    virtual string to_string_recursive(Environment &env,
                                       TypeNaming &tenv,
                                       int &bindersencountered,
                                       int &unknownfree,
                                       bool brackets,
                                       bool annotated);
      /* does the work for to_string; "env" can be assumed to assign
       * names to the free variables in the term - for anything not
       * in env a unique name will be chosen depending and the value
       * of unknownfree (and then unknownfree is upped to enforce
       * uniqueness), while bindersencountered keeps track of the
       * total number of abstractions occuring in the term (so we
       * will use different variables for each);
       * all inheriting classes must mask this function
       */
    virtual bool instantiate(PTerm term, TypeSubstitution &theta,
                             Substitution &gamma, Renaming &bound);
      /* does the work for instantiate; "bound" contains the variabls
       * bound somewhere higher up in the term (since a variable or
       * metavariable does not match with a bound variable).
       */
};

/**
 * Constants are global in the program: a constant is identified by
 * its name, which is a string not containing any special characters
 * (see also the SPECIAL file)
 *
 * Constants can be instantiated with different types.  We consider
 * two types only equal as terms if they have the same type.
 */

class Constant : public Term {
  private:
    string name;
    int possargs;   // for arity
    string typestring;
      // to_string() of its type; since typing is important for
      // comparing two constants, it's useful to keep this cached

  public:
    Constant(string _name, PType _type);
    Constant(Constant *con);
    
    bool query_constant();
    int query_max_arity();
    string query_name();
    void rename(string newname);
    void adjust_arities(map<string,int> &arities);

    void apply_type_substitution(TypeSubstitution &s);
    PTerm copy_recursive(Renaming &boundrename);
    bool equals_recursive(PTerm other, Renaming &boundrename);
    string to_string_recursive(Environment &env, TypeNaming &tenv,
                               int &bindersencountered,
                               int &unknownfree,
                               bool brackets, bool annotated);
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma, Renaming &bound);
};

const int FRESHVAR = -1;

/**
 * Variables have a special function in the program, because they can
 * be substituted for and renamed as necessary to avoid illegal
 * capture.  They are uniquely identified by an index, and any new
 * variable gets an index that was not used before.  The index of a
 * Variable is hidden otherwise.
 */
class Variable : public Term {
  friend class Abstraction;
  private:
    long index;
    string typestring;    // cached for printing purposes
  
  public:
    Variable(PType _type, long _index = FRESHVAR);
    Variable(Variable *var);
    
    bool query_variable();

    Varset free_var(bool metavars = false);
    PType lookup_type(int v);

    PTerm apply_substitution(Substitution &s);
    void apply_type_substitution(TypeSubstitution &s);
    PTerm copy_recursive(Renaming &boundrename);
    bool equals_recursive(PTerm other, Renaming &boundrename);
    string to_string_recursive(Environment &env, TypeNaming &tenv,
                               int &bindersencountered,
                               int &unknownfree,
                               bool brackets, bool annotated);
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma, Renaming &bound);
    
    long query_index();
};

class Application : public Term {
  private:
    PTerm left;
    PTerm right;

  public:
    Application(PTerm _left, PTerm _right);
    ~Application();
    
    bool query_application();

    PTerm apply_substitution(Substitution &s);
    void apply_type_substitution(TypeSubstitution &s);
    PTerm subterm(string position);
    PTerm replace_subterm(PTerm subterm, string position);
    int number_children();
    PTerm get_child(int index);
    PTerm replace_child(int index, PTerm newchild);
    Varset free_var(bool metavars = false);
    Varset free_typevar();
    PTerm query_head();
    vector<string> query_positions(string startwith = "");
    vector<PTerm> split();
    PType lookup_type(int v);
    bool query_pattern();
    void adjust_arities(map<string,int> &arities);

    PTerm copy_recursive(Renaming &boundrename);
    bool equals_recursive(PTerm other, Renaming &boundrename);
    string to_string_recursive(Environment &env, TypeNaming &tenv,
                               int &bindersencountered,
                               int &unknownfree,
                               bool brackets, bool annotated);
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma, Renaming &bound);
};

class Abstraction : public Term {
  private:
    PVariable var;
    PTerm term;
  
  public:
    Abstraction(PVariable _var, PTerm _term);
    ~Abstraction();
    
    bool query_abstraction();
    PVariable query_abstraction_variable();

    PTerm apply_substitution(Substitution &s);
    void apply_type_substitution(TypeSubstitution &s);
    PTerm subterm(string position);
    PTerm replace_subterm(PTerm subterm, string position);
    int number_children();
    PTerm get_child(int index);
    PTerm replace_child(int index, PTerm newchild);
      // if index = -1, this returns the variable
    Varset free_var(bool metavars = false);
    Varset free_typevar();
    vector<string> query_positions(string startwith = "");
    PType lookup_type(int v);
    bool query_pattern();

    PTerm copy_recursive(Renaming &boundrename);
    bool equals_recursive(PTerm other, Renaming &boundrename);
    string to_string_recursive(Environment &env, TypeNaming &tenv,
                               int &bindersencountered,
                               int &unknownfree,
                               bool brackets, bool annotated);
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma, Renaming &bound);
};

class MetaApplication : public Term {
  private:
    PVariable metavar;
    vector<PTerm> children;
      // a metavariable application should not have more than
      // 74 children; if it does, the latter ones will not be
      // accessible with subterm steps
    string typestring;    // cached for printing purposes
    
    void determine_type();
  
  public:
    MetaApplication(PVariable _metavar, vector<PTerm> &_children);
    MetaApplication(PVariable _metavar);
    ~MetaApplication();
    
    bool query_meta();
    PVariable get_metavar();

    PTerm apply_substitution(Substitution &s);
    void apply_type_substitution(TypeSubstitution &s);
    PTerm subterm(string position);
      /* For a MetaApplication Z[s0...sn], subterm 2 is position "2",
       * subterm 10 is position ":" and so on, following the ascii
       * table.
       */
    PTerm replace_subterm(PTerm subterm, string position);
    int number_children();
    PTerm get_child(int index);
    PTerm replace_child(int index, PTerm newchild);
    Varset free_var(bool metavars = false);
    Varset free_typevar();
    vector<string> query_positions(string startwith = "");
    PType lookup_type(int v);
    bool query_pattern();

    PTerm copy_recursive(Renaming &boundrename);
    bool equals_recursive(PTerm other, Renaming &boundrename);
    string to_string_recursive(Environment &env, TypeNaming &tenv,
                               int &bindersencountered,
                               int &unknownfree,
                               bool brackets, bool annotated);
    bool instantiate(PTerm term, TypeSubstitution &theta,
                     Substitution &gamma, Renaming &bound);
};

#endif

