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

#ifndef ATRSREADER_H
#define ATRSREADER_H

/**
 * This class is used to read an ATRS: an untyped applicative system.
 * If possible, such a system is transformed into a monomorphic AFSM
 * If not possible, it fails.
 * There are two styles of typing attempted: either use a single base
 * type, or assign different base types wherever possible.
 */

#include "textconverter.h"
#include "matchrule.h"
#include "afs.h"

class InputReaderATRS : public TextConverter {
  private:
    PPartTypedTerm parse_atrs_term(string txt, Naming &constants,
                                   Naming &vars);
    int parse_system(vector<string> &vars,
                     vector<string> &srules,
                     Alphabet &Sigma,
                     vector<MatchRule*> &rules);
    void fix_newlines(string &text);
      // replaces all entries of either \n or \r by \n

    string readf(string filename);
    string get_part(string txt, string part);
    bool read_vars(string txt, vector<string> &vars);
    bool read_rules(string txt, vector<string> &l, vector<string> &r);
    bool read_rule(string txt, vector<string> &l, vector<string> &r);
    bool parse_rules(vector<string> &sl, vector<string> &sr,
             vector<PPartTypedTerm> &pl, vector<PPartTypedTerm> &pr,
             vector<string> &vars, Naming &consnames);
    int new_index();
    PTerm type_term(PPartTypedTerm term, Alphabet &F);
    void get_alphabet(PTerm &system, Naming &consnames, Alphabet &F);
    void get_environment(PTerm system, Environment &G);
    void get_rules(PTerm &system, vector<PTerm> &lhs, vector<PTerm> &rhs);

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
    
    MonomorphicAFS *read_as_afs(string filename);
      // returns NULL if parsing failed with some error
};

#endif

