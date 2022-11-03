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

#ifndef TYPER_H
#define TYPER_H

/**
 * The Typer class is part of the input module of WANDA, used for
 * reading ambiguous formalisms (such as untyped but typable
 * applicative systems) and for transforming formalisms.
 */

#include "term.h"
#include "alphabet.h"
#include "environment.h"

/**
 * Partially typed term; a "term" where some subterms have a
 * minimal type given.  By assumption, if s has a type, then so do
 * all of its subterms.
 */
typedef struct PartTypedTerm* PPartTypedTerm;
class PartTypedTerm {
  public:
    string name;
    PType type;       // is NULL if the type has not been given or
                      // calculated
    int idata;        // variables (and meta-variables) have an index
    string sdata;     // constants have a name
    vector<PPartTypedTerm> children;

    PartTypedTerm() :type(NULL) {}
    ~PartTypedTerm();

    string to_string(bool brackets = false);
};

class Typer {
  private:
    string last_warning;

    /**
     * The internal mechanics of the Typer uses the concept of a
     * non-recursive type substitution (all occurrences of theta in
     * any of these functions are assumed to be non-recursive).
     * During typing and unifying of types, the type under
     * consideration is usually sigma theta^N, where theta^N is
     * obtained by repeatedly composing theta with itself until this
     * no longer makes a difference
     */
    
    PType substitute_recursively(PType type, TypeSubstitution &theta);
      // returns type with theta^N applied on

    bool contains(PType sigma, TypeVariable *a,
                  TypeSubstitution &theta);
      // returns whether sigma theta^N contains the type variable a

    PType unify_help(PType sigma, PType tau, TypeSubstitution &theta);
      // returns the minimal unifying type of sigma theta^N and
      // tau theta^N; this type may still contain type variables
      // which should be substituted with theta^N to really obtain
      // this unification

    string unify_failure(PType sigma, PType tau,
                         TypeSubstitution &theta);
      // returns the string "could not unify <sigma theta^N> with
      // <tau theta^N>."

    PType fresh_copy(PType sigma);
      // returns a copy of sigma with all type variables replaced by
      // fresh type variables
    
    bool check_consistency(PPartTypedTerm term, Alphabet &Sigma,
                           Environment &Gamma);
      // returns false if a constant has incorrect typing, or any
      // variable is typed different on different occasions (even if
      // these types are unifiable);
      // also assigns types to all constants, and to variables for
      // which a typed variable has been found earlier or is in Gamma

    PType type_help(PPartTypedTerm todo, Environment &Gamma,
                    TypeSubstitution &theta);
    PType type_variable(PPartTypedTerm var, Environment &Gamma);
      // does type_help when todo is a variable
    PType type_abstraction(PPartTypedTerm todo, Environment &Gamma,
                           TypeSubstitution &theta);
      // does type_help when todo is an abstraction
    PType type_application(PPartTypedTerm todo, Environment &Gamma,
                           TypeSubstitution &theta);
      // does type_help when todo is an application
    PType type_meta(PPartTypedTerm todo, Environment &Gamma,
                    TypeSubstitution &theta);
      // does type_help when todo is a meta-variable application

    PTerm parse_fully_typed_term(PPartTypedTerm term,
                                 TypeSubstitution &theta);
      // turns term into a proper term, assuming all types have been
      // calculated and are internally correct taking theta into
      // account

  public:
    PType unify(PType type1, PType type2);
      // returns the minimal type which unifies type1 and type2, or
      // NULL if no such type exists
    
    PTerm type_term(PPartTypedTerm term, Alphabet &Sigma,
                    PType expected_type = NULL);
      // returns the minimally typed version of term if it exists,
      // otherwise returns NULL and sets last_warning
    
    string query_warning();
      // if type_term or type_rule does not return a result,
      // query_warning will return a reason why typing failed
};

#endif

