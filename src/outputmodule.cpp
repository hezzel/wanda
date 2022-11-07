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

#include "outputmodule.h"
#include "afs.h"
#include <cstdio>
#include <iostream>
#include <fstream>

#define INDENT "  "

/* =============== BASICS =============== */

OutputModule :: OutputModule()
  :verbose(false), debugging(false), html(false), ansicolour(false),
   useutf(false), paragraph_open(false) {

  initialise_cites();
  start_method("Main");
}

void OutputModule :: set_verbose(bool value) { verbose = value; }
void OutputModule :: set_html(bool value) { html= value; }
void OutputModule :: set_use_colour(bool val) { ansicolour = val; }
void OutputModule :: set_use_utf(bool value) { useutf = value; }

void OutputModule :: set_debugmode(bool value) {
  debugging = value;
  verbose = value;
}

bool OutputModule :: query_verbose() { return verbose; }
bool OutputModule :: query_debugging() { return debugging; }

/* =============== PRINTING =============== */

void OutputModule :: print(string txt, bool avoid_paragraphs) {
  if (txt == "") return;

  if (txt[0] == '\n') {
    if (paragraph_open) {
      methods[methods.size()-1].output += "</p>";
      paragraph_open = false;
    }
    if (verbose) cout << endl;
    print(txt.substr(1));
    return;
  }

  int k = txt.find('\n');
  if (k != string::npos) {
    print(txt.substr(0,k));
    print(txt.substr(k));
    return;
  }

  bool should_open = !avoid_paragraphs;
  if (txt[0] == '<') {
    k = txt.find('>');
    if (k == txt.length()-1) should_open = false;
  }

  if (!paragraph_open && should_open) {
    if (box_open)
      methods[methods.size()-1].output += "<p class=\"boxpar\">";
    else
      methods[methods.size()-1].output += "<p>";
    paragraph_open = true;
  }

  if (verbose) cout << plain_layout(txt);

  if (!verbose || html)
    methods[methods.size()-1].output += txt;
}

void OutputModule :: print_header(string txt) {
  if (box_open) print(INDENT, true);
  print("<header>" + txt + "</header>", true);
}

void OutputModule :: verbose_print(string txt) {
  if (verbose) cout << plain_layout(txt);
}

void OutputModule :: debug_print(string txt) {
  if (debugging) cout << plain_layout(txt) << endl;
}

void OutputModule :: literal_print(string txt) {
  methods[methods.size()-1].output += "<pre>" + txt + "</pre>";
  if (verbose) cout << txt;
}

/* =============== SPECIAL CHARACTERS =============== */

string OutputModule :: str(int num) {
  char tmp[10];
  sprintf(tmp, "%d", num);
  return string(tmp);
}

string OutputModule :: sub(string txt) {
  return "<subscript>" + txt + "</subscript>";
}

string OutputModule :: rule_arrow() {
  return "<rulearrow/>";
}

string OutputModule :: reduce_arrow() {
  return "<rulearrow/>";
}

string OutputModule :: dp_arrow() {
  return "<dparrow/>";
}

string OutputModule :: beta_arrow() {
  return "<betaarrow/>";
}

string OutputModule :: type_arrow() {
  return "<typearrow/>";
}

string OutputModule :: typedec_arrow() {
  return "<typedecarrow/>";
}

string OutputModule :: beta_symbol() {
  return "<beta/>";
}

string OutputModule :: bottom_symbol() {
  return "<bottom/>";
}

string OutputModule :: empty_set_symbol() {
  return "<emptyset/>";
}

string OutputModule :: eta_symbol() {
  return "<eta/>";
}

string OutputModule :: full_projection_symbol() {
  return "<nu/>";
}

string OutputModule :: gamma_symbol() {
  return "<gamma/>";
}

string OutputModule :: gterm_symbol() {
  return "<gterm/>";
}

string OutputModule :: geqterm_symbol() {
  return "<geqterm/>";
}

string OutputModule :: geqorgterm_symbol() {
  return "<geqorgterm/>";
}

string OutputModule :: in_symbol() {
  return "<in/>";
}

string OutputModule :: interpret_left_symbol() {
  return "<leftinterpret/>";
}

string OutputModule :: interpret_right_symbol() {
  return "<rightinterpret/>";
}

string OutputModule :: pi_symbol() {
  return "<pi/>";
}

string OutputModule :: polgeq_symbol() {
  return "<polgeq/>";
}

string OutputModule :: polg_symbol() {
  return "<polg/>";
}

string OutputModule :: projection_symbol() {
  return "<nu/>";
}

string OutputModule :: rank_reduce_symbol() {
  return "<rankreduce/>";
}

string OutputModule :: superterm_symbol() {
  return "<supterm/>";
}

string OutputModule :: up_symbol(PConstant f) {
  return f->query_name() + "^#";
}

/* =============== METHODS =============== */

void OutputModule :: start_method(string method) {
  if (paragraph_open) print("\n");
  methods.push_back(Method(method));
}

void OutputModule :: abort_method(string method) {
  if (methods[methods.size()-1].name != method) {
    cout << "ERROR: trying to abort method '" << method << "' when "
         << "it is not the topmost method.  Can only abort "
         << methods[methods.size()-1].name << "!" << endl;
    return;
  }
  if (method == "Main") {
    cout << "ERROR: cannot abort the Main method!" << endl;
    return;
  }
  methods.pop_back();
  paragraph_open = false;
}

void OutputModule :: succeed_method(string method) {
  if (methods[methods.size()-1].name != method) {
    cout << "ERROR: trying to succeed method '" << method << "' when "
         << "it is not the topmost method.  Can only succeed "
         << methods[methods.size()-1].name << "!" << endl;
    return;
  }
  if (method == "Main") {
    cout << "ERROR: cannot succeed the Main method!" << endl;
    return;
  }
  if (paragraph_open) print("\n");
  string output = methods[methods.size()-1].output;
  set<string> cites = methods[methods.size()-1].cites;
  methods.pop_back();
  methods[methods.size()-1].output += output;
  methods[methods.size()-1].cites.insert(cites.begin(), cites.end());
}

/* =============== BOXES =============== */

void OutputModule :: start_box() {
  if (box_open) {
    cout << "ERROR: trying to open box while it is already open" << endl;
    return;
  }
  if (paragraph_open) print("\n");
  print("<prettybox>");
  start_method("box");
  box_open = true;
}

void OutputModule :: end_box() {
  succeed_method("box");
  print("</prettybox>");
  box_open = false;
}

/* =============== TABLES =============== */

void OutputModule :: start_table() {
  if (paragraph_open) print("\n");
  print("<table>");
}

void OutputModule :: table_entry(vector<string> &columns) {
  if (box_open) print(INDENT, true);
  print("<tr>");
  for (int i = 0; i < columns.size(); i++) {
    print("<td>" + columns[i] + "</td>", true);
  }
  print("</tr>");
}

void OutputModule :: end_table() {
  print("</table>");
}

void OutputModule :: start_reduction(string term) {
  start_table();
  if (box_open) print(INDENT, true);
  print("<tr>");
  print("<doubletd>" + term + "</doubletd>", true);
  print("</tr>");
}

void OutputModule :: continue_reduction(string arrow, string term) {
  vector<string> entry;
  entry.push_back("  " + arrow);
  entry.push_back(term);
  table_entry(entry);
}

void OutputModule :: end_reduction() {
  end_table();
}

/* =============== FINISHING OFF =============== */

void OutputModule :: print_output(string filename) {
  if (methods.size() > 1) {
    cout << "ERROR: did not close method "
         << methods[methods.size()-1].name
         << " before calling print_output!" << endl;
    return;
  }

  if (verbose && !html) return;

  string output = methods[0].output + make_citelist();
  if (html) output = html_layout(output);
  else output = plain_layout(output);

  if (filename == "") cout << output;
  else {
    ofstream ofile(filename.c_str());
    ofile << output;
    ofile.close();
    cout << "Output written to " << filename << "." << endl;
  }
}

string OutputModule :: replace_occurrences(string txt, string from, string to) {
  string start = "";
  while (true) {
    int k = txt.find(from);
    if (k == string::npos) return start + txt;
    start += txt.substr(0,k) + to;
    txt = txt.substr(k+from.length());
  }
}

string OutputModule :: replace_tag(string txt, string tag,
                                   string open, string close) {
  txt = replace_occurrences(txt, "<" + tag + ">", open);
  txt = replace_occurrences(txt, "</" + tag + ">", close);
  return txt;
}

string OutputModule :: make_citelist() {
  string ret = "";
  set<string>::iterator it;
  for (it = methods[0].cites.begin(); it != methods[0].cites.end(); it++) {
    ret += "<citname>" + (*it) + "</citname><citcontents>";
    string desc = citelist[*it];
    desc = replace_tag(desc, "author", "<span class=\"cite author\">", "</span>.  ");
    desc = replace_tag(desc, "title", "<span class=\"cite title'\">", "</span>.  ");
    desc = replace_tag(desc, "booktitle", "In <span class=\"cite booktitle\">", "</span>, ");
    desc = replace_tag(desc, "volume", "volume <span class=\"cite volume\">", "</span> of ");
    desc = replace_tag(desc, "series", "<span class=\"cite series\">", "</span>.  ");
    desc = replace_tag(desc, "pages", "<span class=\"cite pages\">", "</span>, ");
    desc = replace_tag(desc, "publisher", "<span class=\"cite publisher\">", "</span>, ");
    desc = replace_tag(desc, "note", "<span class=\"cite note\">", "</span>, ");
    desc = replace_tag(desc, "year", "<span class=\"cite year\">", "</span>.");
    desc = replace_occurrences(desc, "<journal/>", "In ");
    ret += desc + "</citcontents>";
  }
  if (ret == "") return "";
  else return "<bigheader>Citations</bigheader>" + ret;
}

string OutputModule :: plain_layout(string txt) {
  // types
  txt = replace_tag(txt, "typechildren", "(", ")");
  txt = replace_occurrences(txt, "<nexttypechild/>", ", ");
  txt = replace_occurrences(txt, "<typearrow/>", utf_symbol("->"));
  txt = replace_tag(txt, "typedecchildren", "[", "]");
  txt = replace_occurrences(txt, "<nexttypedecchild/>", " " + utf_symbol("*") + " ");
  txt = replace_occurrences(txt, "<typedecarrow/>", utf_symbol("-->"));

  // terms
  txt = replace_tag(txt, "abstraction", utf_symbol("/\\"), "");
  txt = replace_tag(txt, "binder", "<green>", "</green>.");
  txt = replace_tag(txt, "constant", "<red>", "</red>");
  txt = replace_tag(txt, "metavar", "<blue>", "</blue>");
  txt = replace_tag(txt, "freevariable", "<blue>", "</blue>");
  txt = replace_tag(txt, "boundvariable", "<green>", "</green>");
  txt = replace_tag(txt, "functionchildren", "(", ")");
  txt = replace_occurrences(txt, "<nextfunctionchild/>", ", ");
  txt = replace_tag(txt, "metachildren", "(", ")");
  txt = replace_occurrences(txt, "<nextmetachild/>", ", ");
  txt = replace_occurrences(txt, "<nextapplicationchild/>", utf_symbol(" "));
  txt = replace_tag(txt, "bracket", "(", ")");

  // polynomials
  txt = replace_tag(txt, "freepolvar", "<blue>x", "</blue>");
  txt = replace_tag(txt, "freepolfun", "<blue>F", "</blue>");
  txt = replace_tag(txt, "boundpolvar", "<green>y", "</green>");
  txt = replace_tag(txt, "boundpolfun", "<green>G", "</green>");
  txt = replace_tag(txt, "parameter", "<cyan>a", "</cyan>");
  txt = replace_occurrences(txt, "<funcabstraction/>", utf_symbol("\\"));
  txt = replace_occurrences(txt, "<funcdot/>", ".");
  txt = replace_occurrences(txt, "<addition/>", "<red>+</red>");

  // rules
  txt = replace_occurrences(txt, "<rulearrow/>", utf_symbol("=>"));
  txt = replace_occurrences(txt, "<betaarrow/>", utf_symbol("=>_beta"));
  txt = replace_occurrences(txt, "<dparrow/>", utf_symbol("=#>"));

  // relations
  txt = replace_occurrences(txt, "<gterm/>", utf_symbol("gterm"));
  txt = replace_occurrences(txt, "<geqterm/>", utf_symbol("geqterm"));
  txt = replace_occurrences(txt, "<geqorgterm/>", utf_symbol("gterm") + "?");
  txt = replace_occurrences(txt, "<polgeq/>", utf_symbol(">="));
  txt = replace_occurrences(txt, "<polg/>", utf_symbol(">"));
  txt = replace_occurrences(txt, "<leftinterpret/>", utf_symbol("[["));
  txt = replace_occurrences(txt, "<rightinterpret/>", utf_symbol("]]"));
  txt = replace_occurrences(txt, "<supterm/>", utf_symbol("|>"));
  txt = replace_occurrences(txt, "<rankreduce/>", utf_symbol("[>]"));

  // citations
  txt = replace_tag(txt, "citname", "[", "]  ");
  txt = replace_tag(txt, "citcontents", "", "\n");

  // structure
  txt = replace_tag(txt, "p", "", "\n\n");
  txt = replace_occurrences(txt, "<p class=\"boxpar\">", "<p>  ");
  txt = replace_tag(txt, "table", "", "\n");
  txt = replace_tag(txt, "tr", INDENT, "\n");
  txt = replace_tag(txt, "td", "", " ");
  txt = replace_tag(txt, "doubletd", "", " ");
  txt = replace_tag(txt, "bigheader", "\n<bold>+++ ", " +++</bold>\n\n");
  txt = replace_tag(txt, "header", "<bold>", "</bold>\n\n");
  txt = replace_tag(txt, "pre", "", "");

  // greek
  txt = replace_occurrences(txt, "<beta/>", utf_symbol("beta"));
  txt = replace_occurrences(txt, "<eta/>", utf_symbol("eta"));
  txt = replace_occurrences(txt, "<gamma/>", utf_symbol("gamma"));
  txt = replace_occurrences(txt, "<nu/>", utf_symbol("nu"));
  txt = replace_occurrences(txt, "<pi/>", utf_symbol("pi"));

  // rest
  txt = replace_occurrences(txt, "<bottom/>", utf_symbol("_|_"));
  txt = replace_occurrences(txt, "<in/>", utf_symbol("in"));
  txt = replace_occurrences(txt, "^#", utf_symbol("#"));
  txt = replace_occurrences(txt, "<emptyset/>", utf_symbol("{}"));
  txt = replace_tag(txt, "subscript", "_", "");

  txt = parse_colours(txt);

  // remove all remaining tags and return
  string start = "";
  while (true) {
    int a = txt.find("<");
    if (a == string::npos) break;
    int b = txt.find(">", a);
    start += txt.substr(0,a);
    txt = txt.substr(b+1);
  }
  return start + txt;
}

string OutputModule :: html_layout(string txt) {
    // TODO: all the tags, and replace \n by <br\> _only where necessary_
  replace_occurrences(txt, "\n", "<br/>");
  return txt;
}

/* =============== COLOURS AND UTF8 =============== */

string OutputModule :: utf_symbol(string symbol) {
  if (!useutf) {
    if (symbol == "gterm") return ">";
    if (symbol == "geqterm") return ">=";
    return symbol;
  }
  if (symbol == "=>") return "⇒";
  if (symbol == "=#>") return "⇛";
  if (symbol == "->") return "→";
  if (symbol == "-->") return "⟶";
  if (symbol == "/\\") return "λ";
  if (symbol == "*") return "×";
  if (symbol == " ") return " · ";
  if (symbol == "=>_beta") return "⇒_β";
  if (symbol == "|>") return "▷";
  if (symbol == "[>]") return "⊐";
  if (symbol == "gterm") return "≻";
  if (symbol == "geqterm") return "≽";
  if (symbol == ">=") return "≥";
  if (symbol == "\\") return "Λ";
  if (symbol == "[[") return "⟦";
  if (symbol == "]]") return "⟧";
  if (symbol == "_|_") return "⊥";
  if (symbol == "in") return "∈";
  if (symbol == "beta") return "β";
  if (symbol == "eta") return "η";
  if (symbol == "gamma") return "γ";
  if (symbol == "nu") return "ν";
  if (symbol == "pi") return "π";
  if (symbol == "#") return "♯";
  if (symbol == "{}") return "∅";
  return symbol;
}

string OutputModule :: parse_colour(string txt, string tag, string code) {
  return replace_tag(txt, tag, "\033[" + code + "m", "\033[0m");
}

string OutputModule :: parse_colours(string txt) {
  if (!ansicolour) return txt;
  txt = parse_colour(txt, "bold", "1");
  txt = parse_colour(txt, "red", "31");
  txt = parse_colour(txt, "green", "32");
  txt = parse_colour(txt, "blue", "1;34");
  txt = parse_colour(txt, "cyan", "36");
  return txt;
}

/* =============== PRINTING TYPES =============== */

string OutputModule :: print_type(PType type, TypeNaming &naming,
                                  bool brackets) {
  if (type->query_typevar()) return
    "<typevar>" + type->to_string(naming, brackets) + "</typevar>";

  if (type->query_data()) {
    DataType *dt = dynamic_cast<DataType*>(type);
    string ret = "<typeconstructor>" + dt->query_constructor() +
      "</typeconstructor>";
    if (dt->query_child(0) != NULL) {
      ret += "<typechildren>";
      for (int i = 0; dt->query_child(i) != NULL; i++) {
        if (i != 0) ret += "<nexttypechild/>";
        ret += print_type(dt->query_child(i), naming, false);
      }
      ret += "</typechildren>";
    }
    return ret;
  }

  if (type->query_composed()) {
    string left = print_type(type->query_child(0), naming, true);
    string right = print_type(type->query_child(1), naming, false);
    string ret = left + " " + type_arrow() + " " + right;
    if (brackets) return "(" + ret + ")";
    else return ret;
  }

  return "<unknown type>";
}

string OutputModule :: print_typedec(PType type, int arity) {
  TypeNaming naming;
  if (arity == 0) print_type(type, naming);
  string ret = "<typedecchildren>";
  while (arity > 0) {
    ret += print_type(type->query_child(0), naming, false);
    arity--;
    type = type->query_child(1);
    if (arity != 0) ret += "<nexttypedecchild/>";
  }
  ret += "</typedecchildren> " + typedec_arrow() + " " +
         print_type(type, naming);
  return ret;
}

/* =============== PRINTING TERMS =============== */

string OutputModule :: choose_meta_name(Alphabet &F, PType type,
                                        map<int,string> &existing) {
  set<string> values;
  for (map<int,string>::iterator it = existing.begin();
       it != existing.end(); it++) {
    values.insert(it->second);
  }

  char hnames[] = { 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' };
  char bnames[] = { 'X', 'Y', 'Z', 'U', 'V', 'W', 'Q', 'R', 'S', 'T' };

  for (int i = 0; ; i++) {
    string attempt;
    char tmp[] = " ";
    if (type->query_composed()) tmp[0] = hnames[i%10];
    else tmp[0] = bnames[i%10];
    attempt = string(tmp);
    int k = i / 10;
    for (; k > 0; k--) attempt += "'";
    if (!F.contains(attempt) && values.find(attempt) == values.end())
      return attempt;
  }
}

string OutputModule :: choose_var_name(Alphabet &F, PType type,
                                       map<int,string> &existing1,
                                       map<int,string> &existing2) {
  set<string> values;
  for (map<int,string>::iterator it = existing1.begin();
       it != existing1.end(); it++) {
    values.insert(it->second);
  }
  for (map<int,string>::iterator it = existing2.begin();
       it != existing2.end(); it++) {
    values.insert(it->second);
  }

  char hnames[] = { 'f', 'g', 'h', 'i', 'j', 'k' };
  char bnames[] = { 'x', 'y', 'z', 'u', 'v', 'w' };

  for (int i = 0; ; i++) {
    string attempt;
    char tmp[] = " ";
    if (type->query_composed()) tmp[0] = hnames[i%6];
    else tmp[0] = bnames[i%6];
    attempt = string(tmp);
    int k = i / 6;
    for (; k > 0; k--) attempt += "'";
    if (!F.contains(attempt) && values.find(attempt) == values.end())
      return attempt;
  }
}

string OutputModule :: print_term(PTerm term, ArList &arities,
                                  Alphabet &F,
                                  map<int,string> &metanaming,
                                  map<int,string> &freenaming,
                                  map<int,string> &boundnaming,
                                  bool brackets) {
  
  // deal with variables, both bound and free
  if (term->query_variable()) {
    PVariable x = dynamic_cast<PVariable>(term);
    int index = x->query_index();
    if (boundnaming.find(index) != boundnaming.end()) {
      return "<boundvariable>" + boundnaming[index] +
             "</boundvariable>";
    }
    else {
      if (freenaming.find(index) == freenaming.end())
        freenaming[index] = choose_var_name(F, term->query_type(),
                                            freenaming, boundnaming);
      return "<freevariable>" + freenaming[index] + "</freevariable>";
    }
  }

  // deal with meta-variables
  if (term->query_meta()) {
    MetaApplication *m = dynamic_cast<MetaApplication*>(term);
    PVariable Z = m->get_metavar();
    int index = Z->query_index();
    if (metanaming.find(index) == metanaming.end())
      metanaming[index] = choose_meta_name(F, term->query_type(),
                                           metanaming);
    string ret = "<metaapplication><metavar>" + metanaming[index] +
                 "</metavar>";
    if (term->number_children() > 0) {
      ret += "<metachildren>";
      for (int i = 0; i < term->number_children(); i++) {
        if (i != 0) ret += "<nextmetachild/>";
        ret += print_term(term->get_child(i), arities, F, metanaming,
                          freenaming, boundnaming, false);
      }
      ret += "</metachildren>";
    }
    ret += "</metaapplication>";
    return ret;
  }

  // deal with abstractions
  if (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    PVariable x = abs->query_abstraction_variable();
    int index = x->query_index();
    boundnaming[index] = choose_var_name(F, x->query_type(),
                                         freenaming, boundnaming);
    string ret = "<abstraction><binder>" + boundnaming[index] +
      "</binder><sub>" + print_term(term->get_child(0),
      arities, F, metanaming, freenaming, boundnaming,
      false) + "</abstraction>";
    if (brackets) ret = "<bracket>" + ret + "</bracket>";
    return ret;
  }

  // deal with functional terms, and applications
  vector<PTerm> parts = term->split();
  string ret;
  int n = 1;
  if (parts[0]->query_constant()) {
    PConstant f = dynamic_cast<PConstant>(parts[0]);
    int arity = arities[f->query_name()];
    ret = "<functional><constant>" + f->query_name() + "</constant>";
    if (arity > 0) {
      ret += "<functionchildren>";
      for (int i = 0; i < arity; i++) {
        if (i != 0) ret += "<nextfunctionchild/>";
        ret += print_term(parts[i+1], arities, F, metanaming,
                          freenaming, boundnaming, false);
      }
      ret += "</functionchildren>";
    }
    ret += "</functional>";
    n = arity + 1;
  }
  else if (parts.size() > 0) {
    ret = print_term(parts[0], arities, F, metanaming, freenaming,
                     boundnaming, true);
  }

  if (n < parts.size()) {
    string app = "<application>" + ret;
    for (int i = n; i < parts.size(); i++) {
      app += "<nextapplicationchild/>";
      app += print_term(parts[i], arities, F, metanaming,
                        freenaming, boundnaming, true);
    }
    app += "</application>";
    if (brackets) app = "<bracket>" + app + "</bracket>";
    ret = app;
  }

  return ret;
}

/* =============== CITATIONS =============== */

void OutputModule :: initialise_cites() {
  citelist["FuhGieParSchSwi11"] =
    "<author>C. Fuhs, J. Giesl, M. Parting, P. Schneider-Kamp, "
      "and S. Swiderski</author>"
    "<title>Proving Termination by Dependency Pairs and Inductive "
      "Theorem Proving</title>"
    "<journal/>"
    "<volume>47(2)</volume>"
    "<series>Journal of Automated Reasoning</series>"
    "<pages>133--160</pages>"
    "<year>2011</year>";

  citelist["FuhKop19"] =
    "<author>C. Fuhs, and C. Kop</author>"
    "<title>A static higher-order dependency pair framework</title>"
    "<booktitle>Proceedings of ESOP 2019</booktitle>"
    "<year>2019</year>";

  citelist["Gra95"] =
    "<author>B. Gramlich</author>"
    "<title>Abstract Relations Between Restricted Termination "
      "and Confluence Properties of Rewrite Systems</title>"
    "<journal/>"
    "<volume>24(1-2)</volume>"
    "<series>Fundamentae Informaticae</series>"
    "<pages>3--23</pages>"
    "<year>1995</year>";

  citelist["Kop11"] =
    "<author>C. Kop</author>"
    "<title>Simplifying Algebraic Functional Systems</title>"
    "<booktitle>Proceedings of CAI 2011</booktitle>"
    "<volume>6742</volume>"
    "<series>LNCS</series>"
    "<pages>201--215</pages>"
    "<publisher>Springer</publisher>"
    "<year>2011</year>";

  citelist["Kop12"] =
    "<author>C. Kop</author>"
    "<title>Higher Order Termination</title>"
    "<note>PhD Thesis</note>"
    "<year>2012</year>";

  citelist["Kop13"] =
    "<author>C. Kop</author>"
    "<title>Static Dependency Pairs with Accessibility</title>"
    "<note>Unpublished manuscript, http://cl-informatik.uibk.ac.at/users/kop/static.pdf</note>"
    "<year>2013</year>";

  citelist["Kop13:2"] =
    "<author>C. Kop</author>"
    "<title>StarHorpo with an Eta-Rule</title>"
    "<note>Unpublished manuscript, http://cl-informatik.uibk.ac.at/users/kop/etahorpo.pdf</note>"
    "<year>2013</year>";

  citelist["KusIsoSakBla09"] =
    "<author>K. Kusakari, Y. Isogai, M. Sakai, and F. Blanqui</author>"
    "<title>Static Dependency Pair Method Based On Strong "
      "Computability for Higher-Order Rewrite Systems</title>"
    "<journal/>"
    "<volume>92(10)</volume>"
    "<series>IEICE Transactions on Information and Systems</series>"
    "<pages>2007--2015</pages>"
    "<year>2009</year>";
}

void OutputModule :: use_cite(string paper) {
  methods[methods.size()-1].cites.insert(paper);
}

string OutputModule :: cite(string paper, string extra) {
  use_cite(paper);
  if (extra == "") return "[<cite>" + paper + "</cite>]";
  else return "[<cite>" + paper + "</cite>, " + extra + "]";
}

/* =============== PRINTING THE SYSTEM =============== */

string OutputModule :: parse_filename(string filename) {
  int k = filename.find_last_of('.');
  if (k != string::npos && k > 0) {
    filename = filename.substr(0, k);
  }
  k = filename.find_last_of('/');
  if (k != string::npos) {
    filename = filename.substr(k+1);
  }
  return "<filename>" + filename + "</filename>";

  // TODO: make an html link
}

string OutputModule :: get_extension(string filename) {
  int k = filename.find_last_of('.');
  if (k != string::npos) return filename.substr(k+1);
  else return "";
}

void OutputModule :: print_alphabet(Alphabet &Sigma, ArList &arities) {
  start_table();
  ArList::iterator it;
  for (it = arities.begin(); it != arities.end(); it++) {
    vector<string> entries;
    entries.push_back(it->first);
    entries.push_back(":");
    entries.push_back(print_typedec(Sigma.query_type(it->first), it->second));
    table_entry(entries);
  }
  end_table();
}

void OutputModule :: print_rules(vector<MatchRule*> &rules,
                                 Alphabet &Sigma, ArList &arities) {

  wout.start_table();

  for (int i = 0; i < rules.size(); i++) {
    map<int,string> metanaming, freenaming, boundnaming;
    vector<string> entry;
    entry.push_back(print_term(rules[i]->query_left_side(),
          arities, Sigma, metanaming, freenaming, boundnaming));
    entry.push_back(rule_arrow());
    entry.push_back(print_term(rules[i]->query_right_side(),
           arities, Sigma, metanaming, freenaming, boundnaming));
    table_entry(entry);
  }
  
  wout.end_table();
}

ArList OutputModule :: arities_for_system(Alphabet &Sigma,
                                vector<MatchRule*> &rules) {
  ArList arities;
  vector<string> symbs = Sigma.get_all();
  for (int i = 0; i < symbs.size(); i++) {
    PConstant f = Sigma.get(symbs[i]);
    arities[symbs[i]] = f->query_max_arity();
    delete f;
  }
  for (int j = 0; j < rules.size(); j++) {
    rules[j]->query_left_side()->adjust_arities(arities);
    rules[j]->query_right_side()->adjust_arities(arities);
  }
  return arities;
}

void OutputModule :: print_system(Alphabet &Sigma,
                                  vector<MatchRule*> &rules) {

  ArList arities = arities_for_system(Sigma, rules);
  start_box();
  print_header("Alphabet:");
  print_alphabet(Sigma, arities);
  print_header("Rules:");
  print_rules(rules, Sigma, arities);
  
  end_box();
}

/* =============== SPECIFIC METHODS =============== */

string OutputModule :: post_parse_poly(string txt) {
  int k;

  while ((k = txt.find("a")) != string::npos) txt.replace(k, 1, "A");

  for (int sorts = 0; sorts < 5; sorts++) {
    char c;
    string tag;
    if (sorts == 1) { c = 'x'; tag = "freepolvar"; }
    if (sorts == 0) { c = 'y'; tag = "boundpolvar"; }
    if (sorts == 3) { c = 'F'; tag = "freepolfun"; }
    if (sorts == 2) { c = 'G'; tag = "boundpolfun"; }
    if (sorts == 4) { c = 'A'; tag = "parameter"; }

    while ((k = txt.find(c)) != string::npos) {
      int j = 1;
      for (; k + j < txt.length() && txt[k+j] >= '0' && txt[k+j] <= '9'; j++);
      txt.replace(k, j, "<" + tag + ">" + txt.substr(k+1, j-1) +
                  "</" + tag + ">");
    }
  }

  while ((k = txt.find("\\")) != string::npos) {
    txt.replace(k, 1, "<funcabstraction/>");
  }

  while ((k = txt.find(".")) != string::npos) {
    txt.replace(k, 1, "<funcdot/>");
  }

  while ((k = txt.find("+")) != string::npos) {
    txt.replace(k, 1, "<addition/>");
  }

  return txt;
}

string OutputModule :: print_polynomial(PPol p,
                                        map<int,int> &freerename,
                                        map<int,int> &boundrename) {
  string printed = p->to_string(freerename, boundrename);
  return post_parse_poly(printed);
}

string OutputModule :: print_polynomial_function(PolynomialFunction *p,
                 map<int,int> &freerename, map<int,int> &boundrename) {
  string printed = p->to_string(freerename, boundrename);
  return post_parse_poly(printed);
}

void OutputModule :: print_DPs(vector<DependencyPair*> &dps,
                               Alphabet &Sigma, ArList &arities,
                               bool print_numbers) {
  wout.start_table();

  for (int i = 0; i < dps.size(); i++) {
    map<int,string> metanaming, freenaming, boundnaming;
    vector<string> entry;
    // print number
    if (print_numbers) entry.push_back(str(i) + "]");
    // print left
    entry.push_back(print_term(dps[i]->query_left(),
          arities, Sigma, metanaming, freenaming, boundnaming));
    // print arrow
    entry.push_back(dp_arrow());
    // print right, possibly in extended form
    PTerm right = dps[i]->query_right();
    if (right->query_application() && right->query_head()->query_meta()) {
      vector<PTerm> parts = right->split();
      vector<PTerm> children;
      int i;
      for (i = 0; i < parts[0]->number_children(); i++) {
        children.push_back(parts[0]->get_child(i));
      }
      for (i = 1; i < parts.size(); i++) children.push_back(parts[i]);
      PVariable Z = dynamic_cast<MetaApplication*>(parts[0])->get_metavar();
      Z = dynamic_cast<PVariable>(Z->copy());
      PTerm newright = new MetaApplication(Z, children);
      entry.push_back(print_term(newright, arities, Sigma, metanaming,
                                 freenaming, boundnaming));
      for (i = 0; i < newright->number_children(); i++)
        newright->replace_child(i, NULL);
      delete newright;
    }
    else {
      entry.push_back(print_term(right, arities, Sigma, metanaming,
                                 freenaming, boundnaming));
    }
    // print eating information
    map<int,int> noneating = dps[i]->query_noneating_mapping();
    string eattext = "";
    for (map<int,int>::iterator it = noneating.begin();
         it != noneating.end(); it++) {
      int Z = it->first;
      int pos = it->second;
      if (metanaming.find(Z) != metanaming.end()) {
        if (eattext != "") eattext += ", ";
        eattext += metanaming[Z] + " : " + str(pos);
      }
    }
    if (eattext != "") eattext = "{" + eattext + "}";
    entry.push_back(eattext);
    // print polymorphism information
    if (!dps[i]->query_headmost()) entry.push_back("");
    else entry.push_back("[all applicative instances]");

    table_entry(entry);
  }
  
  wout.end_table();
}

void OutputModule :: print_numeric_graph(vector< vector<bool> > &graph) {
  wout.start_table();
  for (int i = 0; i < graph.size(); i++) {
    vector<string> entry;
    entry.push_back("*");
    entry.push_back(str(i));
    entry.push_back(":");
    string edges = "";
    for (int j = 0; j < graph[i].size(); j++) if (graph[i][j]) {
      if (edges != "") edges += ", ";
      edges += str(j);
    }
    entry.push_back(edges);
    wout.table_entry(entry);
  }
  wout.end_table();
}

OutputModule wout;

