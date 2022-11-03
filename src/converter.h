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

#ifndef CONVERTER_H
#define CONVERTER_H

#include "matchrule.h"
#include "afs.h"

class Converter {
  private:
    vector<string> files;
    // runtime options
    string outputformat;
    string inputformat;
    string outputdir;
    string outputfile;

    string syntax();
    bool read_arguments(vector<string> args);

    string get_extension(string filename);
    string remove_extension(string filename);
    string get_basename(string filename);

    void convert(string form1, string form2, string fname);
    void output(string txt, string format, string inpfile);
    string read_file(string fname);
    bool simple_metavariables(vector<MatchRule*> &R);
    bool monomorphic(Alphabet &F);
    MonomorphicAFS *afsm_to_afs(Alphabet &F, vector<MatchRule*> &R);

    string print_as_xml(MonomorphicAFS *afs, string fname);
    string print_as_afsm(MonomorphicAFS *afs);
    string print_afsm(Alphabet &F, vector<MatchRule*> &R);

    string XMLify(PType type, string indent, TypeNaming &env);
    string XMLify(PTerm term, ArList &arity, string indent,
                  Environment &Xenv, TypeNaming &Tenv);

  public:
    void run(vector<string> args);
};

#endif

