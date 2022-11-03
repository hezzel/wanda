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

#ifndef INPUTREADERAFSM_H
#define INPUTREADERAFSM_H

/**
 * This class reads an AFSM from a textual formalism, either from
 * standard in or from a file.
 */

#include "textconverter.h"
#include "matchrule.h"

class InputReaderAFSM : public TextConverter {
  private:
    PConstant read_constant(string description);
      // reads a constant of the form f : (o -> o) -> o
      // sets last_warning and returns NULL if there's a problem
    MatchRule *read_rule(string description, Alphabet &Sigma);
      // reads a rule of the form l => r
      // sets last_warning and returns NULL if there's a problem

  public:
    bool read_manually(Alphabet &Sigma, vector<MatchRule*> &rules);
      // returns false if user has chosen to abort
    bool read_file(string filename, Alphabet &Sigma,
                   vector<MatchRule*> &rules);
      // returns false if the file has errors
};

#endif

