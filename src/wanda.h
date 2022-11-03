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

#ifndef WANDA_H
#define WANDA_H

/**
 * WANDA: the main class, which parses command-line arguments and
 * sets everything else in motion.
 * See README.txt for an explanation of the command-line arguments.
 */

#include "alphabet.h"
#include "matchrule.h"

class Wanda {
  private:
    
    /***** runtime parameters and basic data *****/

    string formalism;
      // if runtime arguments say everything has a certain formalism,
      // then this is saved here
    bool silent;
      // suppress explanation following answer
    bool do_rewriting;
      // instead of termination analysis, just rewrite a term
    string convert_to;
      // if set to a string, then instead of termination analysis,
      // the given system is converted to a system in the mentioned
      // formalism and printed
    string query;
      // instead of termination analysis, return whether the given
      // system has the given property
    string error; 
      // if set, the program displays ERROR and gives the text in
      // error on the next line, then aborts (either the program
      // or the current termination attempt)
    bool aborted;
      // stop the attempt to prove/disprove termination of the
      // current system without further message
    bool use_betafirst;
      // use a beta-first reduction strategy
    bool simplify_meta;
      // try to simplify "pattern" applications Z x1 ... xn in an AFS
      // to meta-variable applications Z[x1,...,xn]
    string firstorder;
      // if runtime arguments provide a certain first-order prover,
      // then this is saved here
    string firstordernont;
      // if runtime arguments provide a certain first-order
      // non-termination prover, then this is saved here
    bool just_show;
      // if this is true, we just show the system and don't prove
      // termination or handle a query or anything
    string outputfile;
      // the file to which the proof should be written; if empty this
      // is just stdout
    bool allow_nontermination;
                              // 2: only in the first-order part
    bool allow_rulesremoval;
    bool allow_dp;
    bool allow_static_dp;
    bool allow_dynamic_dp;
    bool allow_subcrit;
    bool allow_polynomials;
    bool allow_polyprod;
    bool allow_horpo;
    bool allow_formative;
    bool allow_usable;
    bool allow_local;
    bool allow_graph;
    bool allow_uwrt;
    bool allow_fwrt;
      // features which may be disabled by runtime arguments
    int total_yes;
    int total_no;
    int total_maybe;
      // sum answers, for statistics

    /***** term rewriting *****/
    Alphabet Sigma;
    vector<MatchRule*> rules;

    /***** internal functions *****/
    
    bool is_number(string txt);
      // returns whether the given string is a non-negative integer

    string get_extension(string filename);
      // returns the extension, if any

    bool known_formalism(string format);
      // checks whether format is something we can handle

    void parse_runtime_arguments(vector<string> &args);
      // parses arguments and removes everything except the files
      // from args

    void read_system(string filename);
      // reads the given system into an AFSM

    void write_system();
      // prints the system to the user

    void rewrite_term();
      // prompts the user for a term and rewrites it using the
      // current rules

    void check_query();
      // checks whether the given query is satisfies, and outputs
      // YES or NO accordingly

    string prove_termination(Alphabet &F, vector<MatchRule*> &R,
                             int handle_nontermination);
      // main functionality for determine_termination, using a given
      // alphabet and set of rules, and which may handle nontermination
      // checks within the DP framework in three different ways

    void determine_termination();
      // tries to decide whether the current system is terminating,
      // and prints YES/NO/MAYBE, as well as an explanation if silent
      // is not set

    void respond(string answer);
      // respond with YES, NO or MAYBE

    void respond_bool(bool answer);
      // respond with YES if true, NO if false

  public:
    void run(vector<string> args);
};

#endif

