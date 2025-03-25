/**************************************************************************
   Copyright 2012, 2013, 2019 Cynthia Kop

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

#include "wanda.h"
#include "beta.h"
#include "dpframework.h"
#include "inputreaderafsm.h"
#include "inputreaderatrs.h"
#include "inputreaderari.h"
#include "inputreaderafs.h"
#include "inputreaderfo.h"
#include "nonterminator.h"
#include "outputmodule.h"
#include "ruleremover.h"
#include "xmlreader.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <random>

void Wanda :: run(vector<string> args) {
  //if (args.size() >= 2) args.pop_back();
    // competition parameter, remove afterwards

  parse_runtime_arguments(args);

  // if runtime errors had an argument or were easy to deal with
  // immediately, get out
  if (error != "") {
    cout << "ERROR" << endl << error << endl;
    return;
  }
  if (args.size() == 0) args.push_back("--manual");

  // read all the input files, and work with them!
  aborted = false;
  total_yes = total_no = total_maybe = 0;
  for (int i = 0; i < args.size(); i++) {
    // basic printing: show what we're working on
    if (args.size() > 1) {
      if (!wout.query_verbose() && silent) cout << args[i] << ": ";
      else cout << "++" << args[i] << ":" << endl;
    }

    // read the system
    wout.print("We consider the system " +
      wout.parse_filename(args[i]) + ".\n");
    read_system(args[i]);
    if (error != "") {
      cout << "ERROR" << endl;
      if (!silent) cout << error << endl;
      error = "";
      continue;
    }
    if (aborted) {
      aborted = false;
      continue;
    }
    wout.print_system(Sigma, rules);

    // and deal with it
    if (do_rewriting) rewrite_term();
    else if (query != "") check_query();
    else if (just_show) wout.print_output(outputfile);
    else if (formal) certify_termination_status();
    else determine_termination();

    // clear alphabet and rules for this run
    Sigma.clear();
    for (int j = 0; j < rules.size(); j++) delete rules[j];
    rules.clear();
  }

  // print statistics
  if (args.size() > 1) {
    cout << endl
         << "TOTAL YES:   " << total_yes << endl
         << "TOTAL NO:    " << total_no << endl
         << "TOTAL MAYBE: " << total_maybe << endl;
  }
}

void Wanda :: parse_runtime_arguments(vector<string> &args) {
  formalism = "";
  silent = false;
  error = "";
  do_rewriting = false;
  just_show = false;
  firstorder = "firstorderprover";
  firstordernont = "firstordernonprover";
  outputfile = "";
  string disable = "";
  string style = "";
  use_betafirst = false;
  simplify_meta = true;
  formal = false;

  int i;
  for (i = 0; i < args.size(); i++) {
    string arg = args[i];

    if (arg == "") continue;
    if (arg[0] != '-') continue;  // filename or timeout

    // first deal with the verbose arguments
    if (arg == "--verbose") wout.set_verbose(true);
    else if (arg == "--silent") silent = true;
    else if (arg == "--debug") wout.set_debugmode(true);
    else if (arg == "--rewrite") do_rewriting = true;
    else if (arg == "--show") just_show = true;
    else if (arg == "--betafirst") use_betafirst = true;
    else if (arg == "--dontsimplify") simplify_meta = false;
    else if (arg == "--formal") { formal = true; simplify_meta = false; }
    else if (arg.substr(0,9) == "--format=")
      formalism = arg.substr(9);
    else if (arg.substr(0,13) == "--firstorder=")
      firstorder = arg.substr(13);
    else if (arg.substr(0,13) == "--firstordernon=")
      firstordernont = arg.substr(16);
    else if (arg.substr(0,10) == "--disable=")
      disable = arg.substr(10);
    else if (arg.substr(0,8) == "--query=")
      query = arg.substr(8);
    else if (arg.substr(0,8) == "--style=")
      style = arg.substr(8);
    else if (arg.substr(0,9) == "--output=")
      outputfile = arg.substr(9);
    else if (arg.substr(0,2) == "--") {
      error = "Could not parse runtime arguments: unknown "
        "parameter, '" + arg + "'.";
      return;
    }
    if (arg.substr(0,2) == "--") continue;

    // okay, args[i] has the form -<something>; check the something
    for (int j = 1; j < arg.length(); j++) {
      if (arg[j] == 'v') wout.set_verbose(true);
      else if (arg[j] == 's') silent = true;
      else if (arg[j] == 'r') do_rewriting = true;
      else if (arg[j] == 'D') wout.set_debugmode(true);
      else if (arg[j] == 'w') just_show = true;
      else if (arg[j] == 'l') formal = true;
      else if (arg[j] == 'f' || arg[j] == 'i' || arg[j] == 'd' ||
               arg[j] == 'q' || arg[j] == 'y' || arg[j] == 'o' ||
               arg[j] == 'n') {
        if (i == args.size()-1) {
          char name[] = "- ";
          name[1] = arg[j];
          error = "Could not parse runtime arguments: " + string(name) +
            " should be followed by a parameter!";
          return;
        }
        if (arg[j] == 'f') formalism = args[i+1];
        if (arg[j] == 'i') firstorder = args[i+1];
        if (arg[j] == 'n') firstordernont = args[i+1];
        if (arg[j] == 'd') disable = args[i+1];
        if (arg[j] == 'q') query = args[i+1];
        if (arg[j] == 'y') style = args[i+1];
        if (arg[j] == 'o') outputfile = args[i+1];
        args[i+1] = "--";
        i++;
      }
      else {
        string s = string(1,arg[j]);
        error = "Could not parse runtime arguments: unknown "
          "parameter, -" + s + ".";
        return;
      }
    }
  }

  // check whether the given formalism is acceptable
  if (formalism != "" && !known_formalism(formalism)) {
    error = "Could not parse runtime arguments: do not know "
            "formalism '" + formalism + "'.";
    return;
  }

  // deal with disable
  allow_nontermination = (disable.find("nt") == string::npos);
  allow_rulesremoval   = (disable.find("rem") == string::npos);
  allow_redpair        = (disable.find("rr") == string::npos);
  allow_dp             = (disable.find("dp") == string::npos);
  allow_subcrit        = (disable.find("sc") == string::npos);
  allow_static_dp      = (disable.find("static") == string::npos);
  allow_dynamic_dp     = (disable.find("dynamic") == string::npos);
  allow_polynomials    = (disable.find("poly") == string::npos);
  allow_polyprod       = (disable.find("pprod") == string::npos);
  allow_horpo          = (disable.find("horpo") == string::npos);
  allow_usable         = (disable.find("ur") == string::npos);
  allow_formative      = (disable.find("fr") == string::npos);
  allow_local          = (disable.find("local") == string::npos);
  allow_graph          = (disable.find("graph") == string::npos);
  allow_uwrt           = (disable.find("uwrt") == string::npos);
  allow_fwrt           = (disable.find("fwrt") == string::npos);

  if (!allow_redpair) allow_rulesremoval = false;

  // deal with style
  if (style == "plain") {
    wout.set_html(false);
    wout.set_use_colour(false);
    wout.set_use_utf(false);
  }
  else if (style == "html") {
    wout.set_html(true);
    wout.set_use_colour(false);
    wout.set_use_utf(false);
  }
  else if (style == "ansi") {
    wout.set_html(false);
    wout.set_use_colour(true);
    wout.set_use_utf(false);
  }
  else if (style == "utf") {
    wout.set_html(false);
    wout.set_use_colour(false);
    wout.set_use_utf(true);
  }
  else if (style == "ansiutf") {
    wout.set_html(false);
    wout.set_use_colour(true);
    wout.set_use_utf(true);
  }
  else if (style != "") {
    error = "Unknown style: " + style;
    return;
  }

  // prune arguments so only the files remain, and check formalisms
  for (i = 0; i < args.size(); i++) {
    if (args[i] == "" || args[i][0] == '-' || is_number(args[i])) {
      for (int j = i; j < args.size()-1; j++) args[j] = args[j+1];
      args.pop_back();
      i--;
    }
    else {
      string extension = get_extension(args[i]);
      if (formalism == "" && !known_formalism(extension)) {
        error = "Cannot read " + args[i] + ": unknown formalism.";
        return;
      }
    }
  }

  if (do_rewriting) {
    wout.set_verbose(true);
    if (args.size() > 1) {
      error = "Please use --rewrite only with a single or no input "
        "file.";
      return;
    }
  }
}

bool Wanda :: known_formalism(string format) {
  return format == "afsm" || format == "atrs" || format == "afs" ||
         format == "xml" || format == "trs" || format == "ari" || format == "";
}

string Wanda :: get_extension(string filename) {
  int k = filename.find_last_of('.');
  if (k == string::npos) return "";
  else return filename.substr(k+1);
}

bool Wanda :: is_number(string txt) {
  for (int i = 0; i < txt.length(); i++) {
    if (txt[i] < '0' || txt[i] > '9') return false;
  }
  return true;
}

void Wanda :: read_system(string filename) {
  if (filename == "--manual") {
    InputReaderAFSM reader;
    if (!reader.read_manually(Sigma, rules)) aborted = true;
    return;
  }

  string extension = get_extension(filename);
  if (formalism != "") extension = formalism;

  if (extension == "afsm") {
    InputReaderAFSM reader;
    if (!reader.read_file(filename, Sigma, rules)) {
      error = reader.query_warning();
      if (error == "") error = "Unknown error reading the file.";
    }
    return;
  }

  if (extension != "atrs" && extension != "afs" && extension != "ari" &&
      extension != "xml" && extension != "trs") {

    error = "Unsupported style: " + extension + ".";
    return;
  }

  int k = 2;
  string warning;

  // get the right reader, and see what it makes of the file
  if (extension == "atrs") {
    InputReaderATRS reader;
    k = reader.read_file(filename, Sigma, rules);
    warning = reader.query_warning();
  }
  if (extension == "afs") {
    InputReaderAFS reader;
    k = reader.read_file(filename, Sigma, rules);
    warning = reader.query_warning();
  }
  if (extension == "ari") {
    InputReaderARI reader;
    k = reader.read_file(filename, Sigma, rules);
    warning = reader.query_warning();
  }
  if (extension == "xml") {
    XMLReader xreader;
    string strategy;
    string txt = xreader.read_file(filename, strategy);
    if (txt == "TRS") {
      size_t k = filename.find(" ");
      while (k != string::npos) {
        filename.replace(k, 1, "\\ ");
        k = filename.find(" ", k + 2);
      }
      system(("xsltproc resources/xtc2tpdb.xsl " + filename +
             " > tmp.trs").c_str());
      filename = "tmp.trs";
      extension = "trs";
    }
    if (txt.find("!ERR!") != string::npos) {
      error = "Could not parse system:\n" + txt;
      return;
    }
    if (strategy != "FULL") {
      allow_nontermination = false;
      wout.verbose_print("This file asks us to use use a " + strategy +
        " strategy; since we ignore strategies, we should not attempt to "
        "prove non-termination!\n");
    }
    if (extension != "trs") {
      InputReaderAFS reader;
      k = reader.read_text(txt, Sigma, rules);
      warning = reader.query_warning();
    }
  }
  if (extension == "trs") {
    InputReaderFO reader;
    bool innermost;
    k = reader.read_file(filename, Sigma, rules, innermost);
    warning = reader.query_warning();
#ifndef TESTAPROVE
    firstorder = "none";
#endif
    if (innermost) allow_nontermination = false;
  }

  // deal with the results
  if (k == 2 || k == 3 || k == 4) {
    if (k == 2) cout << "MAYBE" << endl;
    if (k == 3) cout << "YES" << endl;
    if (k == 4) cout << "NO" << endl;
    if (!silent) cout << warning << endl;
    aborted = true;
  }
  else if (k != 0) {
    error = warning;
    if (error == "") error = "Unknown error reading the file.";
  }
}

void Wanda :: write_system() {
  cout << "Alphabet: " << Sigma.to_string() << endl;
  cout << "Rules:" << endl;
  for (int i = 0; i < rules.size(); i++)
    cout << "  " << rules[i]->to_string() << endl;
  cout << endl;
}

// Windows compatibility layer for generation of random values.
// Here, we use the <random> header for generating random integers.
// This random number generation is compatible with non POSIX systems,
// So no usage of "random()" is necessary.
// Documentation: https://en.cppreference.com/w/cpp/numeric/random
int generate_uniform_int(int n) {
    std::random_device rd;
    std::mt19937 e2(rd());
    std::uniform_int_distribution<> dist(1, n);
    return dist(e2);
}

void Wanda :: rewrite_term() {
  cout << "Please enter a term you would like to see normalised.  "
       << "The type may be included (in the form term : type) or "
       << "omitted if it is evident." << endl << "> ";

  string sterm;
  PTerm term = NULL;
  TextConverter converter;

  getline(cin, sterm);
  if (sterm == "ABORT" || sterm == "") return;
  int colon = sterm.find_last_of(':');
  if (colon == string::npos || sterm.find('.', colon+1) != string::npos) {
    term = converter.TERM(sterm, Sigma);
  }
  else {
    PType type = converter.TYPE(sterm.substr(colon+1));
    if (type != NULL) {
      term = converter.TERM(sterm.substr(0,colon), Sigma, type);
      delete type;
    }
  }
  if (term == NULL) {
    cout << "ERROR" << endl << converter.query_warning() << endl;
    rewrite_term();
    return;
  }

  // we have a valid term - rewrite it!
  cout << term->to_string();
  NonTerminator nonterminator(Sigma, rules, use_betafirst);
  while (true) {
    // find reducable positions
    vector<string> reducable_pos;
    vector<Rule*> reducable_rule;
    nonterminator.possible_reductions(term, reducable_rule,
                                      reducable_pos);
    if (reducable_pos.size() == 0) break;
    // int N = random() % reducable_pos.size();
    int N = generate_uniform_int(reducable_pos.size());
    term = reducable_rule[N]->apply(term, reducable_pos[N]);
    cout << "  =>" << endl << "  " << term->to_string();
  }
  cout << endl;
}

void Wanda :: respond(string answer) {
  if (answer == "NO") total_no++;
  if (answer == "YES") total_yes++;
  if (answer == "MAYBE") total_maybe++;

  cout << answer << endl;
  if (!silent) { wout.print_output(outputfile); }
}

void Wanda :: respond_bool(bool answer) {
  if (answer) respond("YES");
  else respond("NO");
}

void Wanda :: check_query() {
  RulesManipulator manipulator;
  map<string,int> sortord;

  if (query == "etalong")
    respond_bool(manipulator.eta_long(rules));
  else if (query == "baseoutputs")
    respond_bool(manipulator.eta_long(rules, true));
  else if (query == "local")
    respond_bool(manipulator.left_linear(rules) &&
                 manipulator.algebraic(rules));
  else if (query == "leftlinear")
    respond_bool(manipulator.left_linear(rules));
  else if (query == "algebraic")
    respond_bool(manipulator.algebraic(rules));
  else if (query == "pfp" || query == "strongpfp") {
    ArList arities = manipulator.get_arities(Sigma, rules);
    if (query == "pfp")
      respond_bool(manipulator.plain_function_passing(rules, arities, sortord));
    else
      respond_bool(manipulator.strong_plain_function_passing(rules, arities,
                   sortord) && manipulator.eta_long(rules, true));
  }
  else if (query == "fullyextended")
    respond_bool(manipulator.fully_extended(rules));
  else if (query == "argumentfree")
    respond_bool(manipulator.argument_free(rules));
  else {
    respond("ERROR");
    if (!silent) cout << "Unknown query: " << query << "\n";
  }
}

string Wanda :: prove_termination(Alphabet &F, vector<MatchRule*> &R,
                                  int handle_nontermination) {
  wout.start_method("termination");

  // start off by removing rules as much as possible, but do not
  // use product polynomials if we're still going to do
  // dependency pairs, because those cause timeouts
  if (allow_redpair) {
    RuleRemover remover(allow_polynomials, allow_horpo,
                         allow_polyprod && !allow_dp);
    if (allow_rulesremoval) remover.remove_rules(F, R);
    else remover.remove_all(F, R);
    if (R.empty()) {
      wout.succeed_method("termination");
      return "YES";
    }
  }

  // with the remaining rules, use the dependency pair framework
  if (allow_dp) {
    DependencyFramework framework(F, R, firstorder, firstordernont,
                                  allow_static_dp, allow_dynamic_dp);
    if (!allow_graph) framework.disable_graph();
    if (!allow_subcrit) framework.disable_subcrit();
    if (!allow_polynomials) framework.disable_polynomials();
    if (!allow_polyprod) framework.disable_product_polynomials();
    if (!allow_horpo) framework.disable_horpo();
    if (!allow_local) framework.disable_abssimple();
    if (!allow_formative) framework.disable_formative();
    if (!allow_usable) framework.disable_usable();
    if (!allow_fwrt) framework.disable_fwrt();
    if (!allow_uwrt) framework.disable_uwrt();

    if (handle_nontermination != 0 && framework.first_order_non_terminating()) {
      wout.abort_method("termination");
      framework.document_non_terminating();
      return "NO";
    }

    else if (framework.terminating()) {
      wout.succeed_method("termination");
      return "YES";
    }

    else if (handle_nontermination == 1 && framework.proved_non_terminating()) {
      wout.abort_method("termination");
      framework.document_non_terminating();
      return "NO";
    }
  }

  // in rare cases, rule removal may catch systems which dependency
  // pairs do not; for these cases, try full rule removal, so
  // including product polynomials, afterwards
  if (allow_redpair && allow_dp) {
    RuleRemover remover(allow_polynomials, allow_horpo, allow_polyprod);
    if (allow_rulesremoval) remover.remove_rules(F, R);
    else remover.remove_all(F, R);
    if (R.empty()) {
      wout.succeed_method("termination");
      return "YES";
    }
  }

  wout.abort_method("termination");
  return "MAYBE";
}

void Wanda :: determine_termination() {
  NonTerminator nonterminator(Sigma, rules, use_betafirst);
  if (allow_nontermination && nonterminator.non_terminating()) {
    respond("NO");
  }
  else {
    // create a copy of the alphabet and rules to try with
    int i;
    Alphabet F;
    vector<MatchRule*> R;
    Sigma.copy(F);
    for (i = 0; i < rules.size(); i++) R.push_back(rules[i]->copy());

    wout.start_method("simplification");

    // if appropriate, simplify the system
    RulesManipulator manipulator;
    bool trying_simplification = false;
    if (!use_betafirst && simplify_meta &&
        manipulator.simplify_applications(F, R)) {
      int k = allow_nontermination ? 2 : 0;
      string simplified_result = prove_termination(F, R, k);
      F.clear();
      for (i = 0; i < R.size(); i++) delete R[i];
      R.clear();

      if (simplified_result == "MAYBE") { // we're going on!
        Sigma.copy(F);
        for (i = 0; i < rules.size(); i++) R.push_back(rules[i]->copy());
      }
      else {
        wout.succeed_method("simplification");
        respond(simplified_result);
        return;
      }
    }

    wout.abort_method("simplification");

    int k = allow_nontermination? 1 : 0;
    respond(prove_termination(F, R, k));
    F.clear();
    for (i = 0; i < R.size(); i++) delete R[i];
    R.clear();
  }
}

void Wanda :: certify_termination_status() {
  // print signature
  wout.formal_print("Signature: " + Sigma.to_string() + "\n");
  
  // print rules
  wout.formal_print("Rules: ");
  wout.formal_print_rules(rules, Sigma);
  wout.formal_print("\n");

  if (!allow_redpair || !allow_polynomials) {
    cout << "MAYBE" << endl <<
      "(Disabled the only technique available for certification.)" <<
      endl;
    return;
  }

  RuleRemover remover(true, false, allow_polyprod, false, true);
  if (allow_rulesremoval) remover.remove_rules(Sigma, rules);
  else remover.remove_all(Sigma, rules);
  
  if (rules.empty()) cout << "YES" << endl;
  else cout << "MAYBE" << endl;

  wout.print_formal_output(outputfile);
}

/** main function **/

int main(int argc, char **argv) {
  vector<string> args;
  for (int i = 1; i < argc; i++) args.push_back(argv[i]);

  Wanda wanda;
  wanda.run(args);
  return 0;
}

