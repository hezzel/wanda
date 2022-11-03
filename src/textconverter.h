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

#ifndef TEXTCONVERTER_H
#define TEXTCONVERTER_H

/**
 * This class provides functionality to read a string as outputted
 * by PTerm::to_string() into a PTerm or PType.  It may either be
 * inherited into a "program" class, or just be a variable in one
 * (as you will normally not need more than the two global methods).
 *
 * As input formats may vary from application to application, many of
 * the interpretation functions are virtual and split up in smaller
 * parts.  This way you can build textconverters specific to your
 * input format.
 */

#include <vector>

#include "term.h"
#include "type.h"
#include "alphabet.h"
#include "typer.h"

typedef map<string,int> Naming;

class TextConverter {
  protected:

    /* === PART 1: warnings ===
     * we keep track of one warning, which can be queried if TERM or
     * TYPE returns NULL.
     */
    string last_warning;

    /* === PART 2: basic functionality ===
     * The following functions provide basic string-handling
     * functionality.  None of them are expected to be overwritten.
     */
    void remove_outer_spaces(string &description);
    void remove_outer_brackets(string &description);
    void normalise(string &description);
      // removes outer brackets and spaces until none are left
    bool check_correct_bracket_matching(string description);
      // checks whether round brackets are nested correctly and sets
      // last_warning if not; other brackets are ignored
    int find_matching_bracket(string description, int start,
                              int dir = 1);
      // finds the bracket matching the given bracket in the string,
      // looking in the given direction (1 or -1)
    bool contains(string description, char c);
    int find_substring(string description, string sub);
      // returns the position of the given substring, -1 if it can't
      // be found; everything inside any brackets is ignored
    
    /* === PART 3: converting a string into a type ===
     * The following functions provide functionality for parsing
     * types.
     */
    virtual bool generic_character(char c);
      // returns whether c is a character that might be part of a
      // name (for a variable, type constructor etc)

    virtual PType convert_type(string &description, Naming &env);
      // does the main work for TYPE; env keeps track of the
      // type variables which are encountered
      
    virtual vector<PType> convert_typedeclaration(string &description,
                                                  Naming &env);
      // returns a type declaration; ret[0] is the output type, the
      // rest are the parameters

    /* === PART 4: reading a string into a term ===
     * A term or meta-term is first read into an PartTypedTerm, as
     * defined in typer.h.  Subsequently, the Typer is used to 
     * obtain a real term from this.
     */
    void split(string description, string &A, string &B, int &endp);
      // given a string A_{B}C returns A and B, and sets endp to the
      // index where C appears in description; if description
      // has an unclosed _{ part, endp is set to -1
    bool check_parts(string description, vector<string> &ret);
      // given a string representing an application of one or more
      // parts, splits it into the pieces of the application and
      // returns this as a vector; if parsing fails, returns false

    virtual
    PPartTypedTerm convert_term(string description, Alphabet &sigma,
                              Naming &Gamma, Naming &Zeta,
                              Naming &Theta, bool metadef);
      /* Sigma: the existing function symbols
       * Gamma: assigns to all variable names encountered so far an id
       * Zeta: assigns to all meta-variables names so far an id
       * Theta: assigns to aal typevariable names so far an id
       * metadef: true if unknown symbols should be interpreted as
       *   meta-variables, false if they should be interpreted as
       *   variables
       */
    virtual PPartTypedTerm check_constant(string description,
                           Alphabet &Sigma, Naming &Theta, bool &ok);
      // try whether the description is a constant, return NULL if not
      // (and set ok to false if it should be, but has an erroneous
      // type)
    virtual PPartTypedTerm check_variable(string description,
                           Naming &Gamma, Naming &Theta, bool &ok);
      // try whether the description might be a constant; return
      // NULL if not (and set ok to false if it should be, but some
      // problem occurred)
    virtual PPartTypedTerm check_abstraction(string description,
                           Alphabet &Sigma, Naming &Gamma,
                           Naming &Zeta, Naming &Theta, bool &ok,
                           bool metadef);
      // try whether the description is an abstraction; return NULL
      // if not and set ok to false if it should be but the term
      // cannot be parsed
    virtual PPartTypedTerm check_application(string description,
                           Alphabet &Sigma, Naming &Gamma,
                           Naming &Zeta, Naming &Theta, bool &ok,
                           bool metadef);
      // try whether the description is an application; return NULL
      // if not and set ok to false if it should be but the term
      // cannot be parsed
    virtual PPartTypedTerm check_meta(string description,
                           Alphabet &Sigma, Naming &Gamma,
                           Naming &Zeta, Naming &Theta, bool &ok);
      // try whether the description might be a meta-variable
      // application; return NULL if not and set ok to false if it
      // should be but the term cannot be parsed
    PPartTypedTerm make_single_term(vector<PPartTypedTerm> &l,
                                    vector<PPartTypedTerm> &r,
                                    Alphabet &F);
      // makes a complete term out of a list of partially typed
      // rules, to send to the typer in one go

  public:
    TextConverter() :last_warning("") {}

    string query_warning();
      // this returns the warning for any errors or an empty string
      // if there is none; it also resets the warning
    
    PType TYPE(string description);
      // turns a description of a type into a type (using
      // convert_type for the main part of the work)

    vector<PType> TYPEDEC(string description);
      // returns a "type declaration": for the return value ret,
      // ret[0] is the output type and the rest are the input
      // types, in order

    PTerm TERM(string description, Alphabet &sigma,
               PType expected_type = NULL);
      // turns a description of a (meta-)term into a term (using
      // convert_term for the main part of the work); if
      // expected_type is not given, the most general typing is
      // chosen
    
    void RULE(string left, string right, Alphabet &sigma, PTerm
              &tleft, PTerm &tright, PType expected_type = NULL);
      // turns two descriptions of meta-terms into two meta-terms
      // (using convert_term for the main part of the work); if
      // expected_type is not given, the most general typing is
      // chosen

    string fix_name(string txt);
      // given a string, uniquely assigns a string with only valid
      // characters for variable / constant names to it
};

#endif

