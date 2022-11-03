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

#ifndef AFSREADER_H
#define AFSREADER_H

#include <map>
#include "textconverter.h"
#include "matchrule.h"
#include "afs.h"

/**
 * This class is used to read a monomorphic afs-format with all
 * information into an AFSM.  To this end, it is first read into an
 * AlgebraicFunctionalSystem class, and this is then converted to an
 * AFSM.
 */

class InputReaderAFS : public TextConverter {
  private:
    bool read_constant(string line, Alphabet &Sigma, ArList &arities);
    bool read_variable(string line, Environment &Gamma);
    bool read_rule(string line, Alphabet &Sigma, Environment &Gamma,
                   ArList &arities, vector<PTerm> &left, vector<PTerm> &right);
    
    string remove_whitespace(string txt);
    PTerm read_abstraction(string desc, Alphabet &Sigma,
                           Environment &Gamma, ArList &arities);
    PTerm read_application(string desc, Alphabet &Sigma,
                           Environment &Gamma, ArList &arities);
    PTerm read_functional(string desc, Alphabet &Sigma,
                          Environment &Gamma, ArList &arities);
    PTerm read_term(string desc, Alphabet &Sigma, Environment &Gamma,
                    ArList &arities);

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

    MonomorphicAFS *read_afs(string txt);
      /**
       * Reads the given AFS into the structure.  Doesn't do any
       * normalising yet.
       */
};

#endif

