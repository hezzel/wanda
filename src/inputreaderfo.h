/**************************************************************************
   Copyright 2013 Cynthia Kop

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

#ifndef FOREADER_H
#define FOREADER_H

#include <map>
#include "textconverter.h"
#include "matchrule.h"
#include "typer.h"

/**
 * This class is used to read a human-readable first-order TRS into
 * an AFSM.  It is not fully developed yet!
 */

class InputReaderFO : public TextConverter {
  private:
    map<string,string> split_parts(string txt);
    bool read_rule(string line, Alphabet &Sigma, Environment &Gamma,
                   PTerm &left, PTerm &right);
    
    string remove_whitespace(string txt);
    PTerm read_term(string desc, Alphabet &Sigma, Environment &Gamma,
                    PType expected_type, bool funsdeclared, bool varasmeta);
    PTerm read_functional(string desc, Alphabet &Sigma, Environment &Gamma,
                          PType expected_type, bool funsdeclared,
                          bool varasmeta);
    PTerm read_variable(string desc, Environment &Gamma,
                        PType expected_type, bool varasmeta);
    bool assign_types(Alphabet &F, vector<MatchRule*> &rules);
    PPartTypedTerm make_parttyped_term(PTerm original,
                                       map<string,int> &naming);
    PType make_sort_constants(PType polysort);

  public:
    PTerm parse_term(string desc, Alphabet &Sigma);
      /**
       * Reads the given string into a term; returns NULL if this
       * (for type reasons or other problems).
       */

    int read_file(string filename, Alphabet &Sigma,
                  vector<MatchRule*> &rules,
                  bool &innermost);
      /** 
       * POSSIBLE RETURN VALUES:
       * 0: parsing succesful
       * 1: parsing failed with some error
       * 2: system has no error, but cannot be converted
       * 3: system is obviously terminating (warning contains the proof)
       * 4: system is obviously non-terminating (warning contains the proof)
       *
       * Innermost is set to true if an innermost strategy is requested,
       * not just if we can derive that one is needed.
       */
    int read_text(string txt, Alphabet &Sigma,
                  vector<MatchRule*> &rules,
                  bool &innermost);
      /**
       * Same as read_file, only it parses from the given string
       * rather than the contents of a file.
       */
};

#endif

