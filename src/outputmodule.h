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

#ifndef OUTPUTMODULE_H
#define OUTPUTMODULE_H

#include "alphabet.h"
#include "matchrule.h"
#include "polynomial.h"
#include "dependencypair.h"

typedef map<string,int> ArList;

struct Method {
  string name;
  string output;
  set<string> cites;

  Method(string n) {
    name = n;
    output = "";
  }
};

class OutputModule {
  private:
    bool verbose;
    bool debugging;
    bool html;
    bool ansicolour;
    bool useutf;
    map<string,string> citelist;
    bool paragraph_open;
    bool box_open;

    vector<Method> methods;

    string plain_layout(string txt);
    string html_layout(string txt);

    string choose_meta_name(Alphabet &F, PType type,
                            map<int,string> &existing);
    string choose_var_name(Alphabet &F, PType type,
                           map<int,string> &existing1,
                           map<int,string> &existing2);

    string replace_occurrences(string txt, string from, string to);
    string replace_tag(string txt, string tag, string open, string close);
    string get_extension(string filename);
    void initialise_cites();
    void use_cite(string paper);
    string make_citelist();
    string print_type(PType type, TypeNaming &naming, bool brackets = false);
    string print_typedec(PType type, int arity);
    string utf_symbol(string txt);
    string parse_colour(string txt, string tag, string code);
    string parse_colours(string txt);

    string post_parse_poly(string txt);

  public:
    OutputModule();

    /* settings */

    void set_verbose(bool value);
    void set_debugmode(bool value);
    void set_html(bool value);
    void set_use_colour(bool value);
      // only affects non-html mode
    void set_use_utf(bool value);
      // only affects non-html mode

    bool query_verbose();
    bool query_debugging();

    // note: verbose output is always non-html; if verbose and html
    // are both enabled, html output will be given as well as the
    // verbose output (if html is not enabled, then verbose output
    // will be given BEFORE the program's output, and no
    // justification will be printed afterwards)

    /* general printing */

    void print(string txt, bool avoid_paragraphs = false);
    void print_header(string txt);
    void verbose_print(string txt);
      // is only printed if verbose or debug is set to true
    void debug_print(string txt);
      // is only printed if debug is set to true
    void literal_print(string txt);

    void start_table();
    void table_entry(vector<string> &columns);
    void end_table();

    void start_reduction(string term);
    void continue_reduction(string arrow, string term);
    void end_reduction();

    /* special characters */

    string str(int num);
      // returns a string representation of the given number
    string sub(string txt);
      // returns the given text surrounded by subscript tags
    string rule_arrow();
    string reduce_arrow();
    string dp_arrow();
    string beta_arrow();
    string type_arrow();
    string typedec_arrow();
    string beta_symbol();
    string eta_symbol();
    string gamma_symbol();
    string pi_symbol();
    string superterm_symbol();
    string gterm_symbol();
    string geqterm_symbol();
    string geqorgterm_symbol();
    string polgeq_symbol();
    string polg_symbol();
    string rank_reduce_symbol();
    string interpret_left_symbol();
    string interpret_right_symbol();
    string bottom_symbol();
    string in_symbol();
    string projection_symbol();
    string full_projection_symbol();
    string empty_set_symbol();

    string up_symbol(PConstant f);

    /* dealing with "methods" */

    // if you start a method, anything printed will be shown only if
    // the method succeeds; if the method is aborted, then nothing is
    // printed unless verbose is set to true (in which case methods
    // are ignored)
    // you can have multiple methods active at the same time; they
    // are considered layered (so if you start Rule Removal and then
    // Polynomials, anything printed then will only be output to the
    // user if *both* Polynomials and Rule Removal succeed)

    void start_method(string method);
    void abort_method(string method);
    void succeed_method(string method);

    /* dealing with "boxes" */

    // if you start a box, anything printed is put in a grey box in
    // the HTML setting, and indented in the plain setting
    // you can only have one box open at a time, and may not close
    // the method in which the box was defined without first closing
    // the box

    void start_box();
    void end_box();

    /* citations */

    string cite(string paper, string extra = "");

    /* special stuff */

    ArList arities_for_system(Alphabet &Sigma, vector<MatchRule*> &rules);
    string print_term(PTerm term, ArList &arities, Alphabet &F,
                      map<int,string> &metanaming,
                      map<int,string> &freenaming,
                      map<int,string> &boundnaming,
                      bool brackets = false);
    string print_polynomial(PPol p, map<int,int> &freerename,
                            map<int,int> &boundrename);
    string print_polynomial_function(PolynomialFunction *p,
                                     map<int,int> &freerename,
                                     map<int,int> &boundrename);

    string parse_filename(string filename);
    void print_alphabet(Alphabet &Sigma, ArList &arities);
    void print_rules(vector<MatchRule*> &rules, Alphabet &Sigma,
                     ArList &arities);
    void print_DPs(DPSet &dps, Alphabet &Sigma, ArList &arities,
                   bool print_numbers = false);
    void print_system(Alphabet &Sigma, vector<MatchRule*> &rules);
    void print_numeric_graph(vector< vector<bool> > &graph);

    /* printing the output */

    void print_output(string filename = "");
};

extern OutputModule wout; // wanda-out

#endif

