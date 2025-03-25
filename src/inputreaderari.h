/**************************************************************************
   Copyright 2024-2025 Cynthia Kop

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

#ifndef ARIREADER_H
#define ARIREADER_H

#include "alphabet.h"
#include "matchrule.h"

enum TokenKinds { BROPEN, BRCLOSE, FORMAT, FUN, SORT, RULE, LAMBDA,
                  ARROW, ID };

struct Token {
  int row;
  int col;
  TokenKinds kind;
  string text;
};

/** A ParseTree is EITHER a single token, OR a non-empty vector of subtrees. */
struct ParseTree {
  Token *token;
  vector<ParseTree*> subtrees;
  ParseTree() { token = NULL; }
  ParseTree(Token *t) { token = t; }
  ~ParseTree() {
    for (int i = 0; i < subtrees.size(); i++) delete subtrees[i];
  }
};

/**
 * This class is used to read the higher-order format from the termination
 * and confluence competitions.  This format supplies an AFSM.  The
 * functionality is split into a lexing and a parsing phase.
 */
class InputReaderARI {
  private:
    string last_warning;

    bool tokenise(string txt, vector<Token> &tokens);
      /**
       * tokenises the given string, adding to the end of tokens
       * returns true if successful, false if the file has illegal tokens
       */
    ParseTree *make_tree(vector<Token> &tokens);
      /**
       * turns the list of tokens into a parse tree by treating each occurrence
       * of (...) as a separate subtree; returns NULL if parsing failed
       */
    bool verify_shape(ParseTree *tree);
      /**
       * this checks if the given parse tree has the overall shape of a hotrs
       * in the ARI higher-order syntax
       */

    string fix_name(string txt);
      /** makes a name adhere to Wanda naming standards */
    int read_format(ParseTree *tree);
      /** checks that the format line is as expected: (format <format>) */
    int read_sort(ParseTree *tree, set<string> &sorts);
      /** reads tokens for a (sort <identifier>) line, adding into sorts */
    PType read_type(ParseTree *tree, set<string> &sorts);
      /**
       * reads tokens for an IDENTIFIER | (-> type+ IDENTIFIER ) line,
       * returning the result as a PType; if parsing failed, then last_warning
       * is set and NULL returned
       */
    int read_func(ParseTree *tree, set<string> &sorts, Alphabet &Sigma);
      /**
       * reads tokens for a (fun <identifier> <type>) line, adding the
       * resulting function symbol declarations into Sigma
       */
    PTerm read_term(ParseTree *tree, set<string> &sorts, Alphabet &Sigma,
                    map<string,PVariable> &variables, map<string,int> &arities,
                    PType expected, bool lhs);
      /**
       * Reads a term and returns it, or returns NULL if the given tree could
       * not be parsed as a term.  Here, expected is allowed to be NULL.
       */
    PTerm read_single_token_term(Token *token, set<string> &sorts, Alphabet
                    &Sigma, map<string,PVariable> &variables, map<string,int>
                    &arities, PType expected, bool lhs);
    PTerm read_application(ParseTree *tree, set<string> &sorts, Alphabet
                    &Sigma, map<string,PVariable> &variables, map<string,int>
                    &arities, PType expected, bool lhs);
    PTerm read_application(PTerm head, ParseTree *tree, set<string> &sorts,
                    Alphabet &Sigma, map<string,PVariable> &variables,
                    map<string,int> &arities, PType expected, bool lhs);
    PTerm read_meta_application(PVariable F, ParseTree *tree, set<string>
                    &sorts, Alphabet &Sigma, map<string,PVariable> &variables,
                    map<string,int> &arities, PType expected, bool lhs);
    PTerm read_abstraction(ParseTree *tree, set<string> &sorts, Alphabet
                    &Sigma, map<string,PVariable> &variables, map<string,int>
                    &arities, PType expected, bool lhs);
    bool check_all_bound_variables(ParseTree *tree, map<string,PVariable>
                                   &variables, map<string,int> &arities);
    PTerm verify_type(PTerm term, PType expected);
      /**
       * Returns term if either expected is NULL, or the type of term is
       * expected, and otherwise stores an error message, deletes the term,
       * and returns NULL,
       */
    int read_rule(ParseTree *tree, set<string> &sorts, Alphabet &Sigma,
                  vector<MatchRule*> &rules);
      /** reads tokens for a (rule <term> <term>) line */

  public:
    int read_file(string filename, Alphabet &Sigma,
                  vector<MatchRule*> &rules);
      /** 
       * POSSIBLE RETURN VALUES:
       * 0: parsing succesful
       * 1: parsing failed with some error
       * 2: system has no error, but cannot be converted
       * 3: system is obviously terminating (warning contains the proof)
       * 4: system is obviously non-terminating (warning contains the proof)
       */
    int read_text(string txt, Alphabet &Sigma,
                  vector<MatchRule*> &rules);
      /**
       * Same as read_file, only it parses from the given string
       * rather than the contents of a file.
       */

    string query_warning();
      /** this returns the warning for any errors or an empty string
       * if there is none; it also resets the warning */
};

#endif

