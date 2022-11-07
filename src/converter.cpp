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

/**
 * Converter: converts examples in one formalism to another.
 */

#include "converter.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "alphabet.h"
#include "matchrule.h"
#include "inputreaderafs.h"
#include "inputreaderafsm.h"
#include "inputreaderatrs.h"
#include "xmlreader.h"
#include "substitution.h"

#define EXTENSION "~!~NONE*"

void Converter :: run(vector<string> args) {
  // no arguments given: print syntax and return
  if (args.size() == 0) {
    cout << syntax() << endl;
    return;
  }

  // read arguments and set global variables
  if (!read_arguments(args)) return;

  // convert all the files!
  for (int i = 0; i < files.size(); i++) {
    // check the two formats
    string form1 = inputformat;
    if (form1 == "") form1 = get_extension(files[i]);
    if (form1 != "afsm" && form1 != "afs" && form1 != "xml" &&
        form1 != "atrs") {
      cout << "Cannot convert file: " << files[i] << ": no legal "
           << "input type given." << endl << "(Legal types are "
           << "AFSM, AFS, XML and ATRS..)" << endl;
      continue;
    }
    string form2 = outputformat;

    convert(form1, form2, files[i]);
  }
}

string Converter :: syntax() {
  return
    "USAGE:\n  converter <files> <options>\n\n"
    "AVAILABLE OPTIONS:\n"
    "  -f format -- output format, should be afsm, afs, xml or html\n"
    "  -o file   -- output file\n"
    "  -d dir    -- output directory\n"
    "  -e        -- print files a.<extension> to a.<newextension>\n"
    "  -r format -- input files are presented in the given format "
                      "(by default this follows from its extension)"
    "\n";
}

string Converter :: get_extension(string filename) {
  int k = filename.find_last_of('.');
  if (k == string::npos) return "";
  else return filename.substr(k+1);
}

string Converter :: remove_extension(string filename) {
  int k = filename.find_last_of('.');
  if (k == string::npos) return filename;
  else return filename.substr(0,k);
}

string Converter :: get_basename(string filename) {
  int k = filename.find_last_of('/');
  if (k == string::npos) return filename;
  else return filename.substr(k+1);
}

bool Converter :: read_arguments(vector<string> args) {
  string err = "Error reading runtime arguments: ";
  for (int i = 0; i < args.size(); i++) {
    if (args[i] == "") continue;
    if (args[i][0] != '-') { files.push_back(args[i]); continue; }

    string cmd = args[i];

    // -e: we don't save to a special directory
    if (cmd == "-e") {
      if (outputdir == "") {
        outputdir = EXTENSION;
        continue;
      }
      if (outputdir == EXTENSION)
        cout << err << "-e given more than once." << endl;
      else
        cout << err << "cannot give both an output directory and "
             << "ask for an extension-change!" << endl;
      return false;
    }
    
    // the other options should be given a parameter
    if (cmd != "-o" && cmd != "-d" && cmd != "-f" && cmd != "-r") {
      cout << err << "unknown option: " << cmd << endl;
      return false;
    }

    if (i == args.size()-1) {
      cout << err << cmd << " given without parameter." << endl;
      return false;
    }
    string arg = args[i+1];
    i++;
    
    // -f format: output format
    if (cmd == "-f") {
      if (outputformat != "") {
        cout << err << "output format given more than once." << endl;
        return false;
      }
      outputformat = arg;
      continue;
    }

    // -o file: output file
    if (cmd == "-o") {
      if (outputfile != "") {
        cout << err << "output file given more than once." << endl;
        return false;
      }
      outputfile = arg;
      continue;
    }

    // -d dir: output directory
    if (cmd == "-d") {
      if (outputdir == "") {
        if (arg != "" && arg[arg.length()-1] != '/') arg += "/";
        outputdir = arg;
        continue;
      }
      if (outputdir == EXTENSION)
        cout << err << "cannot give both an output directory and "
             << "ask for an extension-change!" << endl;
      else cout << err << "output directory given twice" << endl;
      return false;
    }

    // -r format: input format
    if (cmd == "-r") {
      if (inputformat != "") {
        cout << err << "input format given more than once." << endl;
        return false;
      }
      inputformat = arg;
      continue;
    }

  } /* end for */

  if (files.size() == 0) {
    cout << err << "no files given!" << endl;
    return false;
  }

  if (outputdir != "" && outputfile != "") {
    cout << err << "options -o, -d and -e are mutually exclusive."
         << endl;
    return false;
  }

  if (files.size() > 1 && outputfile != "") {
    cout << err << "cannot write multiple files to the same output "
         << "file!" << endl;
    return false;
  }

  if (inputformat == "" && files.size() == 1)
    inputformat = get_extension(files[0]);

  // make sure input- and outputformat are lower case
  std::transform(inputformat.begin(), inputformat.end(), inputformat.begin(), ::tolower);
  std::transform(outputformat.begin(), outputformat.end(), outputformat.begin(), ::tolower);

  if (outputformat == "") {
    if (outputfile != "") outputformat = get_extension(outputfile);
    else if (files.size() == 1 && outputdir == "") {
      if (inputformat == "atrs" || inputformat == "xml") {
        cout << "No output format given, defaulting to afs" << endl
             << endl;
        outputformat = "afs";
      }
    }
    if (outputformat == "") {
      cout << err << "no outputformat given." << endl;
      return false;
    }
  }

  if (inputformat != "afsm" && inputformat != "afs" &&
      inputformat != "xml" && inputformat != "atrs" &&
      inputformat != "") {
    cout << err << "illegal input format: can only read "
         << "AFSM, AFS, XML or ATRS." << endl;
    return false;
  }

  if (outputformat != "afsm" && outputformat != "afs" &&
      outputformat != "xml" && outputformat != "html") {
    cout << err << "illegal output format: can only convert to "
         << "AFSM, AFS, XML and HTML." << endl;
    return false;
  }

  return true;
}

string Converter :: read_file(string fname) {
  string txt;

  ifstream file(fname.c_str());
  while (!file.eof()) {
    string input;
    getline(file, input);
    txt += input + "\n";
  }

  return txt;
}

void Converter :: convert(string form1, string form2, string fname) {
  MonomorphicAFS *afs = NULL;
  XMLReader x;
  InputReaderAFS iafs;
  InputReaderATRS iatrs;
  
  // check the things where we don't need to bother reading anything
  if (form1 == form2 && form1 != "afsm") { output(read_file(fname), form2, fname); return; }
  else if (form1 == "xml" && form2 == "afs") { output(x.read_file(fname), form2, fname); return; }
  else if (form1 == "xml" && form2 == "html") { output(x.read_as_html(fname), form2, fname); return; }

  // afsm => afsm: to correct things like extra brackets
  if (form1 == "afsm" && form2 == "afsm") {
    Alphabet Sigma;
    vector<MatchRule*> rules;
    InputReaderAFSM iafsm;
    if (!iafsm.read_file(fname, Sigma, rules)) {
      cout << "Could not read " << fname << ":\n  "
           << iafsm.query_warning() << endl;
    }
    else output(print_afsm(Sigma, rules), "afsm", fname);
    return;
  }
  
  // read the input into an afs (it always either *is* one, or needs
  // to be converted to it)
  if (form1 == "xml") afs = iafs.read_afs(x.read_file(fname));
  if (form1 == "afs") afs = iafs.read_afs(read_file(fname));
  if (form1 == "atrs") {
    afs = iatrs.read_as_afs(fname);
    if (afs != NULL) afs->recalculate_arity();
  }
  if (form1 != "afsm" && afs == NULL) {
    cout << "Could not read " << fname << ":\n  ";
    if (form1 == "atrs") cout << iatrs.query_warning() << endl;
    else cout << iafs.query_warning() << endl;
    return;
  }
  if (form1 == "afsm") {
    Alphabet Sigma;
    vector<MatchRule*> rules;
    InputReaderAFSM iafsm;
    if (!iafsm.read_file(fname, Sigma, rules)) {
      cout << "Could not read " << fname << ":\n  "
           << iafsm.query_warning() << endl;
      return;
    }
    afs = afsm_to_afs(Sigma, rules);
    if (afs == NULL) {
      cout << "AFSM " << fname << " could not be converted to an "
           << "AFS." << endl;
      return;
    }
    afs->recalculate_arity();
  }

  if (form2 == "afs") output(afs->to_string(), form2, fname);
  if (form2 == "html") {
    cout << "Can only convert the xml-style input to html." << endl;
  }
  if (form2 == "afsm") {
    string reason;
    if (afs->trivially_terminating(reason)) {
      cout << "System from " << fname << " is trivially terminating "
           << "(" << reason << "), so is not converted."
           << endl;
    }
    else if (afs->trivially_nonterminating(reason)) {
      cout << "System from " << fname << " is trivially "
           << "non-terminating (" << reason << "), so is not "
           << "converted." << endl;
    }
    else output(print_as_afsm(afs), form2, fname);
  }
  if (form2 == "xml") output(print_as_xml(afs, fname), form2, fname);
  delete afs;
}

void Converter :: output(string txt, string format, string inpfile) {
  if (outputfile == "" && outputdir == "") {
    cout << txt;
    return;
  }

  string fname = outputfile;
  if (outputdir == EXTENSION)
    fname = remove_extension(inpfile) + "." + format;
  else if (outputdir != "")
    fname = outputdir + remove_extension(get_basename(inpfile)) +
            "." + format;

  cout << "Writing to " << fname << "..." << endl;
  ofstream f(fname.c_str());
  f << txt;
  f.close();
}

bool Converter :: simple_metavariables(vector<MatchRule*> &R) {
  vector<PTerm> subs;
  for (int i = 0; i < R.size(); i++) {
    subs.push_back(R[i]->query_left_side());
    subs.push_back(R[i]->query_right_side());
  }
  for (int j = 0; j < subs.size(); j++) {
    if (subs[j]->query_application() || subs[j]->query_abstraction())
      subs.push_back(subs[j]->subterm("1"));
    if (subs[j]->query_application())
      subs.push_back(subs[j]->subterm("2"));
    if (subs[j]->query_meta() && subs[j]->subterm("0") != NULL)
      return false;
  }
  return true;
}

bool Converter :: monomorphic(Alphabet &F) {
  vector<string> symbols = F.get_all();
  for (int i = 0; i < symbols.size(); i++) {
    if (F.query_type(symbols[i])->vars().size() != 0) return false;
  }
  return true;
}


MonomorphicAFS *Converter :: afsm_to_afs(Alphabet &F,
                                          vector<MatchRule*> &R) {
  if (!simple_metavariables(R)) return NULL;
  if (!monomorphic(F)) return NULL;

  Substitution sub;
  Environment gamma;
  int i;

  // extract the environment, and substitute a variable for all
  // meta-variables
  for (i = 0; i < R.size(); i++) {
    PTerm l = R[i]->query_left_side();
    Varset FV = l->free_var(true);
    for (Varset::iterator it = FV.begin(); it != FV.end(); it++) {
      PType type = l->lookup_type(*it);
      if (type == NULL) continue;   // errrrr
      gamma.add(*it, type->copy());
      sub[*it] = new Variable(type->copy(), *it);
    }
  }

  // create an AFS!
  vector<PTerm> lhs;
  vector<PTerm> rhs;
  for (i = 0; i < R.size(); i++) {
    PTerm l = R[i]->query_left_side()->apply_substitution(sub);
    PTerm r = R[i]->query_right_side()->apply_substitution(sub);
    lhs.push_back(l);
    rhs.push_back(r);
  }

  R.clear();
  MonomorphicAFS *ret = new MonomorphicAFS(F, gamma, lhs, rhs);
  ret->recalculate_arity();
  return ret;
}

string Converter :: print_afsm(Alphabet &F, vector<MatchRule*> &R) {
  string ret;
  int i;

  vector<string> symb = F.get_all();
  for (i = 0; i < symb.size(); i++) {
    ret += symb[i] + " : " + F.query_type(symb[i])->to_string() + "\n";
  }
  ret += "\n";

  for (i = 0; i < R.size(); i++) {
    ret += R[i]->to_string() + "\n";
    delete R[i];
  }
  ret += "\n";

  return ret;
}

string Converter :: print_as_afsm(MonomorphicAFS *afs) {
  Alphabet F;
  vector<MatchRule*> R;
  string ret = "";

  afs->to_afsm(F, R);
  return print_afsm(F, R);
}

string Converter :: XMLify(PType type, string indent, TypeNaming &env) {
  string ret;

  if (type->query_data())
    ret = "<basic>" + type->to_string() + "</basic>";
  if (type->query_composed())
    ret += "<arrow>\n" +
           XMLify(type->query_child(0), indent+"  ", env) +
           XMLify(type->query_child(1), indent+"  ", env) +
           indent + "</arrow>";
  if (type->query_typevar())
    ret += "<typevariable>" + type->to_string(env) +
           "</typevariable>";

  return indent + "<type>" + ret + "</type>\n";
}

string Converter :: XMLify(PTerm term, ArList &arity, string indent,
                           Environment &Xenv, TypeNaming &Tenv) {

  // variables
  if (term->query_variable()) {
    return indent + "<var>" + term->to_string(Xenv) + "</var>\n";
  }

  // abstractions
  if (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    PVariable x = abs->query_abstraction_variable();
    return indent + "<lambda>\n" +
           XMLify(x, arity, indent+"  ", Xenv, Tenv) +
           XMLify(x->query_type(), indent + "  ", Tenv)+
           XMLify(abs->get_child(0), arity, indent + "  ", Xenv, Tenv) +
           indent + "</lambda>\n";
  }

  // function applications
  if (term->query_head()->query_constant()) {
    vector<PTerm> tsplit = term->split();
    string f = tsplit[0]->to_string(false);
    int ar = (arity.find(f) != arity.end() ? arity[f] : 0);
    int Nargs = tsplit.size()-1;
    if (Nargs <= ar) {
      string ret = indent + "<funapp>\n" + indent + "  <name>" + f +
                   "</name>\n";
      for (int i = 1; i < tsplit.size(); i++) {
        ret += indent + "  <arg>\n" +
               XMLify(tsplit[i], arity, indent+"    ", Xenv, Tenv) +
               indent + "  </arg>\n";
      }
      ret += indent + "</funapp>\n";
      return ret;
    }
  }

  // other applications
  if (!term->query_application()) {
    cout << "Error finding XML version of term: cannot parse "
         << term->to_string() << endl;
    return "ERR";
  }
  return indent + "<application>\n" +
         XMLify(term->get_child(0), arity, indent+"  ", Xenv, Tenv)+
         XMLify(term->get_child(1), arity, indent+"  ", Xenv, Tenv)+
         indent + "</application>\n";
}

string Converter :: print_as_xml(MonomorphicAFS *afs, string fname) {
  TypeNaming tn;
  string alphabet;
  string environment;
  string rules;
  string trs;
  int i;

  // print the alphabet
  alphabet = "      <functionSymbolTypeInfo>\n";
  vector<string> symb = afs->Sigma.get_all();
  for (i = 0; i < symb.size(); i++) {
    // calculate the parts of the type declaration
    int arity = afs->arities[symb[i]];
    PType type = afs->Sigma.query_type(symb[i]);
    vector<string> types;
    int j;
    for (j = 0; j < arity; j++) {
      types.push_back(XMLify(type->query_child(0), "            ", tn));
      type = type->query_child(1);
    }
    types.push_back(XMLify(type, "            ", tn));
    // and print it
    alphabet += "        <funcDeclaration>\n"
                "          <name>" + symb[i] + "</name>\n"
                "          <typeDeclaration>\n";
    for (j = 0; j < types.size(); j++) alphabet += types[j];
    alphabet += "          </typeDeclaration>\n"
                "        </funcDeclaration>\n";
  }
  alphabet += "      </functionSymbolTypeInfo>\n";

  // print the environment
  environment = "      <variableTypeInfo>\n";
  Varset vars = afs->gamma.get_variables();
  afs->rename_variables();
  for (Varset::iterator it = vars.begin(); it != vars.end(); it++) {
    PType type = afs->gamma.get_type(*it);
    string name = afs->gamma.get_name(*it);
    environment += "        <varDeclaration>\n          <var>" +
                   name + "</var>\n" + XMLify(type, "          ", tn)
                   + "        </varDeclaration>\n";
  }
  environment += "      </variableTypeInfo>\n";

  // print the rules
  rules = "    <rules>\n";
  for (i = 0; i < afs->lhs.size(); i++) {
    rules += "      <rule>\n        <lhs>\n";
    string l = XMLify(afs->lhs[i], afs->arities, "          ",
                      afs->gamma, tn);
    rules += l + "        </lhs>\n        <rhs>\n";
    string r = XMLify(afs->rhs[i], afs->arities, "          ",
                      afs->gamma, tn);
    rules += r + "        </rhs>\n      </rule>\n";
  }
  rules += "    </rules>\n";

  // combine this into a trs
  trs = "  <trs>\n" + rules + "    <higherOrderSignature>\n" +
        environment + alphabet + "    </higherOrderSignature>\n" +
        "  </trs>\n";

  return
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<?xml-stylesheet href=\"xtc2tpdb.xsl\" type=\"text/xsl\"?>\n"
    "<problem xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
    "type=\"termination\" xsi:noNamespaceSchemaLocation=\""
    "http://dev.aspsimon.org/xtc.xsd\">\n" + trs +
    "  <strategy>FULL</strategy>\n" +
    "  <metainformation>\n"
    "    <originalfilename>" + fname + "</originalfilename>\n"
    "  </metainformation>\n"
    "</problem>\n";
}

/** main function **/

int main(int argc, char **argv) {
  vector<string> args;
  for (int i = 1; i < argc; i++) args.push_back(argv[i]);

  Converter conv;
  conv.run(args);
  return 0;
}
