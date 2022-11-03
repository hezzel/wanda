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

#ifndef XMLREADER_H
#define XMLREADER_H

#include <string>
using namespace std;

/**
 * This class transforms XML code into human readable code.
 * Unlike the name suggests, it also goes the other way around.
 */

class XMLReader {
  private:
    bool transform_html;
      // set to true if the input is read only for conversion to
      // html, to false otherwise

    string get_part(string txt, string identifier,
                    int start, int &end);
      // returns the part between the first <name> and the
      // corresponding </name>, where nesting of <name> is taken into
      // account; the tags themselves are not included

    string get_first_part(string txt, string identifier);
      // returns the part between the first (properly nested)
      // occurrence of <name> and </name> from txt, or the empty
      // string if <name> and </name> don't occur

    string fix_name(string name);
      // makes a name adhere to naming standards

    string read_type(string txt, int start, int &end,
                     bool brackets = false);
      // reads a type declaration from txt.substr(start), and
      // sets 'end' to its last character 

    string read_term(string txt, int start, int &end,
                     int brackets = 0);
      // reads a term from txt.substr(start), and sets 'end' to its
      // last character
      // brackets: 0 - no brackets, 1 - brackets if term is an
      // abstraction or application, 2 - brackets if term is an
      // abstraction

    string read_function_declaration(string &txt);
      // reads the first function declaration from the given string

    string read_variable_declaration(string &txt);
      // reads the first variable declaration from the given string

    string read_rule_declaration(string &txt);
      // reads the first rule eclaration from the given string

    void sar(string &str, string from, string to);
      // replaces from by to everywhere in str

  public:
    string read_file(string xml, string &strategy);
    string read_file(string xml);
      // both return TRS if the file is not an AFSM

    bool is_afsm(string xml);
    
    string read_as_html(string xml);
};

#endif

